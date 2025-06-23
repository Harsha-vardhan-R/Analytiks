#include "Analyser.h"

SpectrumAnalyserComponent::SpectrumAnalyserComponent(AudioProcessorValueTreeState& apvts_reference)
    : apvts_ref(apvts_reference)
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

SpectrumAnalyserComponent::~SpectrumAnalyserComponent()
{
}

void SpectrumAnalyserComponent::newDataPoint(float* data_pointer, int num_values)
{
    /*for (int index = 0; index < num_values; ++index)
        amplitude_data[index] = data_pointer[index];*/
    
}

void SpectrumAnalyserComponent::clearData()
{
    for (int index = 0; index < AMPLITUDE_DATA_SIZE; ++index)
        amplitude_data[index] = 0.0;
}

void SpectrumAnalyserComponent::newOpenGLContextCreated()
{
    using namespace juce::gl;
    createShaders();

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    //glGenTextures(1, &dataTexture);
    //glBindTexture(GL_TEXTURE_1D, dataTexture);
    //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexImage1D(
    //    GL_TEXTURE_1D,
    //    0,
    //    GL_RG32F,
    //    RING_BUFFER_TEXEL_SIZE, // each texel is 2 floats.
    //    0,
    //    GL_RG,
    //    GL_FLOAT,
    //    ring_buffer);

    //ring_buf_read_index = 0;

    send_triggerRepaint = true;
    opengl_context.triggerRepaint();
}

void SpectrumAnalyserComponent::renderOpenGL()
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

    /*jassert(shader_uniforms->readIndex != nullptr);
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
    }*/

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

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_1D, dataTexture);
    //if (dirty.load())
    //{
    //    glTexSubImage1D(
    //        GL_TEXTURE_1D,
    //        0,
    //        0,
    //        RING_BUFFER_TEXEL_SIZE, // each texel is 2 floats.
    //        GL_RG,
    //        GL_FLOAT,
    //        ring_buffer);
    //    dirty.store(false);
    //}

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

void SpectrumAnalyserComponent::openGLContextClosing()
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

void SpectrumAnalyserComponent::createShaders()
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

SpectrumAnalyserComponent::Uniforms::
Uniforms(OpenGLContext& OpenGL_Context, OpenGLShaderProgram& shader_program)
{
    resolution.reset(createUniform(OpenGL_Context, shader_program, "resolution"));
    
}

OpenGLShaderProgram::Uniform* SpectrumAnalyserComponent::Uniforms::createUniform(
    OpenGLContext& gl_context,
    OpenGLShaderProgram& shader_program,
    const char* uniform_name)
{
    if (
        gl_context.extensions.glGetUniformLocation(shader_program.getProgramID(), uniform_name) < 0)
        return nullptr;

    return new OpenGLShaderProgram::Uniform(shader_program, uniform_name);
}