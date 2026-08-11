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

enum Language
{
    EN = 0,
    JP = 1
};

Language language = Language::JP;

struct L10nText
{
    const char * en = nullptr;
    const char * jp = nullptr;

    const char * txt();
};

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

class GlobalSettings
{
public:
    void processInputs() {}

    void drawUIs()
    {
        ImGuiIO& io = ImGui::GetIO();

        ImVec2 windowPos = ImVec2(5.0f, 5.0f);
        ImVec2 windowPivot = ImVec2(0.0f, 0.0f);

        ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPivot);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoBackground;

        ImGui::Begin("LanguageButton", nullptr, flags);

        // ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));        // normal state (red)
        // ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f)); // hover state (lighter red)
        // ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));  // pressed state (darker red)

        

        // ImGui::PopStyleColor(3); // pop the same number of colors you pushed
        ImGui::End();
    }
};

const char * L10nText::txt()
    {
        switch (language)
        {
        case Language::JP:
            return jp;
        
        default:
            return en;
        }
    }

struct InputFile_Image
{
    L10nText name;
    const char * filename;
    BitmapEdgeMode edgeMode;
    bool generateTransformations;
};

class DemoManager
{
public:
    virtual void start() = 0;
    virtual void processInputs() = 0;   // Handle any inputs BEFORE rendering (not during)
    virtual void update() = 0;
    virtual void drawGameObjects() = 0;
    virtual void drawUIs() = 0;
};

class WFC_Image_DemoManager : public DemoManager
{
private:
    std::vector<InputFile_Image> inputImages;
    int currentInputIndex = -1;
    Bitmap * basis = nullptr;
    Texture * basisTexture = nullptr;    // The texture to display in imgui
    WFC_Image * wfc = nullptr;
    bool running = false;
    int framesUntilStep = 0;
    int processingSpeed = 0;
    UpdatingTexture * outputTexture = nullptr;  // The texture to display in game
    Sprite * outputSprite = nullptr;    // The sprite to use to display the output

    int outputSize = 32;
    int wrapOutput = false;
    int seed = 123456;

    // UI State variables
    std::vector<L10nText> listItems;
    int selectedListItem = 0;
    bool needsHandleStartStop = false;
    bool needsHandleClear = false;
    bool needsChangeSeed = false;

    enum Element_OutputSize { size16, size32, size64, size96, size128, META_COUNT };
    int elem_os = Element_OutputSize::size32;
    const char * elem_os_names[Element_OutputSize::META_COUNT] = { "16", "32", "64", "96", "128" };
    int outputSizes[Element_OutputSize::META_COUNT] { 16, 32, 64, 96, 128 };


public:
    WFC_Image_DemoManager(vec2 windowSize)
    {
        int size = min(windowSize.x, windowSize.y);
        size = 0.8 * size;
        this->outputSprite = new Sprite(windowSize / 2.f - size / 2.f, vec2(size, size));
    }

    void addInput(InputFile_Image input)
    {
        this->inputImages.push_back(input);
    }

    void changeInput(int index)
    {
        if(index < 0 || index > inputImages.size()) return;
        
        if(this->currentInputIndex != -1)
        {
            delete this->basis;
            this->basis = nullptr;

            delete this->basisTexture;
            this->basisTexture = nullptr;

            delete this->outputTexture;
            this->outputTexture = nullptr;

            delete this->wfc;

            this->currentInputIndex = -1;
        }

        this->currentInputIndex = index;
        this->selectedListItem = index;
        InputFile_Image imageToLoad = this->inputImages[this->currentInputIndex];
        this->basis = new Bitmap(imageToLoad.filename, imageToLoad.edgeMode);
        this->basisTexture = new Texture(this->basis);
        this->wfc = new WFC_Image(this->basis, this->outputSize, imageToLoad.generateTransformations, this->seed, this->wrapOutput);
        this->outputTexture = new UpdatingTexture(this->wfc->getOutputImage());
        
        // Update our sprite with the new texture
        this->outputSprite->updateTexture(this->outputTexture);
    }

    virtual void start() override
    {
        // Arbitrarilty select index 0
        this->changeInput(0);

        // Initialize some of our ui data
        listItems.resize(this->inputImages.size());
        for(int i = 0; i < this->inputImages.size(); i++)
        {
            listItems[i] = this->inputImages[i].name;
        }
    }

