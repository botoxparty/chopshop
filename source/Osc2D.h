#pragma once


#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>


#if JUCE_WINDOWS
    #include <gl/GL.h>
#elif JUCE_MAC
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include "RingBuffer.h"

/** This 2D Oscilloscope uses a Fragment-Shader based implementation.
 
    Future Update: modify the fragment-shader to do some visual compression so
    you can see both soft and loud movements easier. Currently, the most loud
    parts of a song to a bit too far out of the frame and the soft parts don't
    always move the wave very much.
 */

#define RING_BUFFER_READ_SIZE 256

using namespace juce;
using namespace juce::gl;

class Oscilloscope2D :  public Component,
                        public OpenGLRenderer,
                        public AsyncUpdater
{
    
public:
    
    Oscilloscope2D (RingBuffer<GLfloat>* bufferToUse)
    : readBuffer (2, RING_BUFFER_READ_SIZE)
    {
        // Sets the OpenGL version to 3.2
        openGLContext.setOpenGLVersionRequired (OpenGLContext::OpenGLVersion::openGL3_2);
        
        this->ringBuffer = bufferToUse;
        
        // Attach the OpenGL context but do not start [ see start() ]
        openGLContext.setRenderer(this);
        openGLContext.attachTo(*this);
        
        // Setup GUI Overlay Label: Status of Shaders, compiler errors, etc.
        addAndMakeVisible (statusLabel);
        statusLabel.setJustificationType (Justification::topLeft);
        statusLabel.setFont (FontOptions(14.0f));
    }
    
    ~Oscilloscope2D() override
    {
        // Turn off OpenGL
        openGLContext.setContinuousRepainting (false);
        openGLContext.detach();

        stop();
        
        // Detach ringBuffer
        ringBuffer = nullptr;
    }
    
    void handleAsyncUpdate() override
    {
        statusLabel.setText (statusText, dontSendNotification);
    }
    
    //==========================================================================
    // Oscilloscope2D Control Functions
    
    void start()
    {
        openGLContext.setContinuousRepainting (true);
    }
    
    void stop()
    {
        openGLContext.setContinuousRepainting (false);
    }
    
    
    //==========================================================================
    // OpenGL Callbacks
    
    /** Called before rendering OpenGL, after an OpenGLContext has been associated
        with this OpenGLRenderer (this component is a OpenGLRenderer).
        Sets up GL objects that are needed for rendering.
     */
    void newOpenGLContextCreated() override
    {
        // Add safety check for ring buffer
        if (ringBuffer == nullptr)
        {
            statusText = "No buffer available";
            triggerAsyncUpdate();
            return;
        }
        
        createShaders();
        
        // Setup Buffer Objects
        openGLContext.extensions.glGenBuffers (1, &VBO); // Vertex Buffer Object
        openGLContext.extensions.glGenBuffers (1, &EBO); // Element Buffer Object
    }
    
    /** Called when done rendering OpenGL, as an OpenGLContext object is closing.
        Frees any GL objects created during rendering.
     */
    void openGLContextClosing() override
    {
        shader.release();
        uniforms.release();
    }
    
    
    /** The OpenGL rendering callback.
     */
    void renderOpenGL() override
    {
        if (ringBuffer == nullptr)
            return;
        
        // Create a temporary buffer for reading samples
        AudioBuffer<GLfloat> tempBuffer(2, RING_BUFFER_READ_SIZE);
        
        // Try to read samples, but handle empty buffer case
        try 
        {
            ringBuffer->readSamples(tempBuffer, RING_BUFFER_READ_SIZE);
        }
        catch (...)
        {
            // If reading fails, just return without rendering
            return;
        }
        
        jassert (OpenGLHelpers::isContextActive());
        
        // Setup Viewport
        const float renderingScale = (float) openGLContext.getRenderingScale();
        glViewport (0, 0, roundToInt (renderingScale * getWidth()), roundToInt (renderingScale * getHeight()));
        
        // Set background Color
        OpenGLHelpers::clear (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        
        // Enable Alpha Blending
        glEnable (GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Use Shader Program that's been defined
        shader->use();
        
        // Setup the Uniforms for use in the Shader
        
        if (uniforms->resolution != nullptr)
            uniforms->resolution->set ((GLfloat) renderingScale * getWidth(), (GLfloat) renderingScale * getHeight());
        
        // Read in samples from ring buffer
        if (uniforms->audioSampleData != nullptr)
        {
            FloatVectorOperations::clear (visualizationBuffer, RING_BUFFER_READ_SIZE);
            
            // Sum channels together
            for (int i = 0; i < 2; ++i)
            {
                FloatVectorOperations::add (visualizationBuffer, tempBuffer.getReadPointer(i, 0), RING_BUFFER_READ_SIZE);
            }
            
            uniforms->audioSampleData->set (visualizationBuffer, 256);
        }
        
        // Define Vertices for a Square (the view plane)
        GLfloat vertices[] = {
            1.0f,   1.0f,  0.0f,  // Top Right
            1.0f,  -1.0f,  0.0f,  // Bottom Right
            -1.0f, -1.0f,  0.0f,  // Bottom Left
            -1.0f,  1.0f,  0.0f   // Top Left
        };
        // Define Which Vertex Indexes Make the Square
        GLuint indices[] = {  // Note that we start from 0!
            0, 1, 3,   // First Triangle
            1, 2, 3    // Second Triangle
        };
        
        // Vertex Array Object stuff for later
        //openGLContext.extensions.glGenVertexArrays(1, &VAO);
        //openGLContext.extensions.glBindVertexArray(VAO);
        
        // VBO (Vertex Buffer Object) - Bind and Write to Buffer
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, VBO);
        openGLContext.extensions.glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
                                                                    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
                                                                    // Don't we want GL_DYNAMIC_DRAW since this
                                                                    // vertex data will be changing alot??
                                                                    // test this
        
        // EBO (Element Buffer Object) - Bind and Write to Buffer
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, EBO);
        openGLContext.extensions.glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STREAM_DRAW);
                                                                    // GL_DYNAMIC_DRAW or GL_STREAM_DRAW
                                                                    // Don't we want GL_DYNAMIC_DRAW since this
                                                                    // vertex data will be changing alot??
                                                                    // test this
        
        // Setup Vertex Attributes
        openGLContext.extensions.glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        openGLContext.extensions.glEnableVertexAttribArray (0);
    
        // Draw Vertices
        //glDrawArrays (GL_TRIANGLES, 0, 6); // For just VBO's (Vertex Buffer Objects)
        glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr); // For EBO's (Element Buffer Objects) (Indices)
        
    
        
        // Reset the element buffers so child Components draw correctly
        openGLContext.extensions.glBindBuffer (GL_ARRAY_BUFFER, 0);
        openGLContext.extensions.glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, 0);
        //openGLContext.extensions.glBindVertexArray(0);
    }
    
    
    //==========================================================================
    // JUCE Callbacks
    
    void paint ([[maybe_unused]] Graphics& g) override {}
    
    void resized () override
    {
        statusLabel.setBounds (getLocalBounds().reduced (4).removeFromTop (75));
    }
    
