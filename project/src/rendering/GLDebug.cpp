#include "GLDebug.h"

#include <glad/glad.h>
#include <cstdio>

static void GLAPIENTRY debugCallback(
    GLenum source, GLenum type, GLuint id,
    GLenum severity, GLsizei /*length*/,
    const GLchar* message, const void* /*userParam*/)
{
    // Suppress non-significant notifications
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;

    const char* srcStr = [source]() {
        switch (source) {
            case GL_DEBUG_SOURCE_API:             return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
            case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
            case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
            default:                              return "Other";
        }
    }();

    const char* typeStr = [type]() {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               return "Error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undefined Behaviour";
            case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
            case GL_DEBUG_TYPE_PERFORMANCE:         return "Performance";
            default:                                return "Other";
        }
    }();

    const char* sevStr = [severity]() {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:   return "HIGH";
            case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
            case GL_DEBUG_SEVERITY_LOW:    return "LOW";
            default:                      return "NOTIFICATION";
        }
    }();

    fprintf(stderr, "[GL %s] %s/%s (id=%u): %s\n", sevStr, srcStr, typeStr, id, message);
}

void wireUpGLDebugOutput() {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}
