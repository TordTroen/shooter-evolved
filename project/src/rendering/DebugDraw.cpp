#include "DebugDraw.h"
#include "JoltDebugRenderer.h"

void DebugDraw::wireBox(JoltDebugRenderer& renderer, const glm::vec3& center, const glm::quat& rotation,
                        const glm::vec3& halfExtents, JPH::ColorArg color)
{
    glm::vec3 corners[8];
    int i = 0;
    for (float sx : { -1.0f, 1.0f })
        for (float sy : { -1.0f, 1.0f })
            for (float sz : { -1.0f, 1.0f })
                corners[i++] = center + rotation * glm::vec3(sx * halfExtents.x, sy * halfExtents.y, sz * halfExtents.z);

    auto toRVec3 = [](const glm::vec3& v) { return JPH::RVec3(v.x, v.y, v.z); };

    // Index bits: (sx<<2)|(sy<<1)|sz, matching the nested loop order above. Each edge
    // pair differs in exactly one bit (one axis).
    static constexpr int kEdges[12][2] = {
        {0,1}, {2,3}, {4,5}, {6,7}, // vary sz
        {0,2}, {1,3}, {4,6}, {5,7}, // vary sy
        {0,4}, {1,5}, {2,6}, {3,7}, // vary sx
    };
    for (const auto& edge : kEdges)
    {
        renderer.DrawLine(toRVec3(corners[edge[0]]), toRVec3(corners[edge[1]]), color);
    }
}