    virtual void processInputs() override
    {
        if(this->outputSizes[this->elem_os] != this->outputSize)
        {
            // We need to update our output size
            this->outputSize = this->outputSizes[this->elem_os];
            this->needsHandleClear = true;
            this->wfc->resizeOutput(this->outputSize);

            // Delete the old output texture
            delete this->outputTexture;
            this->outputTexture = nullptr;

            // And create a new one
            this->outputTexture = new UpdatingTexture(this->wfc->getOutputImage());
            this->outputSprite->updateTexture(this->outputTexture);
        }

        if(this->selectedListItem != this->currentInputIndex)
        {
            // We need to change our input
            this->needsHandleClear = false;
            this->needsHandleStartStop = false;
            this->changeInput(this->selectedListItem);
        }

        if(this->needsHandleClear || this->needsChangeSeed)
        {
            this->needsHandleStartStop = false;
            this->needsHandleClear = false;
            this->running = false;

            static Random rand;
            if(this->needsChangeSeed)
                this->wfc->clear(rand.range(1, 999999));
            else
                this->wfc->clear();
            this->outputTexture->updateFromBitmap();
        }

        if(this->needsHandleStartStop)
        {
            this->needsHandleStartStop = false;
            this->running = !this->running;
        }

        if(this->needsChangeSeed)
        {
            this->needsChangeSeed = false;
            this->running = true;
        }
    }

    virtual void update() override
    {
        if(this->running)
        {
            if(this->framesUntilStep-- == 0)
            {
                this->framesUntilStep = this->processingSpeed;
                if(this->wfc->step())
                {
                    this->outputTexture->updateFromBitmap();
                }
                else
                {
                    this->running = false;
                }
            }
        }
    }

    virtual void drawGameObjects() override
    {
        this->outputSprite->draw();
    }
    
    virtual void drawUIs() override
    {
        static double avgProp = 0;
        static double avgColl = 0;
        static int GET_AVG_FRAMES = 60;
        static int framesUntilGetAvg = GET_AVG_FRAMES;

        if(framesUntilGetAvg-- <= 0)
        {
            framesUntilGetAvg = GET_AVG_FRAMES;
            avgProp = this->wfc->avgPropPerStep;
            avgColl = this->wfc->avgCollapsedPerStep;
        }

        ImGui::Begin("##Settings");
        
        static L10nText LANG_BTN_TXT = {"日本語", "English"};
        if (ImGui::Button(LANG_BTN_TXT.txt())) {
            if(language == Language::EN)
                language = Language::JP;
            else
                language = Language::EN;
        }

        static L10nText CHOOSE_TXT = {"Choose Input Image:", "インプットイメージを選ぶ："};
        ImGui::Text(CHOOSE_TXT.txt());

        // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
        const char* combo_preview_value = this->listItems[this->selectedListItem].txt();
        if(ImGui::BeginCombo(" ", combo_preview_value))
        {
            for (int n = 0; n < this->listItems.size(); n++)
            {
                const bool is_selected = (this->selectedListItem == n);
                if(ImGui::Selectable(this->listItems[n].txt(), is_selected))
                    this->selectedListItem = n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        ImGuiIO& io = ImGui::GetIO();
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Layout canvas
        static L10nText IM_LABEL_TXT = {"Current Input", "現在のインプット"};
        ImGui::Text(IM_LABEL_TXT.txt());

        // We want max dimension to be 256
        int w = this->basis->getWidth();
        int h = this->basis->getHeight();
        float ratio = 256.f / float(w > h ? w : h);
        w *= ratio;
        h *= ratio;

        ImGui::InvisibleButton("##Canvas", ImVec2(w, h));
        ImVec2 canvas_min = ImGui::GetItemRectMin();
        ImVec2 canvas_max = ImGui::GetItemRectMax();

        draw_list->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest);
        draw_list->AddImage(this->basisTexture->as_imgui(), canvas_min, canvas_max);
        draw_list->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear);

        static L10nText START_TXT = {"Start", "スタート"};
        static L10nText PAUSE_TXT = {"Pause", "ポーズ"};
        const char * currText = this->running ? PAUSE_TXT.txt() : START_TXT.txt();
        if(ImGui::Button(currText))
            this->needsHandleStartStop = true;
        ImGui::SameLine();

        static L10nText CLEAR_TXT = {"Clear", "クリアー"};
        if(ImGui::Button(CLEAR_TXT.txt()))
            this->needsHandleClear = true;
        ImGui::SameLine();

        static L10nText SEED_TXT = {"Change Seed", "シードを変える"};
        if(ImGui::Button(SEED_TXT.txt()))
            this->needsChangeSeed = true;

        
        const char* elem_name = (elem_os >= 0 && elem_os < Element_OutputSize::META_COUNT) ? elem_os_names[elem_os] : "??";

        ImGui::BeginDisabled(this->running);
        static L10nText OUTPUTSIZE_TXT = {"Output Size:", "アウトプットサイズ："};
        ImGui::Text(OUTPUTSIZE_TXT.txt());
        ImGui::SliderInt("##outputSize", &elem_os, 0, Element_OutputSize::META_COUNT - 1, elem_name); // Use ImGuiSliderFlags_NoInput flag to disable Ctrl+Click here.
        ImGui::EndDisabled();

        ImGui::End();
    }
};

