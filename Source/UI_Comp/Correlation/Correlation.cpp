#include "Correlation.h"

PhaseCorrelationAnalyserComponent::PhaseCorrelationAnalyserComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
    addAndMakeVisible(opengl_comp);
    addAndMakeVisible(correl_amnt_comp);
    addAndMakeVisible(volume_meter_comp);

    apvts_ref.getParameter("v_rms_time")->addListener(this);
}

PhaseCorrelationAnalyserComponent::~PhaseCorrelationAnalyserComponent()
{
}

void PhaseCorrelationAnalyserComponent::paint(Graphics& g)
{
    float hue = apvts_ref.getRawParameterValue("ui_acc_hue")->load();
    Colour accentColour = Colour::fromHSV(hue, 0.75, 0.35, 1.0);

    correl_amnt_comp.accent_colour = accentColour;
    volume_meter_comp.accent_colour = accentColour;
}

void PhaseCorrelationAnalyserComponent::resized()
{

    float vol_bounds_width = getWidth() * 0.05;
    float correl_amnt_bounds_height = getHeight() * 0.05;
    auto bounds = getLocalBounds();

    // TODO : this would be at the top if we used a 
    // rotated orientation.
    auto volume_bounds = bounds.removeFromLeft(vol_bounds_width);
    auto corell_amount_bounds = bounds.removeFromBottom(correl_amnt_bounds_height);

    // Now select the biggest square that you can make out of the 
    // remaining bounds justified center.
    int wid = bounds.getWidth();
    int high = bounds.getHeight();

    if (wid > high)
    {
        int toRemove = (wid - high)/2;
        bounds.removeFromLeft(toRemove);
        bounds.removeFromRight(toRemove);
    }
    else 
    {
        int toRemove = (high - wid) / 2;
        bounds.removeFromTop(toRemove);
        bounds.removeFromBottom(toRemove);
    }

    bounds.removeFromTop(correl_amnt_bounds_height);

    opengl_comp.setBounds(bounds.reduced(5));
    correl_amnt_comp.setBounds(corell_amount_bounds);
    volume_meter_comp.setBounds(volume_bounds);

}

void PhaseCorrelationAnalyserComponent::parameterValueChanged(int param_index, float new_value)
{
    float rms_T = apvts_ref.getRawParameterValue("v_rms_time")->load();
    rms_time = rms_T;

    window_length_samples = sample_rate * rms_time;
    update_window_samples = sample_rate / TARGET_TRIGGER_HZ;
}

void PhaseCorrelationAnalyserComponent::prepareToPlay(float SR, float block_size)
{
    sample_rate = SR;

    window_length_samples = sample_rate * rms_time;
    update_window_samples = sample_rate / TARGET_TRIGGER_HZ;

    sample_counter = 0;

    sumLR = 0.0f;
    sumRR = 0.0f;
    sumLL = 0.0f;

    while (SquareQueLR.size() > 0) SquareQueLR.pop();
    while (SquareQueLL.size() > 0) SquareQueLL.pop();
    while (SquareQueRR.size() > 0) SquareQueRR.pop();
}

void PhaseCorrelationAnalyserComponent::zeroOutMeters()
{
    opengl_comp.newDataPoint(0.5f, 0.5f);
    correl_amnt_comp.newPoint(0.5f);
    volume_meter_comp.newPoint(0.0f, 0.0f);
}

