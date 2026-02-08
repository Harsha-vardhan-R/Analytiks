#include "Correlation.h"

PhaseCorrelationAnalyserComponent::PhaseCorrelationAnalyserComponent(AudioProcessorValueTreeState& apvts_reference)
    :   apvts_ref(apvts_reference),
        correl_amnt_comp(String("-1"), String("+1")),
        balance_amnt_comp(String("L"), String("R"))
{
    addAndMakeVisible(opengl_comp);
    addAndMakeVisible(volume_meter_comp);
    addAndMakeVisible(correl_amnt_comp);
    addAndMakeVisible(balance_amnt_comp);

    startTimerHz(REFRESH_RATE_HZ);

    apvts_ref.getParameter("gb_clrmap")->addListener(&opengl_comp);
}

PhaseCorrelationAnalyserComponent::~PhaseCorrelationAnalyserComponent()
{
    apvts_ref.getParameter("gb_clrmap")->removeListener(&opengl_comp);
}

void PhaseCorrelationAnalyserComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    opengl_comp.repaint();
    dirty = true;
}

void PhaseCorrelationAnalyserComponent::paint(Graphics& g)
{
    float hue = apvts_ref.getRawParameterValue("ui_acc_hue")->load();
    Colour accentColour = Colour::fromHSV(hue, 0.75, 0.35, 1.0);

    correl_amnt_comp.accent_colour = accentColour;
    volume_meter_comp.accent_colour = accentColour;
    balance_amnt_comp.accent_colour = accentColour;
}

void PhaseCorrelationAnalyserComponent::resized()
{

    float vol_bounds_width = std::clamp<float>(getWidth() * 0.05, 11, 37);
    auto bounds = getLocalBounds();

    // TODO : this would be at the top if we used a rotated orientation.
    auto volume_bounds = bounds.removeFromLeft(vol_bounds_width);
    auto corell_amount_bounds = bounds.removeFromRight(vol_bounds_width);
    auto balance_amount_bounds = bounds.removeFromBottom(vol_bounds_width);

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

    bounds.removeFromTop(vol_bounds_width);

    opengl_comp.setBounds(bounds.reduced(5));
    correl_amnt_comp.setBounds(corell_amount_bounds);
    balance_amnt_comp.setBounds(balance_amount_bounds);
    volume_meter_comp.setBounds(volume_bounds);

}

void PhaseCorrelationAnalyserComponent::timerCallback()
{
    if (dirty) {
        opengl_comp.repaint();

        volume_meter_comp.repaint();
        correl_amnt_comp.repaint();
        balance_amnt_comp.repaint();

        dirty = false;
    }

}

void PhaseCorrelationAnalyserComponent::prepareToPlay(float SR, float block_size)
{
    sample_rate = SR;

    sample_counter = 0;

    while (SquareQueLL.size()) SquareQueLL.pop();
    while (SquareQueRR.size()) SquareQueRR.pop();
    while (SquareQueLR.size()) SquareQueLR.pop();
    
    sumLL = 0.0f;
    sumRR = 0.0f;
    sumLR = 0.0f;

    zeroOutMeters();
}

void PhaseCorrelationAnalyserComponent::zeroOutMeters()
{
    opengl_comp.newDataPoint(0.5f, 0.5f);
    correl_amnt_comp.newPoint(0.5f);
    volume_meter_comp.newPoint(0.0f, 0.0f);
    balance_amnt_comp.newPoint(0.5);
    volume_meter_comp.zeroOut();
}

