#include "Spectrogram.h"
#include "../util.h"

SpectrogramComponent::SpectrogramComponent(
    AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
    // if there is a change in the amount of history to be stored,
    // clear the existing data.
    apvts_ref.addParameterListener("sp_measure", this);
    apvts_ref.addParameterListener("sp_multiple", this);
    apvts_ref.addParameterListener("gb_fft_ord", this);
    apvts_ref.addParameterListener("gb_vw_mde", this);

    setOpaque(true);

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    opengl_context.setRenderer(this);
    opengl_context.setComponentPaintingEnabled(false);
    opengl_context.setContinuousRepainting(false);
    opengl_context.attachTo(*this);

    startTimer(1000.0f / (float)SPECTROGRAM_FPS);

    bool vSync_success = opengl_context.setSwapInterval(1);
    if (!vSync_success) DBG("V SYNC NOT SUPPORTED");
    else DBG("V SYNC ENABLED");
}

void SpectrogramComponent::timerCallback()
{
    // trigger repaint at fixed fps.
    if (new_data_flag && trigger_repaint)
        opengl_context.triggerRepaint();
}


void SpectrogramComponent::newDataBatch(std::array<std::vector<float>, 32> &data, int valid, int numBins, float bpm, float sample_rate, int N, int D)
{
    numValidBins = numBins;
    int fft_size = (numBins - 1) * 2;
    float fft_bar_measure = apvts_ref.getRawParameterValue("sp_measure")->load();
    float fft_bar_multiple = apvts_ref.getRawParameterValue("sp_multiple")->load();
    int hop_size = HOP_SIZE;

    // float history_length_in_bars = (float)(powf(2.0, fft_bar_measure - 5.0) * fft_bar_multiple);

    // number of frames per column of spectrogram image data.
    // float frames_per_column =
    //     (((history_length_in_bars * 60.0f * (float)N) / bpm)
    //     * (sample_rate / hop_size)) / (float)SPECTROGRAM_MAX_WIDTH;

    static const float measureTable[] = {
        1.0f / 4.0f,
        1.0f / 3.0f,
        1.0f / 7.0f,
        1.0f / 5.0f
    };

    float barsPerWindow =
        measureTable[(int)fft_bar_measure] * fft_bar_multiple;

    float secondsPerBar =
        (60.0f / bpm) * N;

    float historySeconds =
        barsPerWindow * secondsPerBar;

    float fftFramesPerSecond =
        sample_rate / hop_size;

    float frames_per_column =
        (historySeconds * fftFramesPerSecond)
        / (float)SPECTROGRAM_MAX_WIDTH;

    int id = 0;

    // cout << "History seconds: " << historySeconds << endl;
    // cout << "Frames/column: " << frames_per_column << endl;

    while (id < valid) {
        for (int bin = 0; bin < numBins; ++bin)
            spectrogram_data[bin][writeIndex] =
                std::max(spectrogram_data[bin][writeIndex], data[id][bin]);

        if (frames_per_column >= 1.0f) {
            accumulator += 1.0f;

            if (accumulator >= frames_per_column) {
                accumulator -= frames_per_column;

                writeIndex = (writeIndex + 1) % SPECTROGRAM_MAX_WIDTH;
                validColumnsInData = jmin(validColumnsInData + 1, SPECTROGRAM_MAX_WIDTH);
                for (int bin = 0; bin < numValidBins; ++bin)
                    spectrogram_data[bin][writeIndex] = 0.0f;
            }

            id++;
        } else { // Duplication happens implicitly because id does not grow.
            accumulator += frames_per_column;

            if (accumulator >= 1.0f) {
                accumulator -= 1.0f;
                id++;
            }

            writeIndex = (writeIndex + 1) % SPECTROGRAM_MAX_WIDTH;
            validColumnsInData = jmin(validColumnsInData + 1, SPECTROGRAM_MAX_WIDTH);
            for (int bin = 0; bin < numValidBins; ++bin)
                spectrogram_data[bin][writeIndex] = 0.0f;

        }
    }

    new_data_flag = true;
    SR = sample_rate;

}

void SpectrogramComponent::parameterChanged(const String &parameterID, float newValue)
{
    // In case FFT size is changed.
    clearData();
    writeIndex = 0;
    validColumnsInData = 0;
    accumulator = 0.0f;
}

void SpectrogramComponent::clearData()
{
    for (int i = 0; i < SPECTROGRAM_FFT_BINS_MAX; ++i) {
        for (int j = 0; j < SPECTROGRAM_MAX_WIDTH; ++j) {
            spectrogram_data[i][j] = 0.0f;
        }
    }
}

void SpectrogramComponent::newOpenGLContextCreated()
{
    createShaders();

    using namespace juce::gl;

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glGenTextures(1, &dataTexture);
    glBindTexture(GL_TEXTURE_2D, dataTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R32F,
        SPECTROGRAM_MAX_WIDTH,
        SPECTROGRAM_FFT_BINS_MAX,
        0,
        GL_RED,
        GL_FLOAT,
        spectrogram_data
    );

    trigger_repaint = true;
}

