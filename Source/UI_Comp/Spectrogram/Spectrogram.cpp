#include "Spectrogram.h"
#include "../util.h"
#include <cstdio>
#include <cmath>

SpectrogramComponent::SpectrogramComponent(
    AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
{
    // if there is a change in the amount of history to be stored,
    // clear the existing data.
    apvts_ref.addParameterListener("gb_vw_mde", this);
    apvts_ref.addParameterListener("gb_chnl", this);
    apvts_ref.addParameterListener("gb_fft_ord", this);
    apvts_ref.addParameterListener("sp_measure", this);
    apvts_ref.addParameterListener("sp_multiple", this);

    setOpaque(true);

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    opengl_context.setRenderer(this);
    opengl_context.setComponentPaintingEnabled(true);
    opengl_context.setContinuousRepainting(false);
    opengl_context.attachTo(*this);
    
    setRepaintsOnMouseActivity(true);
    startTimerHz(30); // Timer for overlay updates when data changes (scrolling, panning)

    bool vSync_success = opengl_context.setSwapInterval(1);
    if (!vSync_success) DBG("V SYNC NOT SUPPORTED");
    else DBG("V SYNC ENABLED");

    parameterChanged("", 0.0f);
}

SpectrogramComponent::~SpectrogramComponent()
{
    apvts_ref.removeParameterListener("gb_vw_mde", this);
    apvts_ref.removeParameterListener("gb_chnl", this);
    apvts_ref.removeParameterListener("gb_fft_ord", this);
    apvts_ref.removeParameterListener("sp_measure", this);
    apvts_ref.removeParameterListener("sp_multiple", this);

    opengl_context.detach();
}

void SpectrogramComponent::timerCallback()
{
    // Trigger OpenGL render at fixed fps
    if (trigger_repaint)
        opengl_context.triggerRepaint();
    // Repaint overlay when data changes (scrolling, panning)
    if (mouseOver && new_data_flag)
        repaint();
}

void SpectrogramComponent::mouseEnter(const juce::MouseEvent& e) {
    mouseOver = true;
    lastMousePos = e.getPosition();
    repaint();
}

void SpectrogramComponent::mouseExit(const juce::MouseEvent&) {
    mouseOver = false;
    repaint();
}

void SpectrogramComponent::mouseMove(const juce::MouseEvent& e) {
    lastMousePos = e.getPosition();
    repaint();
}

float SpectrogramComponent::getFrequencyFromNormalizedY(float normalizedY) const {
    float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
    float max_freq = std::max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
    // Apply logarithmic frequency mapping (same as freqFromNorm in shader)
    return min_freq * std::pow(max_freq / min_freq, normalizedY);
}

void SpectrogramComponent::getFrequencyToNoteBuf(float frequency, char* buf) const {
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (frequency <= 0.0f) {
        std::snprintf(buf, 8, "-");
        return;
    }
    int midiNote = int(69 + 12 * std::log2(frequency / 440.0f) + 0.5f);
    int noteIdx = (midiNote % 12 + 12) % 12; // always positive
    int octave = midiNote / 12 - 1;
    std::snprintf(buf, 8, "%s%d", noteNames[noteIdx], octave);
}

juce::String SpectrogramComponent::frequencyToNote(float frequency) {
    char buf[8];
    getFrequencyToNoteBuf(frequency, buf);
    return juce::String(buf);
}

void SpectrogramComponent::paint(Graphics& g) {
    if (!mouseOver) return;  // Only show overlay when mouse is over
    
    // Get normalized Y coordinate (0 = top = high frequency, 1 = bottom = low frequency)
    float normalizedY = juce::jlimit(0.0f, 1.0f, (getHeight() - lastMousePos.y) / (float)std::max(1, getHeight()));
    float freq = getFrequencyFromNormalizedY(normalizedY);
    
    // Get normalized X coordinate (column index)
    float normalizedX = juce::jlimit(0.0f, 1.0f, lastMousePos.x / (float)std::max(1, getWidth()));
    int col = int(normalizedX * validColumnsInData.load());
    col = juce::jlimit(0, validColumnsInData.load() - 1, col);
    
    // Get bin from frequency
    float sampleRate = SR;
    int numBins = numValidBins.load();
    float fftSize = float(2 * (numBins - 1));
    int bin = juce::jlimit(0, numBins - 1, (int)(freq * fftSize / sampleRate));
    
    // Get value from spectrogram data
    float value = spectrogram_data[bin][col];
    
    // Convert normalized dB (0-1) back to actual dB range (-80 to 0)
    float dB = value * 80.0f - 80.0f;
    
    // Get note name
    char noteBuf[8];
    getFrequencyToNoteBuf(freq, noteBuf);
    
    // Build text using char buffer to avoid encoding issues
    char textBuf[256];
    std::snprintf(textBuf, sizeof(textBuf), "Freq: %.1f Hz\nNote: %s\nLevel: %.2f dB", freq, noteBuf, dB);
    juce::String text(textBuf);

    auto bounds = getLocalBounds().removeFromRight(120).removeFromBottom(50);
    bounds.reduce(4, 4);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));
    g.drawFittedText(text, bounds.reduced(4), juce::Justification::topLeft, 3);
}

