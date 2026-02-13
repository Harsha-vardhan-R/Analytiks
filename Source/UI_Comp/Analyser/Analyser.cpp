#include "Analyser.h"
#include <cstdio>

SpectrumAnalyserComponent::SpectrumAnalyserComponent(
    AudioProcessorValueTreeState& apvts_reference,
    std::function<void(string)>& label_callback)
    : apvts_ref(apvts_reference)
{
    setOpaque(true);

    for (int i = 0; i < AMPLITUDE_DATA_SIZE; ++i) {
        amplitude_data[i] = 0.0f;
        ribbon_data[i] = 0.0f;
    }

    opengl_context.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    opengl_context.setRenderer(this);
    opengl_context.setComponentPaintingEnabled(true);
    opengl_context.setContinuousRepainting(false);
    opengl_context.attachTo(*this);
    
    bool vSync_success = opengl_context.setSwapInterval(1);
    if (!vSync_success) DBG("V SYNC NOT SUPPORTED");
    else DBG("V SYNC ENABLED");
    
    setRepaintsOnMouseActivity(true);
    startTimerHz(30); // For overlay updates
}
void SpectrumAnalyserComponent::mouseEnter(const juce::MouseEvent& e) {
    mouseOver = true;
    lastMousePos = e.getPosition();
    setMouseCursor(MouseCursor::CrosshairCursor);
    repaint();
}

void SpectrumAnalyserComponent::mouseExit(const juce::MouseEvent&) {
    mouseOver = false;
    setMouseCursor(MouseCursor::NormalCursor);
    repaint();
}

void SpectrumAnalyserComponent::mouseMove(const juce::MouseEvent& e) {
    lastMousePos = e.getPosition();
    repaint();
}

SpectrumAnalyserComponent::~SpectrumAnalyserComponent()
{
}

void SpectrumAnalyserComponent::prepareToPlay(float SampleRate, float BlockSize)
{
    SR = SampleRate;
}

void SpectrumAnalyserComponent::clearData()
{
    for (int i = 0; i < AMPLITUDE_DATA_SIZE; ++i) {
        amplitude_data[i] = 0.0f;
        ribbon_data[i] = 0.0f;
    }
}

void SpectrumAnalyserComponent::timerCallback()
{
    if (send_triggerRepaint) opengl_context.triggerRepaint();
    repaint();
}
// Paint overlay in paint() override
void SpectrumAnalyserComponent::paint(Graphics& g)
{
    drawOverlay(g);
}
float SpectrumAnalyserComponent::getTopFrequency(float& outAmplitude) const {
    float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
    float max_freq = std::max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
    float sampleRate = SR.load();
    int numBins = bins_number.load();
    float fftSize = float(2 * (numBins - 1));
    int minBin = std::max(0, int(min_freq * fftSize / sampleRate));
    int maxBin = std::min(numBins - 1, int(max_freq * fftSize / sampleRate));
    float maxAmp = 0.0f;
    int maxIdx = minBin;
    for (int i = minBin; i <= maxBin; ++i) {
        if (amplitude_data[i] > maxAmp) {
            maxAmp = amplitude_data[i];
            maxIdx = i;
        }
    }
    outAmplitude = maxAmp;
    float freq = (sampleRate * maxIdx) / fftSize;
    return freq;
}

