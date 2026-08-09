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
#include "shader.h"
#include "texture.h"
#include "mesh.h"
#include "renderer.h"
#include "sprite.h"
#include "camera.h"
#include "random.h"

#include "wfc.h"

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

#define RUN_TESTS
#ifdef RUN_TESTS
void run_tests();
#else
void run_tests() {}
#endif

struct InputFile_Image
{
    const char * filename;
    BitmapEdgeMode edgeMode;
    bool generateTransformations;
};

int main(int, char**)
{
    run_tests();

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
    window = glfwCreateWindow(1280 * main_scale, 800 * main_scale, "デモプロジェクト", NULL, NULL);
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

    // Add a font
    io.Fonts->AddFontFromFileTTF(RESOURCE_DIR "/NotoSansJP-Regular.ttf");

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
    Bitmap *bmp = new Bitmap("test.png");
    UpdatingTexture texture(bmp);
    Random r;

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
        // Find the sprite we interact with
        for(auto& spr : sprites)
        {
            if(spr.containsPoint(position))
            {
                // std::cout << "one contains!" << std::endl;
                spr.disabled = true;
                break;
            }
        }
    };

    // Actual project code
    
    InputFile_Image flower = {.filename = "Flower.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = false};
    InputFile_Image trees = {.filename = "Trees.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = false};
    InputFile_Image grassy = {.filename = "Grassy.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = false};
    InputFile_Image brick = {.filename = "Brick.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = false};
    InputFile_Image maze = {.filename = "Maze.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = true};
    InputFile_Image QR = {.filename = "QR.png", .edgeMode = BitmapEdgeMode::None, .generateTransformations = false};
    InputFile_Image cave = {.filename = "Cave.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = false};
    InputFile_Image spiral = {.filename = "Spiral.png", .edgeMode = BitmapEdgeMode::None, .generateTransformations = true};
    InputFile_Image regions = {.filename = "Regions.png", .edgeMode = BitmapEdgeMode::None, .generateTransformations = true};
    InputFile_Image glyphs = {.filename = "Glyphs.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = false};
    InputFile_Image gradient = {.filename = "Gradient.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = true};
    InputFile_Image coastal = {.filename = "Coastal.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = true};


    InputFile_Image * fileToUse = &coastal;

    int outputSize = 32;
    bool wrapOutput = false;
    int seed = 123457;
    
    Bitmap * bmp_basis_1_1 = new Bitmap(fileToUse->filename, fileToUse->edgeMode);
    Texture tex_basis_1_1(bmp_basis_1_1);
    WFC_Image wfc_2(bmp_basis_1_1, outputSize, fileToUse->generateTransformations, seed, wrapOutput);

    int N = 10;
    WFC_TriColor wfc_1(N);
    const WFC_TriColor::PixelState * state = wfc_1.getOutput();
    Bitmap * stateDisplay = new Bitmap(N, N);

    auto updateDisplay = [N, state, stateDisplay]()
    {
        for(int y = 0; y < N; y++)
        {
            for(int x = 0; x < N; x++)
            {
                WFC_TriColor::PixelState ps = state[y*N + x];
                if(ps.collapsed)
                    stateDisplay->setPixel(x, y, ColorRGBA(ps.red ? 255 : 0, ps.green ? 255 : 0, ps.blue ? 255 : 0, 255));
                else
                    stateDisplay->setPixel(x, y, ColorRGBA(255, 255, 255, 255));
            }
        }
    };
    
    updateDisplay();

    UpdatingTexture outputTex(wfc_2.getOutputImage());
    Sprite outputSprite(windowSize / 2.f, vec2(200, 200), &outputTex, Shader::DEFAULT_SHADER);

    // Init loop variables
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
        
        /*
        // Corrupt random pixels in our image
        int x = r.range(0, bmp->getWidth());
        int y = r.range(0, bmp->getHeight());
        ColorRGBA color = r.colorRGBA();

        bmp->setPixel(x, y, color);
        texture.updateFromBitmap();

        // Render game content
        for (auto& sprite : sprites) {
            sprite.draw();
        }
        */

        static int SKIP_FRAMES = 0;
        static int NUM_STEPS = 1;
        static int framesUntilStep = SKIP_FRAMES;

        if(framesUntilStep-- == 0)
        {
            framesUntilStep = SKIP_FRAMES;
            for(int i = 0; i < NUM_STEPS; i++)
            {
                // if(wfc_1.step())
                // {
                //     updateDisplay();
                //     outputTex.updateFromBitmap();
                // }

                // HERE!

                if(wfc_2.step())
                {
                    outputTex.updateFromBitmap();
                }
            }
        }

        static bool pressed = false;
        static bool justPressed = false;

        justPressed = false;
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !pressed)
        {
            justPressed = true;
            pressed = true;
        }
        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
            pressed = false;

        if(justPressed)
        {
            if(wfc_2.step())
            {
                outputTex.updateFromBitmap();
            }
        }
        
        outputSprite.draw();
        
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
            
            ImGui::Text("FrameBufferScale: (%f, %f)", io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
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

        // Display the current block
        {
            ImGui::Begin("テクスチャテスト", &show_another_window);
            
            ImGuiIO& io = ImGui::GetIO();
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Layout canvas
            ImGui::Text("現在のベーシス");
            ImGui::InvisibleButton("##Canvas", ImVec2(256, 256));
            ImVec2 canvas_min = ImGui::GetItemRectMin();
            ImVec2 canvas_max = ImGui::GetItemRectMax();

            draw_list->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest);
            draw_list->AddImage(tex_basis_1_1.as_imgui(), canvas_min, canvas_max);
            draw_list->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear);

            ImGui::End();
        }

        {
            static double avgProp = 0;
            static double avgColl = 0;
            static int GET_AVG_FRAMES = 60;
            static int framesUntilGetAvg = GET_AVG_FRAMES;

            if(framesUntilGetAvg-- <= 0)
            {
                framesUntilGetAvg = GET_AVG_FRAMES;
                avgProp = wfc_2.avgPropPerStep;
                avgColl = wfc_2.avgCollapsedPerStep;
            }

            ImGui::Begin("ディーバッグ");
            ImGui::Text("Avg. propagations per step: %.0f", avgProp);
            ImGui::Text("Avg. collapses per step: %.0f", avgColl);
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

#ifdef RUN_TESTS
class TestCase
{
private:
    WFC_Image::Kernel3x3 a, b;   // Kernels
    int ox, oy; // Kernel offset
    bool expectedOutput;

public:
    TestCase(WFC_Image::Kernel3x3 a, WFC_Image::Kernel3x3 b, int ox, int oy, bool expectedOutput) : a(a), b(b), ox(ox), oy(oy), expectedOutput(expectedOutput) {}

    bool run()
    {
        return this->a.match(&this->b, this->ox, this->oy) == this->expectedOutput;
    }
};

void run_tests()
{
    // Load our test cases file
    Bitmap * exampleKernels = new Bitmap("testcases.png");
    int numRealTiles = 20;
    std::vector<WFC_Image::Kernel3x3> k = std::vector<WFC_Image::Kernel3x3>();

    // Create kernels for each cell
    for(int y = 0; y < 10; y++)
    {
        for(int x = 0; x < 10; x++)
        {
            k.push_back(WFC_Image::Kernel3x3(exampleKernels, 3*x + 1, 3*y + 1));
        }
    }

    std::vector<TestCase> testCases = std::vector<TestCase>();
    // Simple full black vs full white check. All should fail
    for(int oy = -1; oy <= 1; oy++)
        for(int ox = -1; ox <= 1; ox++)
            if(!(ox == 0 && oy == 0))
                testCases.push_back(TestCase(k[0], k[1], -1, -1, false));

    // Full black vs. part black
    {
        testCases.push_back(TestCase(k[1], k[2], 1, -1, true));
        testCases.push_back(TestCase(k[1], k[2], 1, 0, true));
        testCases.push_back(TestCase(k[1], k[2], 1, 1, true));

        // The inverse should fail
        testCases.push_back(TestCase(k[2], k[1], 1, -1, false));
        testCases.push_back(TestCase(k[2], k[1], 1, 0, false));
        testCases.push_back(TestCase(k[2], k[1], 1, 1, false));
    }

    // Verify flipping works
    testCases.push_back(TestCase(k[2], k[3].reflectX(), 0, 0, true));
    testCases.push_back(TestCase(k[3], k[2].reflectX(), 0, 0, true));

    // Full black vs. part black, but tile 2 is flipped
    {
        testCases.push_back(TestCase(k[3], k[1], 1, -1, true));
        testCases.push_back(TestCase(k[3], k[1], 1, 0, true));
        testCases.push_back(TestCase(k[3], k[1], 1, 1, true));

        // The inverse should fail
        testCases.push_back(TestCase(k[1], k[3], 1, -1, false));
        testCases.push_back(TestCase(k[1], k[3], 1, 0, false));
        testCases.push_back(TestCase(k[1], k[3], 1, 1, false));
    }

    // Verify rotation works
    WFC_Image::Kernel3x3 r90 = k[2].rotateCCW();
    WFC_Image::Kernel3x3 r180 = r90.rotateCCW();
    testCases.push_back(TestCase(k[1], r90, 0, -1, true));
    testCases.push_back(TestCase(k[1], r180, -1, 0, true));
    testCases.push_back(TestCase(k[1], r180.rotateCCW(), 0, 1, true));

    // Verify diagonals work properly
    testCases.push_back(TestCase(k[1], k[2], 1, -1, true));
    testCases.push_back(TestCase(k[1], k[2], 1, 1, true));
    testCases.push_back(TestCase(k[1], k[3], -1, -1, true));
    testCases.push_back(TestCase(k[1], k[3], -1, 1, true));

    // Other cases
    testCases.push_back(TestCase(k[4], k[5], 1, 0, true));
    testCases.push_back(TestCase(k[5], k[4], 1, 0, false));
    testCases.push_back(TestCase(k[6], k[7], 1, 0, true));
    testCases.push_back(TestCase(k[7], k[6], 1, 0, true));

    testCases.push_back(TestCase(k[8], k[9], 1, 0, false));
    testCases.push_back(TestCase(k[8], k[9], 0, -1, true));
    testCases.push_back(TestCase(k[8], k[9], -1, 0, false));
    testCases.push_back(TestCase(k[8], k[9], 0, 1, true));

    // Run all test cases
    int total = testCases.size();
    int passes = 0;
    std::vector<TestCase> failures;
    for(int i = 0; i < total; i++)
    {
        bool pass = testCases[i].run();
        
        if(pass)
            passes += 1;
        else
            failures.push_back(testCases[i]);
    }

    std::cout << "Tests Complete." << std::endl << "Passed " << passes << " out of " << total << " cases." << std::endl;
}
#endif