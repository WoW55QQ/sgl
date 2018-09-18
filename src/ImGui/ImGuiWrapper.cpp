//
// Created by christoph on 13.09.18.
//

#include <Utils/AppSettings.hpp>
#include <SDL/SDLWindow.hpp>
#include <SDL/HiDPI.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "ImGuiWrapper.hpp"

namespace sgl
{

void ImGuiWrapper::initialize() {
    float scaleFactorHiDPI = getHighDPIScaleFactor();
    float fontScale = scaleFactorHiDPI;
    float uiScale = scaleFactorHiDPI;//*2.0f;

    // --- Code from here on partly taken from ImGui usage example ---
    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.FontGlobalScale = scaleFactorHiDPI*2.0f;

    SDLWindow *window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());
    SDL_GLContext context = window->getGLContext();
    ImGui_ImplSDL2_InitForOpenGL(window->getSDLWindow(), &context);
    const char* glsl_version = "#version 430";
    if (!SystemGL::get()->openglVersionMinimum(4,3)) {
        glsl_version = NULL; // Use standard
    }
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(uiScale); // HiDPI scaling

    // Load Fonts
    io.Fonts->AddFontFromFileTTF("Data/Fonts/DroidSans.ttf", 16.0f*fontScale);
}

void ImGuiWrapper::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiWrapper::processSDLEvent(const SDL_Event &event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void ImGuiWrapper::renderStart()
{
    SDLWindow *window = static_cast<SDLWindow*>(AppSettings::get()->getMainWindow());

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(window->getSDLWindow());
    ImGui::NewFrame();
}

void ImGuiWrapper::renderEnd()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiWrapper::renderDemoWindow()
{
    static bool show_demo_window = true;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
}

void ImGuiWrapper::showHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}