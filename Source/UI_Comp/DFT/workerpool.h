#include <thread>
#include <concepts>
#include <functional>
#include <variant>
#include <vector>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <string>
#include <future>

/*
    * WorkerPool class that manages a pool of worker threads to execute tasks concurrently.
    * should be used from a single thread, and is not thread-safe for concurrent access.
    * ====== NOTE please include void() in AllowedSignatures =======
*/

// constructible_from can also allow lambdas that can be converted to std::function, which is a nice bonus.
template<typename Func, typename... AllowedSignatures>
concept MatchAllowedSignature = (std::constructible_from<std::function<AllowedSignatures>, Func> || ...);

template<typename... AllowedSignatures>
class WorkerPool {
public:

    using AllowedFunction = std::variant<std::function<AllowedSignatures>...>;

    struct Metrics {
        size_t tasks_submitted;
        size_t tasks_completed;
        double avg_wait_ms;
        double avg_exec_ms;

        std::string to_string() const {
            return "Tasks submitted: " + std::to_string(tasks_submitted) + "\n"
                + "Tasks completed: " + std::to_string(tasks_completed) + "\n"
                + "Avg wait time:   " + std::to_string(avg_wait_ms)     + " ms\n"
                + "Avg exec time:   " + std::to_string(avg_exec_ms)     + " ms\n";
        }
    };

    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using TimedTask = std::pair<std::pair<AllowedFunction, size_t>, TimePoint>;

    /*
        * Constructor.
        * @param num_workers: The number of worker threads to create in the pool.
        * @param task_init_callback: Optional callback function that is called when a task is initialized. Returns task ID assigned when calling submit_work.
        * @param task_completed_callback: Optional callback function that is called when a task is completed. Returns task ID assigned when calling submit_work.
    
        * MAKE SURE TO KEEP THE CALLBACKS LIGHTWEIGHT, AS THEY ARE CALLED IN THE CRITICAL PATH OF TASK EXECUTION. 
        * ANY HEAVY OPERATION IN THE CALLBACKS CAN SIGNIFICANTLY AFFECT PERFORMANCE.
    */
    WorkerPool(
        size_t num_workers,
        std::function<void(size_t)> task_init_callback = nullptr,
        std::function<void(size_t)> task_completed_callback = nullptr
    ) : num_workers(num_workers),
        worker_busy_states(num_workers),
        worker_running_states(num_workers),
        task_init_callback_ref(task_init_callback),
        task_completed_callback_ref(task_completed_callback) {

        for (size_t i = 0; i < num_workers; ++i) {
            worker_running_states[i].store(true);
            
            workers.emplace_back(
                worker_loop, 
                i, 
                __task_init_callback__, 
                __task_completed_callback__, 
                std::ref(worker_running_states[i]), 
                std::ref(worker_busy_states[i]), 
                std::ref(task_queue), 
                std::ref(task_queue_mutex), 
                std::ref(task_queue_cv),
                std::ref(metrics_total_wait_ns),
                std::ref(metrics_total_exec_ns),
                std::ref(metrics_tasks_completed)
            );
        }

    }

    ~WorkerPool() {
        
        for (size_t i = 0; i < num_workers; ++i) {
            worker_running_states[i].store(false);
        }

        task_queue_cv.notify_all();

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

    }

    /*
        * Get performance metrics for the worker pool.
        * @return: Metrics struct containing task submission count, completion count, and average wait/exec times.
        * NOTE : avg_exec_ms includes task_init_callback and task_completed_callback time
    */
    Metrics get_metrics() const {
        size_t completed = metrics_tasks_completed.load(std::memory_order_acquire);
        double avg_wait  = completed > 0
            ? (metrics_total_wait_ns.load(std::memory_order_relaxed) / 1e6) / completed : 0.0;
        double avg_exec  = completed > 0
            ? (metrics_total_exec_ns.load(std::memory_order_relaxed) / 1e6) / completed : 0.0;
        return Metrics{
            metrics_tasks_submitted.load(std::memory_order_relaxed),
            completed,
            avg_wait,
            avg_exec
        };
    }
    
    template<typename Func>
    requires MatchAllowedSignature<Func , AllowedSignatures...>
    /*
        * FIRE AND FORGET
        * Submit a task to the worker pool. The task must be a std::function with one of the allowed signatures specified in the template parameters.
        * @param f: The task function to execute. Must match one of the allowed signatures.
        * @return: Unique task ID that can be used to track the task in callbacks.
    */
    size_t submit_work(Func&& f) {

        size_t id = task_id_counter;
        task_id_counter.fetch_add(1, std::memory_order_relaxed);

        metrics_tasks_submitted.fetch_add(1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(task_queue_mutex);
            task_queue.emplace(
                std::make_pair(std::forward<Func>(f), id),
                Clock::now()
            );
        }

        _assign_available_task_to_worker_();
        return id;

    }

