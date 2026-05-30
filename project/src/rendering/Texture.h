#pragma once

#include <glad/glad.h>
#include <cstdint>

class Texture
{
public:
    // Takes raw 8-bit pixel data. `channels` must be 1, 3, or 4.
    // The texture is uploaded as sRGB for 3/4-channel data so colour
    // textures (like glTF baseColor) light correctly.
    Texture(const uint8_t* pixels, int width, int height, int channels);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void bind(GLuint unit) const;

    [[nodiscard]] GLuint id() const { return m_id; }

private:
    GLuint m_id = 0;
};
