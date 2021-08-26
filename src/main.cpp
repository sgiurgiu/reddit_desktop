#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <filesystem>

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include "glad/glad.h"  // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING)
#define GLFW_INCLUDE_NONE         // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>  // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#include "utils.h"
#include "database.h"
#include "redditdesktop.h"
#include "log/loggingwindow.h"
#include "images/reddit_icon_48.h"

#ifdef REDDIT_DESKTOP_DEBUG
    #include "markdownrenderer.h"
    void ShowMarkdownWindow(bool *open);
#endif

#ifdef CMARK_ENABLED
#include "markdown/cmarkmarkdownparser.h"
#endif

void runMainLoop(SDL_Window* window,ImGuiIO& io);

#if defined(WIN32_WINMAIN)
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow
)
{
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(nullptr, szPath, MAX_PATH);
    std::filesystem::path programPath(szPath);
#else
int main(int /*argc*/, char** argv)
{
    std::filesystem::path programPath(argv[0]);
#endif    
    auto executablePath = programPath.parent_path();

    // Setup window
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        std::cerr << "Error: "<<SDL_GetError()<<"\n";
        return EXIT_FAILURE;
    }


    // Decide GL+GLSL versions
#ifdef __APPLE__
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 430";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 4.3 + GLSL 430
    const char* glsl_version = "#version 430";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Reddit Desktop", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    {
        int iconWidth, iconHeight, iconChannels;
        auto iconData = Utils::decodeImageData(_reddit_icon_48_png,_reddit_icon_48_png_len,
                                               &iconWidth,&iconHeight,&iconChannels);
        auto iconSurface = SDL_CreateRGBSurfaceFrom((void*)iconData.get(),
                                                    iconWidth,iconHeight,32,iconWidth*4,
                                                    0x000000ff,0x0000ff00,0x00ff0000,0xff000000);

        SDL_SetWindowIcon(window, iconSurface);
        SDL_FreeSurface(iconSurface);
    }
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    Database* db = Database::getInstance();
    {
        int x,y,w,h;
        db->getMainWindowDimensions(&x,&y,&w,&h);
        SDL_SetWindowPosition(window,x,y);
        SDL_SetWindowSize(window,w,h);
    }

    // Initialize OpenGL loader
    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        std::cerr <<"Failed to initialize OpenGL loader!\n";
        return EXIT_FAILURE;
    }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    Utils::LoadFonts(executablePath);
    Utils::LoadRedditImages();

#ifdef CMARK_ENABLED
    CMarkMarkdownParser::InitCMarkEngine();
#endif

    runMainLoop(window,io);    

    {
        int x,y,w,h;
        SDL_GetWindowPosition(window,&x,&y);
        SDL_GetWindowSize(window,&w,&h);
        db->setMainWindowDimensions(x,y,w,h);
    }
    Utils::ReleaseRedditImages();
    Utils::DeleteFonts();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef CMARK_ENABLED
    CMarkMarkdownParser::ReleaseCMarkEngine();
#endif

    return EXIT_SUCCESS;
}


void runMainLoop(SDL_Window* window,ImGuiIO& io)
{
#ifdef REDDIT_DESKTOP_DEBUG
    bool show_demo_window = true;
    bool show_markdown_window = false;
#endif
    boost::asio::io_context uiContext;    
    auto work = boost::asio::make_work_guard(uiContext);
    auto desktop = std::make_shared<RedditDesktop>(uiContext);

    desktop->loginCurrentUser();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        //execute whatever work we have in the UI thread

        uiContext.poll_one();
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                 done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                 done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

#ifdef REDDIT_DESKTOP_DEBUG
        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        if(show_markdown_window)
            ShowMarkdownWindow(&show_markdown_window);
#endif
        int windowWidth;
        int windowHeight;
        SDL_GetWindowSize(window,&windowWidth,&windowHeight);

        desktop->setAppFrameHeight(windowHeight);
        desktop->setAppFrameWidth(windowWidth);
        desktop->showDesktop();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        if(!done)
        {
            done = desktop->quitSelected();
        }
    }
    work.reset();
    uiContext.stop();
}

#ifdef REDDIT_DESKTOP_DEBUG
#include <fstream>
void ShowMarkdownWindow(bool *open)
{
    static std::string body;
    if(body.empty())
    {
        std::ifstream f("spec.txt");
        f.seekg(0, std::ios::end);
        body.resize(f.tellg());
        f.seekg(0, std::ios::beg);
        f.read(&body[0], body.size());
    }

    if(body.empty()) return;

//    ImVec2 min(150,150);
//    ImVec2 max(1000,1000);
//    ImGui::SetNextWindowSizeConstraints(min,max);

    if(!ImGui::Begin("Markdown",open,ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }
    //ImGui::SetWindowFocus();

    MarkdownRenderer markdown(body);
    markdown.RenderMarkdown();

    ImGui::End();
}
#endif