void SpectrogramComponent::newDataBatch(std::array<std::vector<float>, 32> &data, int valid, int numBins, float bpm, float sample_rate, int N, int D)
{
    numValidBins = numBins;
    int fft_size = (numBins - 1) * 2;
    float fft_bar_measure = apvts_ref.getRawParameterValue("sp_measure")->load();
    float fft_bar_multiple = apvts_ref.getRawParameterValue("sp_multiple")->load();
    int hop_size = HOP_SIZE;

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

    float totalFramesForHistory =
        historySeconds * fftFramesPerSecond;

    int numColumnsNeeded = jlimit(1, SPECTROGRAM_MAX_WIDTH, (int) totalFramesForHistory);

    validColumnsInData = numColumnsNeeded;

    float frames_per_column =
        totalFramesForHistory / (float) numColumnsNeeded;

    // Process each incoming FFT frame
    for (int id = 0; id < valid; ++id) {
        // Write data to current column
        for (int bin = 0; bin < numBins; ++bin)
            spectrogram_data[bin][writeIndex] = data[id][bin];

        accumulator += 1.0f;

        // Advance to next column when we've accumulated enough
        if (accumulator >= frames_per_column) {
            accumulator -= frames_per_column;
            writeIndex = (writeIndex + 1) % numColumnsNeeded;
            
            // Clear next column
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
    validColumnsInData = SPECTROGRAM_MAX_WIDTH; // Will be updated in next newDataBatch
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

    glGenTextures(1, &colourMapTexture);
    glBindTexture(GL_TEXTURE_1D, colourMapTexture);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const float* cmap = getColourMapForCode(
        (int)apvts_ref.getRawParameterValue("gb_clrmap")->load()
    );

    glTexImage1D(GL_TEXTURE_1D,
                0,
                GL_RGB32F,
                COLOUR_MAP_NUM_COLOURS,
                0,
                GL_RGB,
                GL_FLOAT,
                cmap);

    trigger_repaint = true;

    parameterChanged("", 0.0f);

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

    if (shader_uniforms->validColumns)
        shader_uniforms->validColumns->set(validColumnsInData.load());

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
    
    const float* clr = getColourMapForCode((int)apvts_ref.getRawParameterValue("gb_clrmap")->load());

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
    
    if (shader_uniforms->colourMapTex)
        shader_uniforms->colourMapTex->set(1);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, colourMapTexture);

    glTexSubImage1D(GL_TEXTURE_1D,
                0,
                0,
                COLOUR_MAP_NUM_COLOURS,
                GL_RGB,
                GL_FLOAT,
                clr);

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

    if (colourMapTexture)
        glDeleteTextures(1, &colourMapTexture);

    colourMapTexture = 0;

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
    colourMapTex.reset(createUniform(OpenGL_Context, shader_program, "colourMapTex"));
    numBins.reset(createUniform(OpenGL_Context, shader_program, "numBins"));
    startIndex.reset(createUniform(OpenGL_Context, shader_program, "startIndex"));
    numIndex.reset(createUniform(OpenGL_Context, shader_program, "numIndex"));
    validColumns.reset(createUniform(OpenGL_Context, shader_program, "validColumns"));
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