private:
    
    //==========================================================================
    // OpenGL Functions
    
    
    /** Loads the OpenGL Shaders and sets up the whole ShaderProgram
    */
    void createShaders()
    {
        vertexShader =
        "attribute vec3 position;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(position, 1.0);\n"
        "}\n";
        
        fragmentShader =
        "uniform vec2  resolution;\n"
        "uniform float audioSampleData[256];\n"
        "\n"
        "void getAmplitudeForXPos (in float xPos, out float audioAmplitude)\n"
        "{\n"
        // Buffer size - 1
        "   float perfectSamplePosition = 255.0 * xPos / resolution.x;\n"
        "   int leftSampleIndex = int (floor (perfectSamplePosition));\n"
        "   int rightSampleIndex = int (ceil (perfectSamplePosition));\n"
        "   audioAmplitude = mix (audioSampleData[leftSampleIndex], audioSampleData[rightSampleIndex], fract (perfectSamplePosition));\n"
        "}\n"
        "\n"
        "#define THICKNESS 0.02\n"
        "void main()\n"
        "{\n"
        "    float y = gl_FragCoord.y / resolution.y;\n"
        "    float amplitude = 0.0;\n"
        "    getAmplitudeForXPos (gl_FragCoord.x, amplitude);\n"
        "\n"
        // Centers & Reduces Wave Amplitude
        "    amplitude = 0.5 - amplitude / 2.5;\n"
        "    float r = abs (THICKNESS / (amplitude-y));\n"
        "\n"
        "gl_FragColor = vec4 (r - abs (r * 0.2), r - abs (r * 0.2), r - abs (r * 0.2), 1.0);\n"
        "}\n";
        
        std::unique_ptr<OpenGLShaderProgram> shaderProgramAttempt = std::make_unique<OpenGLShaderProgram> (openGLContext);
        
        // Sets up pipeline of shaders and compiles the program
        if (shaderProgramAttempt->addVertexShader (OpenGLHelpers::translateVertexShaderToV3 (vertexShader))
            && shaderProgramAttempt->addFragmentShader (OpenGLHelpers::translateFragmentShaderToV3 (fragmentShader))
            && shaderProgramAttempt->link())
        {
            uniforms.release();
            shader = std::move (shaderProgramAttempt);
            uniforms.reset (new Uniforms (openGLContext, *shader));
            
            // statusText = "GLSL: v" + String (OpenGLShaderProgram::getLanguageVersion(), 2);
        }
        else
        {
            statusText = shaderProgramAttempt->getLastError();
        }
        
        triggerAsyncUpdate();
    }
    

    //==============================================================================
    // This class just manages the uniform values that the fragment shader uses.
    struct Uniforms
    {
        Uniforms (OpenGLContext& glContext, OpenGLShaderProgram& shaderProgram)
        {
            //projectionMatrix = createUniform (glContext, shaderProgram, "projectionMatrix");
            //viewMatrix       = createUniform (glContext, shaderProgram, "viewMatrix");
            
            resolution.reset (createUniform (glContext, shaderProgram, "resolution"));
            audioSampleData.reset (createUniform (glContext, shaderProgram, "audioSampleData"));
            
        }
        
        //ScopedPointer<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix;
        std::unique_ptr<OpenGLShaderProgram::Uniform> resolution, audioSampleData;
        
    private:
        static OpenGLShaderProgram::Uniform* createUniform (OpenGLContext& glContext,
                                                            OpenGLShaderProgram& shaderProgram,
                                                            const char* uniformName)
        {
            if (glContext.extensions.glGetUniformLocation (shaderProgram.getProgramID(), uniformName) < 0)
                return nullptr;
            
            return new OpenGLShaderProgram::Uniform (shaderProgram, uniformName);
        }
    };
    
    
    // OpenGL Variables
    OpenGLContext openGLContext;
    GLuint VBO, VAO, EBO;
    
    std::unique_ptr<OpenGLShaderProgram> shader;
    std::unique_ptr<Uniforms> uniforms;
    
    const char* vertexShader;
    const char* fragmentShader;

    
    // Audio Buffer
    RingBuffer<GLfloat> * ringBuffer;
    AudioBuffer<GLfloat> readBuffer;    // Stores data read from ring buffer
    GLfloat visualizationBuffer [RING_BUFFER_READ_SIZE];    // Single channel to visualize
    
    
    
    // Overlay GUI
    String statusText;
    Label statusLabel;
    
    
    /** DEV NOTE
        If I wanted to optionally have an interchangeable shader system,
        this would be fairly easy to add. Chack JUCE Demo -> OpenGLDemo.cpp for
        an implementation example of this. For now, we'll just allow these
        shader files to be static instead of interchangeable and dynamic.
        String newVertexShader, newFragmentShader;
     */
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oscilloscope2D)
};