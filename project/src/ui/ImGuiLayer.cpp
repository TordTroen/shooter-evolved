#include "ImGuiLayer.h"

#include "core/Window.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <SDL3/SDL.h>

namespace
{
    constexpr const char* kGlslVersion = "#version 450";
}

ImGuiLayer::ImGuiLayer(Window& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Don't write imgui.ini

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForOpenGL(window.handle(), window.glContext());
    ImGui_ImplOpenGL3_Init(kGlslVersion);
}

ImGuiLayer::~ImGuiLayer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::processEvent(const SDL_Event& event) const
{
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void ImGuiLayer::beginFrame() const
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
