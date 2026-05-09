#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

static std::string readFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "[Shader] Cannot open: " << path << "\n";
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

GLuint Shader::compile(GLenum type, const std::filesystem::path& path) {
    std::string src = readFile(path);
    if (src.empty()) return 0;

    const char* srcPtr = src.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Compile error in " << path << ":\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Shader::link(GLuint vert, GLuint frag) {
    if (!vert || !frag) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

void Shader::load() {
    GLuint vert = compile(GL_VERTEX_SHADER,   m_vertPath);
    GLuint frag = compile(GL_FRAGMENT_SHADER, m_fragPath);
    GLuint prog = link(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    if (prog) {
        if (m_program) glDeleteProgram(m_program);
        m_program = prog;
    }

    // Record mtimes even on failure so we don't spam reloads on a broken shader.
    if (std::filesystem::exists(m_vertPath)) m_vertMtime = std::filesystem::last_write_time(m_vertPath);
    if (std::filesystem::exists(m_fragPath)) m_fragMtime = std::filesystem::last_write_time(m_fragPath);
}

Shader::Shader(std::string_view vertPath, std::string_view fragPath)
    : m_vertPath(vertPath), m_fragPath(fragPath) {
    load();
}

Shader::~Shader() {
    if (m_program) glDeleteProgram(m_program);
}

void Shader::use() const {
    if (m_program) glUseProgram(m_program);
}

void Shader::checkHotReload() {
    bool changed = false;
    if (std::filesystem::exists(m_vertPath) && std::filesystem::last_write_time(m_vertPath) != m_vertMtime) changed = true;
    if (std::filesystem::exists(m_fragPath) && std::filesystem::last_write_time(m_fragPath) != m_fragMtime) changed = true;
    if (changed) {
        std::cerr << "[Shader] Hot-reloading shaders...\n";
        load();
    }
}

void Shader::setInt(std::string_view name, int v) const {
    if (!m_program) return;
    glUniform1i(glGetUniformLocation(m_program, name.data()), v);
}

void Shader::setFloat(std::string_view name, float v) const {
    if (!m_program) return;
    glUniform1f(glGetUniformLocation(m_program, name.data()), v);
}

void Shader::setVec3(std::string_view name, const glm::vec3& v) const {
    if (!m_program) return;
    glUniform3fv(glGetUniformLocation(m_program, name.data()), 1, glm::value_ptr(v));
}

void Shader::setMat4(std::string_view name, const glm::mat4& m) const {
    if (!m_program) return;
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.data()), 1, GL_FALSE, glm::value_ptr(m));
}