// Overlay: Convert frequency to note name and write to buffer
void SpectrumAnalyserComponent::getFrequencyToNoteBuf(float frequency, char* buf) const {
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

// Overlay: Convert frequency to note name
juce::String SpectrumAnalyserComponent::frequencyToNote(float frequency) {
    char buf[8];
    getFrequencyToNoteBuf(frequency, buf);
    return juce::String(buf);
}

// Overlay: Get overall volume (RMS of visible bins)
float SpectrumAnalyserComponent::getVolumeLevel() const {
    float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
    float max_freq = std::max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
    float sampleRate = SR.load();
    int numBins = bins_number.load();
    float fftSize = float(2 * (numBins - 1));
    int minBin = std::max(0, int(min_freq * fftSize / sampleRate));
    int maxBin = std::min(numBins - 1, int(max_freq * fftSize / sampleRate));
    float sum = 0.0f;
    int count = 0;
    for (int i = minBin; i <= maxBin; ++i) {
        sum += amplitude_data[i] * amplitude_data[i];
        ++count;
    }
    return count > 0 ? std::sqrt(sum / count) : 0.0f;
}

// Overlay: Draw overlay text
void SpectrumAnalyserComponent::drawOverlay(juce::Graphics& g) {
    float amp = 0.0f;
    float freq = 0.0f;
    char noteBuf[8] = { '-', '\0', 0, 0, 0, 0, 0, 0 };
    float vol = 0.0f;

    if (mouseOver) {

        g.setColour(juce::Colours::white.withAlpha(0.6f));

        // Snap to pixel centre (avoids blurry 0.5px lines on retina)
        float x = std::floor((float)lastMousePos.x);
        float y = std::floor((float)lastMousePos.y);

        // Vertical line
        g.drawLine(x, 0.0f, x, (float)getHeight(), 1.0f);

        // Horizontal line
        g.drawLine(0.0f, y, (float)getWidth(), y, 1.0f);

        // Get frequency under mouse X position using logarithmic mapping (same as shader)
        float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
        float max_freq = std::max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
        float sampleRate = SR.load();
        int numBins = bins_number.load();
        float fftSize = float(2 * (numBins - 1));
        
        // Map mouse position to normalized 0-1 coordinate
        float t = juce::jlimit(0.0f, 1.0f, (getHeight() - lastMousePos.y) / (float)std::max(1, getHeight()));
        // Apply logarithmic frequency mapping (same as freqFromNorm in shader)
        freq = min_freq * std::pow(max_freq / min_freq, t);
        
        // Get bin and amplitude
        int bin = juce::jlimit(0, (int)numBins - 1, (int)(freq * fftSize / sampleRate));
        amp = amplitude_data[bin];
        
        // Get note as char buffer
        getFrequencyToNoteBuf(freq, noteBuf);
        // Volume = amplitude at this bin (not RMS)
        vol = amp;
    } else {
        freq = getTopFrequency(amp);
        getFrequencyToNoteBuf(freq, noteBuf);
        vol = getVolumeLevel();
    }

    // Convert normalized dB (0-1) back to actual dB range (-80 to 0)
    // The amplitude_data is mapped from -80 to 0 dB into 0-1 range
    float dB = vol * 80.0f - 80.0f;
    
    // Build text using char buffer to avoid encoding issues
    char textBuf[256];
    if (mouseOver) {
        std::snprintf(textBuf, sizeof(textBuf), "Freq: %.1f Hz\nNote: %s\nLevel: %.2f dB", freq, noteBuf, dB);
    } else {
        std::snprintf(textBuf, sizeof(textBuf), "Freq: %.1f Hz\nNote: %s", freq, noteBuf);
    }
    
    juce::String text(textBuf);

    auto bounds = getLocalBounds().removeFromRight(120).removeFromBottom(50);
    bounds.reduce(4, 4);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(12.0f));
    g.drawFittedText(text, bounds.reduced(4), juce::Justification::topLeft, 3);
}

void SpectrumAnalyserComponent::newOpenGLContextCreated()
{
    using namespace juce::gl;
    createShaders();

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glGenTextures(1, &dataTexture);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, AMPLITUDE_DATA_SIZE, 0, GL_RED, GL_FLOAT, amplitude_data);

    glGenTextures(1, &ribbonTexture);
    glBindTexture(GL_TEXTURE_1D, ribbonTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, AMPLITUDE_DATA_SIZE, 0, GL_RED, GL_FLOAT, ribbon_data);

    send_triggerRepaint = true;
}

