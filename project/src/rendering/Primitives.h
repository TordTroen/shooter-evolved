#pragma once
#include "rendering/Vertex.h"
#include <cstdint>
#include <span>

namespace Primitives
{
    std::span<const Vertex>   planeVertices();
    std::span<const uint32_t> planeIndices();
    std::span<const Vertex>   boxVertices();
    std::span<const uint32_t> boxIndices();
}
