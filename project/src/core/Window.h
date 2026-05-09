#pragma once

#include <SDL3/SDL.h>
#include <string_view>

struct WindowConfig {
    std::string_view title = "FPS Demo";
    int width = 1280;
    int height = 720;
};

class Window {
public:
    explicit Window(const WindowConfig& cfg);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void swapBuffers() const;
    [[nodiscard]] bool shouldClose() const { return m_shouldClose; }
    void setShouldClose(bool v) { m_shouldClose = v; }

    [[nodiscard]] SDL_Window* handle() const { return m_window; }
    [[nodiscard]] int width() const { return m_width; }
    [[nodiscard]] int height() const { return m_height; }
    void onResize(int w, int h);

private:
    SDL_Window*   m_window  = nullptr;
    SDL_GLContext m_context = nullptr;
    int  m_width;
    int  m_height;
    bool m_shouldClose = false;
};
