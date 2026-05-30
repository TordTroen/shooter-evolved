#pragma once

#include <memory>
#include <string_view>

class Mesh;
class Texture;

namespace ModelLoader
{
    struct LoadedModel
    {
        std::unique_ptr<Mesh>    mesh;
        std::unique_ptr<Texture> baseColorTexture; // may be null
    };

    // Loads geometry (merged across all primitives) and the first material's
    // baseColor texture from a glTF / GLB file.
    // Returns a LoadedModel with `mesh == nullptr` on failure.
    LoadedModel loadGltf(std::string_view path);
}
