#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <string_view>

class Shader {
public:
    Shader(std::string_view vertPath, std::string_view fragPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    // Check mtimes and recompile if either source file changed.
    void checkHotReload();

    void setInt  (std::string_view name, int             v) const;
    void setFloat(std::string_view name, float           v) const;
    void setVec3 (std::string_view name, const glm::vec3& v) const;
    void setMat4 (std::string_view name, const glm::mat4& m) const;

private:
    GLuint m_program = 0;
    std::filesystem::path m_vertPath;
    std::filesystem::path m_fragPath;
    std::filesystem::file_time_type m_vertMtime{};
    std::filesystem::file_time_type m_fragMtime{};

    static GLuint compile(GLenum type, const std::filesystem::path& path);
    static GLuint link(GLuint vert, GLuint frag);
    void load();
};