void PhaseCorrelationAnalyserComponent::processBlock(AudioBuffer<float>& buffer)
{
    float* left_channel = buffer.getWritePointer(0);
    float* right_channel = buffer.getWritePointer(1);

    int block_size = buffer.getNumSamples();

    if (window_length_samples <= 0)
        return;

    for (int sample = 0; sample < block_size; ++sample)
    {
        float left_sample = left_channel[sample];
        float right_sample = right_channel[sample];

        float right_sample_squared = right_sample * right_sample;
        float left_sample_squared = left_sample * left_sample;
        float left_times_right = left_sample * right_sample;

        sumLR += left_times_right;
        sumRR += right_sample_squared;
        sumLL += left_sample_squared;

        SquareQueLR.push(left_times_right);
        SquareQueLL.push(left_sample_squared);
        SquareQueRR.push(right_sample_squared);

        while (SquareQueLR.size() > window_length_samples)
        {
            sumLR -= SquareQueLR.front();
            SquareQueLR.pop();
        }
        while (SquareQueLL.size() > window_length_samples)
        {
            sumLL -= SquareQueLL.front();
            SquareQueLL.pop();
        }
        while (SquareQueRR.size() > window_length_samples)
        {
            sumRR -= SquareQueRR.front();
            SquareQueRR.pop();
        }

        sample_counter += 1;

        if (sample_counter >= update_window_samples)
        {
            sample_counter = 0;

            sumLL = std::max(sumLL, 0.0f);
            sumRR = std::max(sumRR, 0.0f);

            float denominator = std::sqrtf(sumLL * sumRR);
            if (denominator < 1e-10f || !std::isfinite(denominator))
                denominator = 1e-10f;

            float y_comp = static_cast<float>(sumLR / denominator);

            float rmsL = std::sqrtf(sumLL / (float)window_length_samples);
            float rmsR = std::sqrtf(sumRR / (float)window_length_samples);

            if (!std::isfinite(rmsL) || rmsL < 0.0f) rmsL = 0.0f;
            if (!std::isfinite(rmsR) || rmsR < 0.0f) rmsR = 0.0f;

            float x_comp = 0.0f;
            if (rmsL + rmsR > 1e-10f)
                x_comp = (rmsR - rmsL) / (rmsL + rmsR);

            y_comp = y_comp * 0.5f + 0.5f;
            x_comp = x_comp * 0.5f + 0.5f;

            y_comp = std::clamp(y_comp, 0.0f, 1.0f);
            x_comp = std::clamp(x_comp, 0.0f, 1.0f);

            constexpr float minRmsThreshold = 1e-6f;

            // when vol too low it the balance sometimes goes crazy
            // because it is normalised.
           /* constexpr float fadeStartRms = 1e-3f;
            float avgRms = 0.5f * (rmsL + rmsR);
            float fade = 1.0f;
            if (avgRms < fadeStartRms)
                fade = std::clamp((avgRms - minRmsThreshold) / (fadeStartRms - minRmsThreshold), 0.0f, 1.0f);
            float x_comp_faded = 0.5f + fade * (x_comp - 0.5f);*/

            if (rmsL < minRmsThreshold && rmsR < minRmsThreshold)
            {
                opengl_comp.newDataPoint(0.5f, 0.5f);
                correl_amnt_comp.newPoint(0.5f);
                volume_meter_comp.newPoint(0.0f, 0.0f);
            }
            else
            {
                // Normal processing
                opengl_comp.newDataPoint(left_sample * 0.5f + 0.5f, right_sample * 0.5f + 0.5f);

                correl_amnt_comp.newPoint(y_comp);
                volume_meter_comp.newPoint(rmsL * 1.414f, rmsR * 1.414f);
            }
        }
    }
}

PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::
CorrelationOpenGLComponent()
{
    setOpaque(true);

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);

    opengl_context.setRenderer(this);

    // no overlay for this.
    opengl_context.setComponentPaintingEnabled(false);

    opengl_context.attachTo(*this);
    bool vSync_success = opengl_context.setSwapInterval(1);
    opengl_context.setContinuousRepainting(true);

    if (!vSync_success) DBG("V SYNC NOT SUPPORTED");
    else DBG("V SYNC ENABLED");

}

PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::
~CorrelationOpenGLComponent()
{
    opengl_context.detach();
    send_triggerRepaint = false;

    shader.reset();
    shader_uniforms.reset();
}

void PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::
newDataPoint(float x, float y)
{

    // not locking as the contention is very low and 
    // cannot even be seen.

    int write_index = (ring_buf_read_index.load() + 1) % RING_BUFFER_TEXEL_SIZE;

    ring_buffer[2 * write_index] = x;
    ring_buffer[2 * write_index + 1] = y;

    ring_buf_read_index.store(write_index);

    dirty.store(true);
}

void PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::
newOpenGLContextCreated()
{

    using namespace juce::gl;
    createShaders();

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glGenTextures (1, &dataTexture);
    glBindTexture (GL_TEXTURE_1D, dataTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RG32F,
        RING_BUFFER_TEXEL_SIZE, // each texel is 2 floats.
        0,
        GL_RG,
        GL_FLOAT,
        ring_buffer);

    ring_buf_read_index = 0;

    send_triggerRepaint = true;
    opengl_context.triggerRepaint();
}

void PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::renderOpenGL()
{
    jassert(OpenGLHelpers::isContextActive());

    using namespace ::juce::gl;
    // Setup viewport
    const float renderingScale = (float)opengl_context.getRenderingScale();
    glViewport(0, 0, roundToInt(renderingScale * getWidth()), roundToInt(renderingScale * getHeight()));

    //// Enable alpha blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Make sure there is a shader.
    if (!shader) return;

    shader->use();

    jassert(shader_uniforms->readIndex != nullptr);
    jassert(shader_uniforms->resolution != nullptr);
    jassert(shader_uniforms->pointsTex != nullptr);
    jassert(shader_uniforms->maxIndex != nullptr);

    int read_index_loaded_val = ring_buf_read_index.load();

    if (shader_uniforms)
    {
        shader_uniforms->resolution->
                set((GLfloat)(renderingScale * getWidth()), (GLfloat)(renderingScale * getHeight()));
        shader_uniforms->readIndex->set((GLint)read_index_loaded_val);
        shader_uniforms->pointsTex->set(0);
        shader_uniforms->maxIndex->set(RING_BUFFER_TEXEL_SIZE);
    }

    // Define vertices and indices for a square (full viewport).
    GLfloat vertices[] = {
        1.0f,  1.0f,
        1.0f, -1.0f,
       -1.0f, -1.0f,
       -1.0f,  1.0f,
    };
    GLuint indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    if (dirty.load())
    {
        glTexSubImage1D(
            GL_TEXTURE_1D, 
            0, 
            0,
            RING_BUFFER_TEXEL_SIZE, // each texel is 2 floats.
            GL_RG,
            GL_FLOAT,
            ring_buffer);
        dirty.store(false);
    }

    // Bind and fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    // Bind and fill EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    // Setup vertex attribute pointer (location 0, 3 floats per vertex)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // Cleanup: unbind buffers and disable attribute
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::
openGLContextClosing()
{
    send_triggerRepaint = false;
    opengl_context.setContinuousRepainting(false);


    // Delete OpenGL buffers
    if (VBO != 0)
    {
        opengl_context.extensions.glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0)
    {
        opengl_context.extensions.glDeleteBuffers(1, &EBO);
        EBO = 0;
    }

    // Delete shader program and uniforms
    shader.reset();
    shader_uniforms.reset();

}

void PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::
createShaders()
{
    std::unique_ptr<OpenGLShaderProgram> shaderProgramAttempt = std::make_unique<OpenGLShaderProgram>(opengl_context);

    // Sets up pipeline of shaders and compiles the program
    if (shaderProgramAttempt->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertexShader))
        && shaderProgramAttempt->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(fragmentShader))
        && shaderProgramAttempt->link())
    {
        shader_uniforms.release();
        shader = std::move(shaderProgramAttempt);
        shader_uniforms.reset(new Uniforms(opengl_context, *shader));

        DBG("GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2));
    }
    else
    {
        DBG(shaderProgramAttempt->getLastError());
    }
}

PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::Uniforms::
Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    readIndex.reset(createUniform(OpenGL_Context, shader_program, "readIndex"));
    pointsTex.reset(createUniform(OpenGL_Context, shader_program, "pointsTex"));
    maxIndex.reset(createUniform(OpenGL_Context, shader_program, "maxIndex"));
}

OpenGLShaderProgram::Uniform* PhaseCorrelationAnalyserComponent::
CorrelationOpenGLComponent::Uniforms::createUniform(
    OpenGLContext& gl_context,
    OpenGLShaderProgram& shader_program, 
    const char* uniform_name)
{
    if (
        gl_context.extensions.glGetUniformLocation(shader_program.getProgramID(), uniform_name) < 0)
        return nullptr;

    return new OpenGLShaderProgram::Uniform(shader_program, uniform_name);
}

PhaseCorrelationAnalyserComponent::CorrelAmntComponent::CorrelAmntComponent()
{
    startTimerHz(refreshRate);
}

void PhaseCorrelationAnalyserComponent::CorrelAmntComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::black);

    int padding = std::max(1.0, 0.1 * getHeight());

    auto bounds = getLocalBounds();

    auto bot = bounds.removeFromBottom(padding);
    auto top = bounds.removeFromTop(padding);
    auto left = bounds.removeFromLeft(padding);
    auto right = bounds.removeFromRight(padding);

    g.setColour(accent_colour);

    g.fillRect(bot);
    g.fillRect(top);
    g.fillRect(left);
    g.fillRect(right);

    bounds.reduce(2, 2);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(
        bounds.getX() + (bounds.getWidth() / 2) - 1,
        bounds.getY(),
        2,
        bounds.getHeight()
    );

    g.setColour(juce::Colours::white);

    int bar_width = std::max<int>(3, 0.02 * (float)bounds.getWidth());

    g.fillRect(
        bounds.getX() + (int)((float)(bounds.getWidth() - bar_width) * correl_value.load()),
        bounds.getY(),
        bar_width,
        bounds.getHeight()
    );

}