int main(int, char**)
{
    run_tests();

    GLFWwindow* window;

    // Set up error callback and init
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit())
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
    if(!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize GLAD
    if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
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
    ImVec4 clear_color = ImVec4(134.f/255.f, 147.f/255.f, 174.f/255.f, 1.f);

    // Game engine things
    // Setup callbacks
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);    
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Initialize internal game engine values
    Shader::initPrimitives();
    Mesh::initPrimitives();
    Texture::initPrimitives();

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
    GlobalSettings * globalSettings = new GlobalSettings();
    language = Language::JP;
    WFC_Image_DemoManager * wfcImageDM = new WFC_Image_DemoManager(windowSize);

    wfcImageDM->addInput({.name = {"Flower", "花"}, .filename = "Flower.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Trees", "林"}, .filename = "Trees.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Grassy", "高原"}, .filename = "Grassy.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Brick", "レンガ"}, .filename = "Brick.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Maze", "迷路"}, .filename = "Maze.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = true});
    wfcImageDM->addInput({.name = {"QR", "QR"}, .filename = "QR.png", .edgeMode = BitmapEdgeMode::None, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Cave", "洞窟"}, .filename = "Cave.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Spiral", "渦巻き"}, .filename = "Spiral.png", .edgeMode = BitmapEdgeMode::None, .generateTransformations = true});
    wfcImageDM->addInput({.name = {"Regions", "地帯"}, .filename = "Regions.png", .edgeMode = BitmapEdgeMode::None, .generateTransformations = true});
    wfcImageDM->addInput({.name = {"Glyphs", "グリフ"}, .filename = "Glyphs.png", .edgeMode = BitmapEdgeMode::Wrap, .generateTransformations = false});
    wfcImageDM->addInput({.name = {"Gradient", "グラディエント"}, .filename = "Gradient.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = true});
    wfcImageDM->addInput({.name = {"Coastal", "海岸"}, .filename = "Coastal.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = true});
    wfcImageDM->addInput({.name = {"WalledCities", "城郭都市"}, .filename = "WalledCities.png", .edgeMode = BitmapEdgeMode::Extend, .generateTransformations = true});

    wfcImageDM->start();

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

        if(glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        // For now, all objects use the default shader
        Shader::DEFAULT_SHADER->use();

        // These don't change, so set them out here
        Shader::DEFAULT_SHADER->setMat4("uView", camera.getView());
        Shader::DEFAULT_SHADER->setMat4("uProjection", camera.getProjection());

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
        
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error: " << err << std::endl;
        }

        globalSettings->processInputs();
        wfcImageDM->processInputs();
        wfcImageDM->update();
        wfcImageDM->drawGameObjects();

        /* START IMGUI */
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        globalSettings->drawUIs();
        wfcImageDM->drawUIs();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if(show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

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
        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
            mouseState.leftClickStart = currentFrame;
        if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            onLeftClick(globalCamera->screenToWorld(mouseState.currPos));
            mouseState.leftClickStart = 0.0;
        }
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
            mouseState.rightClickStart = currentFrame;
        if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
            mouseState.rightClickStart = 0.0;
        if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
            mouseState.middleClickStart = currentFrame;
        if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
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