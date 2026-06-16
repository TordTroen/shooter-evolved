#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>

// Bidirectional bit-packed stream. Construct with Mode::Write to build a buffer;
// construct from a raw byte pointer to read back. The free serialize() functions
// below work in both directions — same code path for read and write.
class BitStream
{
public:
    enum class Mode { Write, Read };

    BitStream();                                            // write mode
    BitStream(const std::byte* data, size_t byteLen);      // read mode

    bool isWriting() const { return m_mode == Mode::Write; }
    bool isReading() const { return m_mode == Mode::Read; }

    // Error state. Set when a read runs past the end of the buffer, or when a
    // serialize() detects semantically out-of-range data (e.g. a count exceeding
    // its limit). Callers must check this after deserializing and drop the
    // message if it is set — never trust the wire.
    bool hasError() const { return m_error; }
    void markError()      { m_error = true; }

    void     writeBits(uint32_t value, int numBits);
    uint32_t readBits(int numBits);

    void  writeFloat(float v);
    float readFloat();

    void      writeVec3(const glm::vec3& v);
    glm::vec3 readVec3();

    void        writeString(std::string_view s);
    std::string readString();

    // Unified bidirectional ops — dispatch based on mode.
    void serializeBits(uint32_t& v, int numBits);
    void serializeBits(uint8_t&  v, int numBits);
    void serializeFloat(float& v);
    void serializeVec3(glm::vec3& v);
    void serializeString(std::string& v);

    // Write-mode accessors.
    const std::byte* bufferData()  const { return m_bytes.data(); }
    size_t           bufferBytes() const { return (m_bitPos + 7) / 8; }

private:
    Mode             m_mode;
    std::vector<std::byte> m_bytes;   // write mode: growing buffer
    const std::byte* m_readPtr  = nullptr;
    size_t           m_readSize = 0;
    int              m_bitPos   = 0;   // current bit offset (both modes)
    bool             m_error    = false; // read overran buffer or data out of range
};
