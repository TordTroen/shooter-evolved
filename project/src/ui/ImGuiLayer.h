#pragma once

union SDL_Event;
class Window;

class ImGuiLayer
{
public:
    explicit ImGuiLayer(Window& window);
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void processEvent(const SDL_Event& event) const;
    void beginFrame() const;
    void endFrame() const;
};
