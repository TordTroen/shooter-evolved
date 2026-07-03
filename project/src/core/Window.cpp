#include "Window.h"

#include <iostream>
#include <glad/glad.h>
#include <stdexcept>
#include <string>
#include <util/CodeTimer.h>

Window::Window(const WindowConfig& cfg)
    : m_width(cfg.width), m_height(cfg.height)
{
    std::cout << "Initing SDL\n";
    {
        CodeTimer timer("SDL init");
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        // if (!SDL_Init(SDL_INIT_VIDEO))
        {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    std::cout << "Creating SDL window\n";
    m_window = SDL_CreateWindow(cfg.title.data(), cfg.width, cfg.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window)
    {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    std::cout << "Creating SDL context\n";
    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context)
    {
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    std::cout << "Loading GL loader\n";
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
    {
        throw std::runtime_error("GLAD failed to load OpenGL functions");
    }
}

Window::~Window()
{
    if (m_context)
    {
        SDL_GL_DestroyContext(m_context);
    }
    if (m_window)
    {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
}

void Window::swapBuffers() const
{
    SDL_GL_SwapWindow(m_window);
}

void Window::onResize(int w, int h)
{
    m_width  = w;
    m_height = h;
    glViewport(0, 0, w, h);
}
