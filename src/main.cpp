#include "imgui.h"

#include <GL/glew.h>    // Initialize with glewInit()

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers


#include <boost/asio/io_context.hpp>
#include <memory>
#include <filesystem>

#if defined(_MSC_VER)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include "RDRect.h"
#include "utils.h"
#include "database.h"
#include "redditdesktop.h"
#include "log/loggingwindow.h"
#include "images/reddit_icon_48.h"
#include "globalresourcescache.h"

#ifdef REDDIT_DESKTOP_DEBUG
    #include "markdownrenderer.h"
    void ShowMarkdownWindow(bool *open);
#endif

#ifdef CMARK_ENABLED
#include "markdown/cmarkmarkdownparser.h"
#endif

void runMainLoop(GLFWwindow* window,ImGuiIO& io);

static void glfw_error_callback(int error, const char* description)
{
    std::cerr<< "GLFW Error "<< error << ": "<< description << std::endl;
}

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
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }


    // Decide GL+GLSL versions
 #if defined(IMGUI_IMPL_OPENGL_ES2)
     // GL ES 2.0 + GLSL 100
     const char* glsl_version = "#version 100";
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
     glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
 #elif defined(__APPLE__)
     // GL 3.2 + GLSL 150
     const char* glsl_version = "#version 150";
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
     glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
 #else
     // GL 3.0 + GLSL 130
     const char* glsl_version = "#version 130";
     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
     //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
     //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
 #endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Reddit Desktop", nullptr, nullptr);
    if (window == nullptr)
    {
      return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    {
        int iconWidth, iconHeight, iconChannels;
        auto iconData = Utils::decodeImageData(_reddit_icon_48_png,_reddit_icon_48_png_len,
                                               &iconWidth,&iconHeight,&iconChannels);
        GLFWimage images[1];
        images[0].pixels = iconData.get();
        images[0].width = iconWidth;
        images[0].height = iconHeight;
        glfwSetWindowIcon(window, 1, images);
    }

    
    glewInit();


    Database* db = Database::getInstance();
    {
        int x,y,w,h;
        db->getMainWindowDimensions(&x,&y,&w,&h);

        int numDisplays = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&numDisplays);
        bool withinBounds = false;
        RDRect windowDimensions {x, y, w, h};
        for (int i = 0; i < numDisplays; i++)
        {
            RDRect monitor;
            glfwGetMonitorWorkarea(monitors[i], &monitor.x, &monitor.y, &monitor.width, &monitor.height);

            auto intersect = getIntersection(windowDimensions, monitor);

            //even if we intersect, we want at least 100px of width or height
            withinBounds = intersect.has_value() &&
                            (intersect->width >= 0 && intersect->height >= 0) &&
                            (intersect->width >= 100 || intersect->height >= 100);
            if (withinBounds) break;
        }

        if (!withinBounds && numDisplays >= 1 /*must be at least one*/)
        {
            //center on the primary screen
            RDRect monitor;
            glfwGetMonitorWorkarea(monitors[0], &monitor.x, &monitor.y, &monitor.width, &monitor.height);
            w = std::min(w,monitor.width);
            h = std::min(h, monitor.height);
            x = monitor.width / 2 - w / 2;
            y = monitor.height / 2 - h / 2;
        }
        glfwSetWindowPos(window,x,y);
        glfwSetWindowSize(window,w,h);
    }


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    auto appConfigFolder = Utils::GetAppConfigFolder();
    auto iniFilePathString = (appConfigFolder / "imgui.ini").string();
    io.IniFilename = iniFilePathString.c_str();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    try
    {
        Utils::LoadFonts(executablePath);
    }
    catch(std::exception const &ex)
    {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    Utils::LoadRedditImages();

#ifdef CMARK_ENABLED
    CMarkMarkdownParser::InitCMarkEngine();
#endif

    runMainLoop(window,io);

    {
        int x,y,w,h;
        glfwGetWindowPos(window, &x,&y);
        glfwGetFramebufferSize(window,&w,&h);
        db->setMainWindowDimensions(x,y,w,h);
    }
    Utils::ReleaseRedditImages();
    Utils::DeleteFonts();
    GlobalResourcesCache::ClearResources();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

#ifdef CMARK_ENABLED
    CMarkMarkdownParser::ReleaseCMarkEngine();
#endif

    return EXIT_SUCCESS;
}


void runMainLoop(GLFWwindow* window,ImGuiIO& io)
{
#ifdef REDDIT_DESKTOP_DEBUG
    bool show_demo_window = true;
    bool show_markdown_window = false;
#endif
    boost::asio::io_context uiContext;    
    auto work = boost::asio::make_work_guard(uiContext);
    auto desktop = std::make_shared<RedditDesktop>(uiContext);

    desktop->loginCurrentUser();

    // Main loop
    bool done = false;
    while (!done)
    {        

        done = glfwWindowShouldClose(window);

        //execute whatever work we have in the UI thread

        uiContext.poll_one();
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
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
        glfwGetFramebufferSize(window,&windowWidth,&windowHeight);

        desktop->setAppFrameHeight(windowHeight);
        desktop->setAppFrameWidth(windowWidth);
        desktop->showDesktop();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        const auto& backgroundColor = desktop->getBackgroundColor();
        glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        if(!done)
        {
            done = desktop->quitSelected();
        }
    }
    desktop->saveBackgroundColor();
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
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[Utils::GetFontIndex(Utils::Fonts::Noto_Medium)]);

//    ImVec2 min(150,150);
//    ImVec2 max(1000,1000);
//    ImGui::SetNextWindowSizeConstraints(min,max);

    if(!ImGui::Begin("Markdown",open,ImGuiWindowFlags_None))
    {
        ImGui::End();
        return;
    }
    ImGui::SetWindowFocus();
    //ImGui::TextUnformatted(reinterpret_cast<const char*>("Bla"  u8"\u1F004" u8"\uFE0F"));
    body = "![gif](giphy|9V1F9o1pBjsxFzHzBr)";
    //MarkdownRenderer markdown(body);
    //markdown.RenderMarkdown();
    ImGui::End();

    ImGui::PopFont();
}
#endif