void SpectrumAnalyserComponent::renderOpenGL()
{
    jassert(OpenGLHelpers::isContextActive());

    if (pause) return;

    int pres_fft_size = po2[(int)apvts_ref.getRawParameterValue("gb_fft_ord")->load()];

    using namespace ::juce::gl;
    const float renderingScale = (float)opengl_context.getRenderingScale();
    glViewport(0, 0, roundToInt(renderingScale * getWidth()), roundToInt(renderingScale * getHeight()));

    if (!shader) return;
    shader->use();

    if (shader_uniforms && shader_uniforms->amplitudeData) shader_uniforms->amplitudeData->set(0);
    if (shader_uniforms && shader_uniforms->ribbonData)    shader_uniforms->ribbonData->set(1);    

    if (shader_uniforms) {
        if (shader_uniforms->resolution)
            shader_uniforms->resolution->set((GLfloat)(renderingScale * getWidth()), (GLfloat)(renderingScale * getHeight()));
        if (shader_uniforms->numBars)
            shader_uniforms->numBars->set((GLint)apvts_ref.getRawParameterValue("sp_num_brs")->load());
        if (shader_uniforms->numBins)
            shader_uniforms->numBins->set((GLint)bins_number.load());

        if (shader_uniforms->sampleRate)
            shader_uniforms->sampleRate->set((GLfloat)SR.load());
        if (shader_uniforms->minFreq && shader_uniforms->maxFreq) {
            float min_freq = (float)apvts_ref.getRawParameterValue("sp_rng_min")->load();
            float max_freq = max((float)apvts_ref.getRawParameterValue("sp_rng_max")->load(), min_freq + 100.0f);
            shader_uniforms->minFreq->set((GLfloat)min_freq);
            shader_uniforms->maxFreq->set((GLfloat)max_freq);
        }

        // if (shader_uniforms->colourmapBias)
        //     shader_uniforms->colourmapBias->set((GLfloat)apvts_ref.getRawParameterValue("sg_cm_bias")->load());

        const float* clr_data = getAccentColoursForCode((int)(apvts_ref.getRawParameterValue("gb_clrmap")->load()));
        if (shader_uniforms->colorMap_lower)
            shader_uniforms->colorMap_lower->set(clr_data[0], clr_data[1], clr_data[2]);
        if (shader_uniforms->colorMap_higher)
            shader_uniforms->colorMap_higher->set(clr_data[3], clr_data[4], clr_data[5]);
    }
    
    // Update amplitude texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, dataTexture);
    if (newDataAvailable) {
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, AMPLITUDE_DATA_SIZE, GL_RED, GL_FLOAT, amplitude_data);
    }

    // Update ribbon texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, ribbonTexture);
    if (newDataAvailable) {
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, AMPLITUDE_DATA_SIZE, GL_RED, GL_FLOAT, ribbon_data);
        newDataAvailable = false;
    }

    // Draw full viewport quad
    GLfloat vertices[] = { 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f };
    GLuint indices[] = { 0, 1, 3, 1, 2, 3 };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SpectrumAnalyserComponent::openGLContextClosing()
{
    send_triggerRepaint = false;

    if (VBO != 0) {
        opengl_context.extensions.glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        opengl_context.extensions.glDeleteBuffers(1, &EBO);
        EBO = 0;
    }

    shader.reset();
    shader_uniforms.reset();
}

void SpectrumAnalyserComponent::newData(float* data, int num_bins)
{
    int bars = (int)apvts_ref.getRawParameterValue("sp_num_brs")->load();
    float onion_speed = apvts_ref.getRawParameterValue("sp_bar_spd")->load() * 0.001f;

    // Exponential smoothing for the onion/background layer
    float alpha_onion = 1.0f - expf(-1.0f / (onion_speed * SR));
    float rem_alpha = 1.0f - alpha_onion;

    // Update amplitude data and ribbon (onion) data
    for (int i = 0; i < num_bins; ++i) {
        amplitude_data[i] = data[i];

        // Onion maintains peak, but decays when new data is lower
        if (data[i] > ribbon_data[i]) {
            ribbon_data[i] = data[i];
        } else {
            ribbon_data[i] = data[i] * alpha_onion + ribbon_data[i] * rem_alpha;
        }
    }

    newDataAvailable = true;
    bins_number = num_bins;

    // cout << "Analyser received new data batch with " + String(num_bins) + " bins." << "\n";

}

void SpectrumAnalyserComponent::createShaders()
{
    std::unique_ptr<OpenGLShaderProgram> shaderProgramAttempt = std::make_unique<OpenGLShaderProgram>(opengl_context);

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

SpectrumAnalyserComponent::Uniforms::Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    amplitudeData.reset(createUniform(OpenGL_Context, shader_program, "amplitudeData"));
    ribbonData.reset(createUniform(OpenGL_Context, shader_program, "ribbonData"));
    colorMap_lower.reset(createUniform(OpenGL_Context, shader_program, "colorMap_lower"));
    colorMap_higher.reset(createUniform(OpenGL_Context, shader_program, "colorMap_higher"));
    numBars.reset(createUniform(OpenGL_Context, shader_program, "numBars"));
    numBins.reset(createUniform(OpenGL_Context, shader_program, "numBins"));
    sampleRate.reset(createUniform(OpenGL_Context, shader_program, "sampleRate"));
    minFreq.reset(createUniform(OpenGL_Context, shader_program, "minFreq"));
    maxFreq.reset(createUniform(OpenGL_Context, shader_program, "maxFreq"));
}

OpenGLShaderProgram::Uniform* SpectrumAnalyserComponent::Uniforms::createUniform(
    OpenGLContext& gl_context,
    OpenGLShaderProgram& shader_program,
    const char* uniform_name)
{
    if (gl_context.extensions.glGetUniformLocation(shader_program.getProgramID(), uniform_name) < 0)
        return nullptr;
    return new OpenGLShaderProgram::Uniform(shader_program, uniform_name);
}