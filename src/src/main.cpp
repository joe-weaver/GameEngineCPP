// Required graphics library imports
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// imgui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Other imports
#include <iostream>
#include <functional>
#include <shader.h>
#include <texture.h>
#include <mesh.h>
#include <renderer.h>
#include <sprite.h>
#include <camera.h>

// TODO: Implement
// #define DEBUG_DRAW

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

Camera *globalCamera = nullptr;
struct mouse_state_t
{
    bool initialized = false;
    vec2 lastPos;
    vec2 currPos;
    vec2 delta;
    double leftClickStart = 0.0;
    double rightClickStart = 0.0;
    double middleClickStart = 0.0;
} mouseState;

void scrollCallback(GLFWwindow * window, double xoffset, double yoffset);
void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow * window, double xpos, double ypos);

std::function<void(vec2)> onLeftClick;

int main(int, char**)
{
    GLFWwindow* window;

    // Set up error callback and init
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return -1;
        
    // Select GL version + let the backend select a GLSL version
    const char* glsl_version = nullptr;
#if defined(__APPLE__)
    // GL 3.2 + generally GLSL 150
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + generally GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif


    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
    vec2 windowSize = vec2(1280 * main_scale, 800 * main_scale);
    window = glfwCreateWindow(1280 * main_scale, 800 * main_scale, "Hello World", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cout << std::unitbuf
                  << "[ERROR] " << __FILE__ << ':' << __LINE__ << ' ' << __PRETTY_FUNCTION__
                  << "\n[ERROR] " << "Failed to initialize GLAD!"
                  << std::nounitbuf << std::endl;

        std::abort();
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark(); //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // State variables
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Game engine things
    // Setup callbacks
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);    
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Initialize internal game engine values
    const_cast<Shader *>(Shader::DEFAULT_SHADER)->fromFile(
        RESOURCE_DIR "/default.vert.glsl",
        RESOURCE_DIR "/default.frag.glsl");
    
    Mesh::initPrimitives();

    // Load our default texture
    Texture texture("test.png");
    std::vector<Sprite> sprites;
    for(int i = 0; i < 10; i++)
    {
        sprites.push_back(Sprite(windowSize * (i / 10.f), vec2(60, 60), &texture, Shader::DEFAULT_SHADER));
    }

    // Create a camera
    Camera camera(windowSize/2.f, windowSize);
    globalCamera = &camera;

    onLeftClick = [&sprites](vec2 position)
    {
        std::cout << "checking click @(" << position.x << "," << position.y << ")" << std::endl;
        // Find the sprite we interact with
        for(auto& spr : sprites)
        {
            if(spr.containsPoint(position))
            {
                std::cout << "one contains!" << std::endl;
                spr.disabled = true;
                break;
            }
        }
    };

    double deltaT = 0.0;
    double currentFrame = 0.0;
    double lastFrame = 0.0;
    bool firstFrame = true;

    // Start the game loop
    while (!glfwWindowShouldClose(window))
    {
        // Update deltaT
        currentFrame = glfwGetTime();
        deltaT = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        if(firstFrame)
        {
            firstFrame = false;
            continue;
        }

        glfwPollEvents();

        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // For now, all objects use the default shader
        // TODO: Add support for switching shaders
        Shader::DEFAULT_SHADER->use();

        // These don't change, so set them out here
        Shader::DEFAULT_SHADER->setMat4("uView", camera.getView());
        Shader::DEFAULT_SHADER->setMat4("uProjection", camera.getProjection());
        
        // Render game content
        for (auto& sprite : sprites) {
            sprite.draw();
        }

#ifdef DEBUG_DRAW
        // Debugging render pass
        for (auto& sprite : sprites) {
            sprite.draw_DEBUG();
        }
#endif
        
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error: " << err << std::endl;
        }

        /* START IMGUI */
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        // Handle high-DPI displays
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // Render imgui
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        /* END IMGUI */

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// Callback code
// This overwites the default imgui hook into scrolls, so add that in too
void scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
    // Defer to imgui first
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

    // imgui is treated like an overlay, so if it didn't absorb the scroll, use it ourselves
    if(!ImGui::GetIO().WantCaptureMouse)
    {
        if(globalCamera != nullptr)
        {
            if(yoffset > 0)
            {
                globalCamera->zoom(1.2, mouseState.currPos);
            }
            else if(yoffset < 0)
            {
                globalCamera->zoom(1 / 1.2, mouseState.currPos);
            }
        }
    }
}

void mouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{
    // Defer to imgui first
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    double currentFrame = glfwGetTime();

    // imgui is treated like an overlay, so if it didn't absorb the scroll, use it ourselves
    if(!ImGui::GetIO().WantCaptureMouse)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
            mouseState.leftClickStart = currentFrame;
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            onLeftClick(globalCamera->screenToWorld(mouseState.currPos));
            mouseState.leftClickStart = 0.0;
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
            mouseState.rightClickStart = currentFrame;
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
            mouseState.rightClickStart = 0.0;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
            mouseState.middleClickStart = currentFrame;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
            mouseState.middleClickStart = 0.0;
    }
}

void cursorPosCallback(GLFWwindow * window, double xpos, double ypos)
{
    // Defer to imgui first
    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

    // imgui is treated like an overlay, so if it didn't absorb the scroll, use it ourselves
    if(!ImGui::GetIO().WantCaptureMouse)
    {
        if(!mouseState.initialized)
        {
            // Initialize mouse state
            mouseState.currPos = vec2(xpos, ypos);
            mouseState.initialized = true;
        }
        else
        {
            mouseState.lastPos = mouseState.currPos;
            mouseState.currPos = vec2(xpos, ypos);
            mouseState.delta = mouseState.currPos - mouseState.lastPos;

            if(mouseState.middleClickStart > 0)
            {
                // Middle mouse dragging - move camera
                globalCamera->move(-mouseState.delta / globalCamera->getZoomFactor());
            }
        }
    }
}