#include "Texture.h"

#include <iostream>

Texture::Texture(const uint8_t* pixels, int width, int height, int channels)
{
    GLenum internalFormat = GL_RGBA8;
    GLenum dataFormat     = GL_RGBA;

    switch (channels)
    {
        case 1:
            internalFormat = GL_R8;
            dataFormat     = GL_RED;
            break;
        case 3:
            internalFormat = GL_SRGB8;
            dataFormat     = GL_RGB;
            break;
        case 4:
            internalFormat = GL_SRGB8_ALPHA8;
            dataFormat     = GL_RGBA;
            break;
        default:
            std::cerr << "[Texture] Unsupported channel count: " << channels << "\n";
            return;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &m_id);
    glTextureStorage2D(m_id, 1, internalFormat, width, height);
    glTextureSubImage2D(m_id, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, pixels);
    glGenerateTextureMipmap(m_id);

    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T,     GL_REPEAT);
}

Texture::~Texture()
{
    if (m_id)
    {
        glDeleteTextures(1, &m_id);
    }
}

void Texture::bind(GLuint unit) const
{
    glBindTextureUnit(unit, m_id);
}