void PhaseCorrelationAnalyserComponent::processBlock(AudioBuffer<float>& buffer)
{
    float new_rms_time = apvts_ref.getRawParameterValue("v_rms_time")->load();
    int new_window_length = (int)(sample_rate * new_rms_time * 0.001f);
    update_window_samples    = (int)(sample_rate / (float)TARGET_TRIGGER_HZ);

    if (new_window_length != window_length_samples && new_window_length > 0)
    {
        window_length_samples = new_window_length;
        sample_counter = 0;
        while (!SquareQueLL.empty()) SquareQueLL.pop();
        while (!SquareQueRR.empty()) SquareQueRR.pop();
        while (!SquareQueLR.empty()) SquareQueLR.pop();
        sumLL = 0.0f;
        sumRR = 0.0f;
        sumLR = 0.0f;
    }

    if (window_length_samples <= 0)
        return;

    float* left_channel  = buffer.getWritePointer(0);
    float* right_channel = buffer.getWritePointer(1);
    int block_size = buffer.getNumSamples();

    static int recalc_counter = 0;
    const int RECALC_INTERVAL = 32768;

    for (int i = 0; i < block_size; ++i)
    {
        float left_sample  = std::isnan(left_channel[i])  ? 0.0f : std::clamp(left_channel[i],  -1.0f, 1.0f);
        float right_sample = std::isnan(right_channel[i]) ? 0.0f : std::clamp(right_channel[i], -1.0f, 1.0f);
        
        // Send to opengl_comp every 3rd sample
            
        float xx = left_sample * 0.5f + 0.5f;
        float yy = right_sample * 0.5f + 0.5f;
        xx = jmap<float>(xx, 0.0f, 1.0f, 0.15f, 0.85f);
        yy = jmap<float>(yy, 0.0f, 1.0f, 0.15f, 0.85f);
        
        const float centerX = 0.5f;
        const float centerY = 0.5f;
        float translatedX = xx - centerX;
        float translatedY = yy - centerY;
        const float cosAngle = 0.707107f;
        const float sinAngle = 0.707107f;
        float rotatedX = translatedX * cosAngle - translatedY * sinAngle;
        float rotatedY = translatedX * sinAngle + translatedY * cosAngle;
        float finalX = std::clamp(rotatedX + centerX, 0.0f, 1.0f);
        float finalY = std::clamp(rotatedY + centerY, 0.0f, 1.0f);
        
        opengl_comp.newDataPoint(finalX, finalY);
        
        float LL = left_sample  * left_sample;
        float RR = right_sample * right_sample;
        float LR = left_sample  * right_sample;

        sumLL += LL;  sumRR += RR;  sumLR += LR;
        
        SquareQueLL.push(LL);
        SquareQueRR.push(RR);
        SquareQueLR.push(LR);

        while ((int)SquareQueLL.size() > window_length_samples)
        {
            sumLL -= SquareQueLL.front();  
            SquareQueLL.pop();

            sumRR -= SquareQueRR.front();  
            SquareQueRR.pop();
            
            sumLR -= SquareQueLR.front();  
            SquareQueLR.pop();
        }

        if (++recalc_counter >= RECALC_INTERVAL)
        {
            recalc_counter = 0;
            recalculateSums();
        }

        if (++sample_counter >= update_window_samples)
        {
            sample_counter = 0;

            float denom = std::sqrtf(sumLL * sumRR);
            if (denom < 1e-6f) denom = 1e-6f;

            float y_comp = sumLR / denom;

            y_comp = y_comp * 0.5f + 0.5f;
            y_comp = 1.0 - y_comp;
            y_comp = std::clamp(y_comp, 0.0f, 1.0f);

            float rmsL = std::sqrtf(sumLL / (float)window_length_samples);
            float rmsR = std::sqrtf(sumRR / (float)window_length_samples);

            const float noiseGate = 5e-4f;
            if (rmsL < noiseGate) rmsL = 0.0f;
            if (rmsR < noiseGate) rmsR = 0.0f;

            float x_comp = 0.0f;
            if (rmsL + rmsR > 1e-8f)
                x_comp = (rmsR - rmsL) / (rmsL + rmsR);

            x_comp = x_comp * 0.5f + 0.5f;
            x_comp = std::clamp(x_comp, 0.0f, 1.0f);

            correl_amnt_comp.newPoint(y_comp);
            volume_meter_comp.newPoint(rmsL, rmsR);
            balance_amnt_comp.newPoint(x_comp);

            dirty = true;
        }
    }
}