void PhaseCorrelationAnalyserComponent::CorrelAmntComponent::resized()
{
}

void PhaseCorrelationAnalyserComponent::CorrelAmntComponent::timerCallback()
{
    repaint();
}

void PhaseCorrelationAnalyserComponent::CorrelAmntComponent::newPoint(float y_val)
{
    // one pole smoothing filter.
    float prev_value = correl_value.load();
    float new_value = prev_value + (y_val - prev_value)*alpha;
        
    correl_value.store(new_value);
}

PhaseCorrelationAnalyserComponent::VolumeMeterComponent::VolumeMeterComponent()
{
    startTimerHz(refreshRate);
}

static float gain_to_db(float gain)
{
    gain = std::max<float>(gain, 1e-5);
    return 20.0 * log10f(gain);
}

const float one_seventh_part = 1.0 / 7.0;
// remaps decible values such that they correspond with 
// the meter readings on the seperator.
// -INF|||-60|||-50|||-40|||-30|||-20|||-10|||0
static float custom_remap(float vol_dB)
{
    // 7 parts.
    // 1st part -> -INFdb(here -INF == -100dB :/ ) to -60.

    vol_dB = std::clamp<float>(vol_dB, -100.0, 0.0);

    if (vol_dB < -60)
    {
        return jmap<float>(vol_dB, -100.0, -60.0, 0.0, one_seventh_part);
    }
    else
    {
        return jmap<float>(vol_dB, -60.0, 0.0, one_seventh_part, 1.0);
    }
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::black);

    int padding = std::max(1.0, 0.1 * getWidth());

    auto bounds = getLocalBounds();

    auto bot = bounds.removeFromBottom(padding);
    auto top = bounds.removeFromTop(padding);
    auto left = bounds.removeFromLeft(padding);
    auto right = bounds.removeFromRight(padding);

    g.setColour(accent_colour);

    g.fillRect(bot);
    g.fillRect(top);
    g.fillRect(left);
    g.fillRect(right);

    bounds.reduce(2, 2);


    float l_vol_frac = custom_remap(gain_to_db(
        std::clamp<float>(l_rms_vol.load(), 0.0, 1.0)
    ));
    float r_vol_frac = custom_remap(gain_to_db(
        std::clamp<float>(r_rms_vol.load(), 0.0, 1.0)
    ));

    auto left_bar_bounds = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto right_bar_bounds = bounds;

    if (left_bar_bounds.getWidth() > 1 && right_bar_bounds.getWidth() > 1)
    {
        left_bar_bounds.removeFromRight(1);
        right_bar_bounds.removeFromLeft(1);
    }

    int half_remove_left  = ((1.0 - l_vol_frac) * bounds.getHeight()) / 2;
    int half_remove_right = ((1.0 - r_vol_frac) * bounds.getHeight()) / 2;

    left_bar_bounds.removeFromTop(half_remove_left);
    left_bar_bounds.removeFromBottom(half_remove_left);

    right_bar_bounds.removeFromTop(half_remove_right);
    right_bar_bounds.removeFromBottom(half_remove_right);

    g.setColour(juce::Colours::white);

    g.fillRect(left_bar_bounds);
    g.fillRect(right_bar_bounds);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(
        padding + 2,
        bounds.getY() + (bounds.getHeight() / 2) - 1,
        bounds.getWidth() * 2,
        2
    );
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::resized()
{
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::timerCallback()
{
    repaint();
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::newPoint(float l_vol, float r_vol)
{
    if (std::isnan(l_vol)) l_vol = 0.0f;
    if (std::isnan(r_vol)) r_vol = 0.0f;

    float prev_l_vol = l_rms_vol.load();
    float prev_r_vol = r_rms_vol.load();

    float new_l = prev_l_vol + alpha * (l_vol - prev_l_vol);
    float new_r = prev_r_vol + alpha * (r_vol - prev_r_vol);

    l_rms_vol.store(new_l);
    r_rms_vol.store(new_r);
}