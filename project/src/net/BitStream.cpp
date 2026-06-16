#include "BitStream.h"

#include <cstring>
#include <algorithm>

BitStream::BitStream()
    : m_mode(Mode::Write)
{
}

BitStream::BitStream(const std::byte* data, size_t byteLen)
    : m_mode(Mode::Read)
    , m_readPtr(data)
    , m_readSize(byteLen)
{
}

void BitStream::writeBits(uint32_t value, int numBits)
{
    for (int i = 0; i < numBits; ++i)
    {
        const int byteIdx = m_bitPos / 8;
        const int bitIdx  = m_bitPos % 8;
        if (byteIdx >= static_cast<int>(m_bytes.size()))
            m_bytes.push_back(static_cast<std::byte>(0));
        if (value & (1u << i))
            m_bytes[byteIdx] |= static_cast<std::byte>(1u << bitIdx);
        ++m_bitPos;
    }
}

uint32_t BitStream::readBits(int numBits)
{
    uint32_t result = 0;
    for (int i = 0; i < numBits; ++i)
    {
        const int byteIdx = m_bitPos / 8;
        const int bitIdx  = m_bitPos % 8;
        if (m_readPtr && byteIdx < static_cast<int>(m_readSize))
        {
            if (static_cast<uint8_t>(m_readPtr[byteIdx]) & (1u << bitIdx))
                result |= (1u << i);
        }
        else
        {
            // Reading past the end of the buffer — flag and keep returning zeros
            // so callers drop the message instead of acting on garbage.
            m_error = true;
        }
        ++m_bitPos;
    }
    return result;
}

void BitStream::writeFloat(float v)
{
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof bits);
    writeBits(bits, 32);
}

float BitStream::readFloat()
{
    const uint32_t bits = readBits(32);
    float v;
    std::memcpy(&v, &bits, sizeof v);
    return v;
}

void BitStream::writeVec3(const glm::vec3& v)
{
    writeFloat(v.x);
    writeFloat(v.y);
    writeFloat(v.z);
}

glm::vec3 BitStream::readVec3()
{
    glm::vec3 v;
    v.x = readFloat();
    v.y = readFloat();
    v.z = readFloat();
    return v;
}

void BitStream::writeString(std::string_view s)
{
    const uint32_t len = static_cast<uint32_t>(std::min(s.size(), size_t(65535)));
    writeBits(len, 16);
    for (uint32_t i = 0; i < len; ++i)
        writeBits(static_cast<uint8_t>(s[i]), 8);
}

std::string BitStream::readString()
{
    const uint32_t len = readBits(16);

    // Clamp before allocating: a length larger than the bytes left in the buffer
    // is malformed. Reject rather than reserve/loop on attacker-controlled size.
    const size_t remainingBits = (m_readSize * 8 >= static_cast<size_t>(m_bitPos))
        ? m_readSize * 8 - static_cast<size_t>(m_bitPos)
        : 0;
    if (len > remainingBits / 8)
    {
        m_error = true;
        return {};
    }

    std::string result;
    result.reserve(len);
    for (uint32_t i = 0; i < len; ++i)
        result += static_cast<char>(readBits(8));
    return result;
}

void BitStream::serializeBits(uint32_t& v, int numBits)
{
    if (m_mode == Mode::Write) writeBits(v, numBits);
    else                        v = readBits(numBits);
}

void BitStream::serializeBits(uint8_t& v, int numBits)
{
    if (m_mode == Mode::Write)
        writeBits(static_cast<uint32_t>(v), numBits);
    else
        v = static_cast<uint8_t>(readBits(numBits));
}

void BitStream::serializeFloat(float& v)
{
    if (m_mode == Mode::Write) writeFloat(v);
    else                        v = readFloat();
}

void BitStream::serializeVec3(glm::vec3& v)
{
    if (m_mode == Mode::Write) writeVec3(v);
    else                        v = readVec3();
}

void BitStream::serializeString(std::string& v)
{
    if (m_mode == Mode::Write) writeString(v);
    else                        v = readString();
}