void PhaseCorrelationAnalyserComponent::recalculateSums()
{
    sumLL = 0.0f;
    sumRR = 0.0f;
    sumLR = 0.0f;
    
    std::queue<float> tempLL = SquareQueLL;
    std::queue<float> tempRR = SquareQueRR;
    std::queue<float> tempLR = SquareQueLR;
    
    while (!tempLL.empty())
    {
        sumLL += tempLL.front();
        sumRR += tempRR.front();
        sumLR += tempLR.front();
        tempLL.pop();
        tempRR.pop();
        tempLR.pop();
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

    opengl_context.setContinuousRepainting(false);

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
parameterValueChanged(int index, float newValue)
{
    const float* clrs = getAccentColoursForCode((int)(newValue * 10));
    for (int i = 0; i < 6; ++i) colours[i] = clrs[i];
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
    //opengl_context.triggerRepaint();
}

void PhaseCorrelationAnalyserComponent::CorrelationOpenGLComponent::renderOpenGL()
{
    using namespace juce::gl;
    
    if (!shader) return;

    const float renderingScale = (float)opengl_context.getRenderingScale();
    glViewport(0, 0, roundToInt(renderingScale * getWidth()), roundToInt(renderingScale * getHeight()));

    OpenGLHelpers::clear(Colours::black);
    
    shader->use();
    
    int read_idx = ring_buf_read_index.load();
    
    // Build vertex buffer with positions and colors
    std::vector<GLfloat> vertices;
    vertices.reserve((RING_BUFFER_TEXEL_SIZE + 12) * 5); // Lissajous + grid lines
    
    // Grid lines (diamond overlay)
    const float gridColor = 0.3f;
    GLfloat gridLines[] = {
        // X-axis lines
        0.0f, 0.5f, gridColor, gridColor, gridColor,
        0.5f, 0.0f, gridColor, gridColor, gridColor,
        0.0f, 0.5f, gridColor, gridColor, gridColor,
        0.5f, 1.0f, gridColor, gridColor, gridColor,
        1.0f, 0.5f, gridColor, gridColor, gridColor,
        0.5f, 0.0f, gridColor, gridColor, gridColor,
        1.0f, 0.5f, gridColor, gridColor, gridColor,
        0.5f, 1.0f, gridColor, gridColor, gridColor,
        // Diagonal lines
        0.25f, 0.75f, gridColor, gridColor, gridColor,
        0.75f, 0.25f, gridColor, gridColor, gridColor,
        0.25f, 0.25f, gridColor, gridColor, gridColor,
        0.75f, 0.75f, gridColor, gridColor, gridColor
    };
    
    // Draw grid
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridLines), gridLines, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glLineWidth(1.5f * renderingScale);
    glDrawArrays(GL_LINES, 0, 12);
    
    // Lissajous curve
    for (int i = 0; i < RING_BUFFER_TEXEL_SIZE; ++i)
    {
        int idx = (read_idx + i) % RING_BUFFER_TEXEL_SIZE;
        float x = ring_buffer[2 * idx];
        float y = ring_buffer[2 * idx + 1];
        
        float t = (float)i / (float)(RING_BUFFER_TEXEL_SIZE - 1);
        float r = colours[0] * (1.0f - t) + colours[3] * t;
        float g = colours[1] * (1.0f - t) + colours[4] * t;
        float b = colours[2] * (1.0f - t) + colours[5] * t;
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(r * t * t);
        vertices.push_back(g * t * t);
        vertices.push_back(b * t * t);
    }
    
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glLineWidth(2.0f * renderingScale);
    glDrawArrays(GL_LINE_STRIP, 0, RING_BUFFER_TEXEL_SIZE);
    
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
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

PhaseCorrelationAnalyserComponent::ValueMeterComponent::ValueMeterComponent
(String low_value_str, String high_value_str)
    :   low_val(low_value_str),
        high_val(high_value_str)
{
}

void PhaseCorrelationAnalyserComponent::ValueMeterComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::black);
    auto bounds = getLocalBounds();

    if (bounds.getWidth() > bounds.getHeight())
    {
        int padding = std::max(1.0, 0.1 * getHeight());

        auto top = bounds.removeFromTop(padding);

        g.setColour(accent_colour);
        g.fillRect(top);

        g.setColour(juce::Colours::white);

        int bar_width = std::min<int>(5, 0.02 * (float)bounds.getWidth());

        g.fillRect(
            bounds.getX() + (int)((float)(bounds.getWidth() - (bar_width/2)) * pres_value.load()),
            bounds.getY(),
            bar_width,
            bounds.getHeight()
        );
    
        g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));

        g.fillRect(
            bounds.getX() + (bounds.getWidth() / 2),
            bounds.getY(),
            2,
            bounds.getHeight()
        );

        g.setFont(std::min(bounds.getWidth(), bounds.getHeight()) * 0.6f);

        g.setColour(Colours::blue);

        g.drawText(
            low_val,
            bounds.removeFromLeft(bounds.getHeight()),
            Justification::centred
        );

        g.setColour(Colours::red);

        g.drawText(
            high_val,
            bounds.removeFromRight(bounds.getHeight()),
            Justification::centred
        );

    }
    else
    {
        int padding = std::max(1.0, 0.1 * getWidth());

        auto left = bounds.removeFromLeft(padding);

        g.setColour(accent_colour);

        g.fillRect(left);

        g.setColour(juce::Colours::white);

        int bar_width = std::min<int>(5, 0.02 * (float)bounds.getHeight());

        g.fillRect(
            bounds.getX(),
            bounds.getY() + (int)((float)(bounds.getHeight() - bar_width) * pres_value.load()),
            bounds.getWidth(),
            bar_width
        );


        g.setColour(juce::Colours::darkgrey);

        g.fillRect(
            bounds.getX(),
            bounds.getY() + (bounds.getHeight() / 2),
            bounds.getWidth(),
            2
        );

        g.setColour(Colours::red);

        g.setFont(std::min(bounds.getWidth(), bounds.getHeight()) * 0.6f);
        g.drawText(
            low_val,
            bounds.removeFromBottom(bounds.getWidth()),
            Justification::centred
        );

        g.setColour(Colours::blue);

        g.drawText(
            high_val,
            bounds.removeFromTop(bounds.getWidth()),
            Justification::centred
        );

    }

}

void PhaseCorrelationAnalyserComponent::ValueMeterComponent::resized()
{
}