    /*
        * Submit a task and get a future for its return value.
        * @param f: Any callable. Return type is inferred automatically.
        * @return: std::future<R> where R is the return type of f.
        * NOTE: Blocks the calling thread if you call .get() before the task completes.
        * NOTE: f.get() unblocks as soon as the task finishes, before task_completed_callback fires.
        *       Use the future for results, callbacks only for lightweight bookkeeping.
    */
    template<typename F>
    auto submit_and_get(F&& f) -> std::future<std::invoke_result_t<F>> {
        static_assert(
            (std::is_same_v<std::function<void()>, std::function<AllowedSignatures>> || ...),
            "submit_and_get requires void() to be in AllowedSignatures. "
            "Use SimpleWorkerPool or add void() to your WorkerPool template parameters."
        );

        using R = std::invoke_result_t<F>;

        // packaged_task wraps f and wires it to a future.
        // shared_ptr because the lambda below needs to own it, outlives this function.
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(f));
        
        std::future<R> fut = task->get_future();

        // Push directly to the queue as void(), bypasses MatchAllowedSignature
        // so submit_and_get works on any WorkerPool regardless of AllowedSignatures.
        size_t id = task_id_counter;
        task_id_counter.fetch_add(1, std::memory_order_relaxed);

        metrics_tasks_submitted.fetch_add(1, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(task_queue_mutex);
            task_queue.emplace(
                std::make_pair(AllowedFunction{ std::function<void()>([task]() { (*task)(); }) }, id),
                Clock::now()
            );
        }

        _assign_available_task_to_worker_();
        return fut;
    }

    int get_num_workers() const { return num_workers; }
    
    /*
        * Get the maximum number of hardware threads available on the system.
    */
    static int get_max_hardware_concurrency() {
        return std::thread::hardware_concurrency();
    }
    
private:
    
    // called when submit_task is called or when task completes(if task queue is not empty).
    inline void _assign_available_task_to_worker_() {
        // If nobody is listening, notify is ignored.
        task_queue_cv.notify_one();
    }    
    
    // thread loop function for worker threads
    static void worker_loop(
        size_t worker_id,
        std::function<void(size_t, size_t)> task_init_callback,
        std::function<void(size_t, size_t)> task_completed_callback,
        std::atomic<bool>& running,
        std::atomic<bool>& busy,
        std::queue<TimedTask>& task_queue,
        std::mutex& mtx,
        std::condition_variable& cv,
        std::atomic<uint64_t>& total_wait_ns,
        std::atomic<uint64_t>& total_exec_ns,
        std::atomic<size_t>&   tasks_completed
    ) {
        for (;;) {
            TimedTask timed_task;
            
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&] {
                    return !task_queue.empty() || !running.load();
                });
                if (!running.load() && task_queue.empty())
                    return;
                timed_task = std::move(task_queue.front());
                task_queue.pop();
                busy.store(true, std::memory_order_relaxed);
            }

            auto& [task_and_id, submit_time] = timed_task;
            auto& [task, task_id]            = task_and_id;

            auto start_time = Clock::now();
            total_wait_ns.fetch_add(
                std::chrono::duration_cast<std::chrono::nanoseconds>(start_time - submit_time).count(),
                std::memory_order_relaxed
            );

            if (task_init_callback)
                task_init_callback(task_id, worker_id);

            std::visit([](auto&& func) { func(); }, task);

            if (task_completed_callback)
                task_completed_callback(task_id, worker_id);

            total_exec_ns.fetch_add(
                std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start_time).count(),
                std::memory_order_relaxed
            );

            // release pairs with acquire in get_metrics — ensures wait/exec accumulations are visible
            tasks_completed.fetch_add(1, std::memory_order_release);
            busy.store(false, std::memory_order_release);
        }

    }

    size_t num_workers;
    
    std::queue<TimedTask> task_queue;

    std::mutex task_queue_mutex;
    std::condition_variable task_queue_cv;
    std::atomic<size_t> task_id_counter{0};
    
    std::vector<std::atomic<bool>> worker_busy_states;
    std::vector<std::atomic<bool>> worker_running_states;
    
    std::function<void(size_t)> task_init_callback_ref;
    std::function<void(size_t)> task_completed_callback_ref;
    
    std::function<void(size_t, size_t)> __task_init_callback__ = [this] (size_t task_id, size_t worker_id) {
        if (task_init_callback_ref)
        task_init_callback_ref(task_id);
    };
    
    std::function<void(size_t, size_t)> __task_completed_callback__ = [this] (size_t task_id, size_t worker_id) {
        if (task_completed_callback_ref)
        task_completed_callback_ref(task_id);
    };
    
    std::atomic<size_t>   metrics_tasks_submitted{0};
    std::atomic<size_t>   metrics_tasks_completed{0};
    std::atomic<uint64_t> metrics_total_wait_ns{0};
    std::atomic<uint64_t> metrics_total_exec_ns{0};
    
    std::vector<std::thread> workers;

};

/*
    * Convenience alias for the most common use case — fire-and-forget void tasks.
    * Callers capture arguments and results via lambdas.
    * For multiple function signatures, use WorkerPool<Sig1, Sig2, ...> directly.
*/
class VoidVoidWorkerPool : public WorkerPool<void()> {
public:
    using WorkerPool<void()>::WorkerPool;
};