void SpectrogramComponent::renderOpenGL()
{
    if (!shader) return;

    using namespace juce::gl;

    const float renderingScale = (float)opengl_context.getRenderingScale();
    glViewport(
        0, 0,
        roundToInt(renderingScale * getWidth()),
        roundToInt(renderingScale * getHeight())
    );

    int scroll_mode = (apvts_ref.getRawParameterValue("gb_vw_mde")->load() > 0.5f) ? 0 : 1;

    shader->use();

    if (shader_uniforms->resolution)
        shader_uniforms->resolution->set(
            renderingScale * getWidth(),
            renderingScale * getHeight());

    if (shader_uniforms->imageData)
        shader_uniforms->imageData->set(0);

    if (shader_uniforms->numBins)
        shader_uniforms->numBins->set(numValidBins.load());

    if (shader_uniforms->startIndex)
        shader_uniforms->startIndex->set(writeIndex.load());

    if (shader_uniforms->numIndex)
        shader_uniforms->numIndex->set(SPECTROGRAM_MAX_WIDTH);

    if (shader_uniforms->minFreq && shader_uniforms->maxFreq) {
        float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
        float max_freq = max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
        shader_uniforms->minFreq->set((GLfloat)min_freq);
        shader_uniforms->maxFreq->set((GLfloat)max_freq);
    }
    
    if (shader_uniforms->sampleRate)
        shader_uniforms->sampleRate->set(SR);

    if (shader_uniforms->scroll)
        shader_uniforms->scroll->set(scroll_mode);

    if (shader_uniforms->bias)
        shader_uniforms->bias->set(apvts_ref.getRawParameterValue("sg_cm_bias")->load());

    if (shader_uniforms->bias)
        shader_uniforms->bias->set(apvts_ref.getRawParameterValue("sg_cm_bias")->load());

    if (shader_uniforms->curve)
        shader_uniforms->curve->set(apvts_ref.getRawParameterValue("sg_cm_curv")->load());

    if (shader_uniforms->blur)
        shader_uniforms->blur->set(((bool)apvts_ref.getRawParameterValue("sg_high_res")->load() == false) ? (GLfloat)0.2f : (GLfloat)0.0f);
    
    const float* clr = getAccentColoursForCode(
        (int)apvts_ref.getRawParameterValue("gb_clrmap")->load());

    shader_uniforms->colorMap_lower->set(clr[0], clr[1], clr[2]);
    shader_uniforms->colorMap_higher->set(clr[3], clr[4], clr[5]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dataTexture);

    if (new_data_flag)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            SPECTROGRAM_MAX_WIDTH,
            numValidBins.load(),
            GL_RED,
            GL_FLOAT,
            spectrogram_data);

        new_data_flag = false;
    }

    GLfloat verts[] = { 1,1, 1,-1, -1,-1, -1,1 };
    GLuint inds[]  = { 0,1,3, 1,2,3 };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STREAM_DRAW);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glDisableVertexAttribArray(0);
}

void SpectrogramComponent::openGLContextClosing() {
    using namespace juce::gl;

    trigger_repaint = false;

    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);

    VBO = 0;
    EBO = 0;

    if (dataTexture)
        glDeleteTextures(1, &dataTexture);

    shader.reset();
    shader_uniforms.reset();
}

void SpectrogramComponent::resized()
{
    if (trigger_repaint) opengl_context.triggerRepaint();
}

void SpectrogramComponent::createShaders()
{
    std::unique_ptr<OpenGLShaderProgram> attempt = std::make_unique<OpenGLShaderProgram>(opengl_context);

    if (attempt->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(vertexShader))
        && attempt->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(fragmentShader))
        && attempt->link())
    {
        shader = std::move(attempt);
        shader_uniforms.reset(new Uniforms(opengl_context, *shader));
    }
}

SpectrogramComponent::Uniforms::Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    imageData.reset(createUniform(OpenGL_Context, shader_program, "imageData"));
    colorMap_lower.reset(createUniform(OpenGL_Context, shader_program, "colorMap_lower"));
    colorMap_higher.reset(createUniform(OpenGL_Context, shader_program, "colorMap_higher"));
    numBins.reset(createUniform(OpenGL_Context, shader_program, "numBins"));
    startIndex.reset(createUniform(OpenGL_Context, shader_program, "startIndex"));
    numIndex.reset(createUniform(OpenGL_Context, shader_program, "numIndex"));
    minFreq.reset(createUniform(OpenGL_Context, shader_program, "minFreq"));
    maxFreq.reset(createUniform(OpenGL_Context, shader_program, "maxFreq"));
    sampleRate.reset(createUniform(OpenGL_Context, shader_program, "sampleRate"));
    scroll.reset(createUniform(OpenGL_Context, shader_program, "scroll"));
    bias.reset(createUniform(OpenGL_Context, shader_program, "bias"));
    curve.reset(createUniform(OpenGL_Context, shader_program, "curve"));
    blur.reset(createUniform(OpenGL_Context, shader_program, "blur"));
}

OpenGLShaderProgram::Uniform* SpectrogramComponent::Uniforms::createUniform(
    OpenGLContext& gl_context,
    OpenGLShaderProgram& shader_program,
    const char* uniform_name)
{
    if (gl_context.extensions.glGetUniformLocation(shader_program.getProgramID(), uniform_name) < 0)
        return nullptr;
    return new OpenGLShaderProgram::Uniform(shader_program, uniform_name);
}