void PhaseCorrelationAnalyserComponent::ValueMeterComponent::newPoint(float y_val)
{
    // one pole smoothing filter.
    float prev_value = pres_value.load();
    float new_value = prev_value + (y_val - prev_value)*alpha;

    pres_value.store(new_value);
}

PhaseCorrelationAnalyserComponent::VolumeMeterComponent::VolumeMeterComponent()
{
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::zeroOut()
{
    l_rms_vol.store(0.0f);
    r_rms_vol.store(0.0f);

    l_rms_vol_smoothed = 0.0f;
    r_rms_vol_smoothed = 0.0f;
}

static float gain_to_db(float gain)
{
    return 20.0 * log10f(gain);
}

static float custom_remap(float vol_dB)
{
    vol_dB = std::clamp<float>(vol_dB, -80.0, 0.0);
    return jmap<float>(vol_dB, -80.0, 0.0, 0.0, 1.0);
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::paint(Graphics& g)
{
    g.fillAll(juce::Colours::black);

    int padding = std::max(1.0, 0.1 * getWidth());

    auto bounds = getLocalBounds();

    auto right = bounds.removeFromRight(padding);

    g.setColour(accent_colour);
    g.fillRect(right);

    bounds.removeFromLeft(1);
    bounds.removeFromRight(1);

    auto center_line_bounds = bounds;
    center_line_bounds.removeFromTop(bounds.getHeight() / 2);
    center_line_bounds.removeFromBottom(bounds.getHeight() / 2 - 1);

    float l_vol_frac = custom_remap(gain_to_db(
        std::clamp<float>(l_rms_vol.load(), 0.0, 1.0)
    ));
    float r_vol_frac = custom_remap(gain_to_db(
        std::clamp<float>(r_rms_vol.load(), 0.0, 1.0)
    ));
    float l_vol_frac_smooth = custom_remap(gain_to_db(
        std::clamp<float>(l_rms_vol_smoothed, 0.0, 1.0)
    ));
    float r_vol_frac_smooth = custom_remap(gain_to_db(
        std::clamp<float>(r_rms_vol_smoothed, 0.0, 1.0)
    ));

    auto left_bar_bounds = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto right_bar_bounds = bounds;

    auto left_background_bar_bound = left_bar_bounds;
    auto right_background_bar_bound = right_bar_bounds;

    if (left_bar_bounds.getWidth() > 1 && right_bar_bounds.getWidth() > 1)
    {
        left_bar_bounds.removeFromRight(1);
        left_background_bar_bound.removeFromRight(1);
    }

    int half_remove_left  = (1.0 - l_vol_frac) * bounds.getHeight() * 0.5;
    int half_remove_right = (1.0 - r_vol_frac) * bounds.getHeight() * 0.5;

    int half_remove_left_smooth  = (1.0 - l_vol_frac_smooth) * bounds.getHeight() * 0.5;
    int half_remove_right_smooth = (1.0 - r_vol_frac_smooth) * bounds.getHeight() * 0.5;

    left_bar_bounds.removeFromTop(half_remove_left);
    left_bar_bounds.removeFromBottom(half_remove_left);

    right_bar_bounds.removeFromTop(half_remove_right);
    right_bar_bounds.removeFromBottom(half_remove_right);

    left_background_bar_bound.removeFromTop(half_remove_left_smooth);
    left_background_bar_bound.removeFromBottom(half_remove_left_smooth);

    right_background_bar_bound.removeFromTop(half_remove_right_smooth);
    right_background_bar_bound.removeFromBottom(half_remove_right_smooth);

    g.setColour(juce::Colours::darkgrey);
    g.fillRect(left_background_bar_bound);
    g.fillRect(right_background_bar_bound);

    g.setColour(juce::Colours::white);
    g.fillRect(left_bar_bounds);
    g.fillRect(right_bar_bounds);

    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    g.fillRect(center_line_bounds);
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::resized()
{
}

void PhaseCorrelationAnalyserComponent::VolumeMeterComponent::newPoint(float l_vol, float r_vol)
{
    if (std::isnan(l_vol)) l_vol = 0.0f;
    if (std::isnan(r_vol)) r_vol = 0.0f;

    float prev_l_vol = l_rms_vol_smoothed;
    float prev_r_vol = r_rms_vol_smoothed;

    float new_l, new_r;

    if (l_vol > prev_l_vol) l_rms_vol_smoothed = l_vol;
    else  l_rms_vol_smoothed = prev_l_vol + (l_vol - prev_l_vol) * alpha;

    if (r_vol > prev_r_vol) r_rms_vol_smoothed = r_vol;
    else r_rms_vol_smoothed = prev_r_vol + (r_vol - prev_r_vol) * alpha;

    l_rms_vol.store(l_vol);
    r_rms_vol.store(r_vol);
}
