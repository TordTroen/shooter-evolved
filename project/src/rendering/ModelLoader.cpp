#include "ModelLoader.h"

#include "Mesh.h"
#include "Texture.h"
#include "Vertex.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr cgltf_size kFloatsPerVec3 = 3;
    constexpr cgltf_size kFloatsPerVec2 = 2;

    struct Accum
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;
    };

    const cgltf_attribute* findAttribute(const cgltf_primitive& prim, cgltf_attribute_type type)
    {
        for (cgltf_size i = 0; i < prim.attributes_count; ++i)
        {
            if (prim.attributes[i].type == type && prim.attributes[i].index == 0)
            {
                return &prim.attributes[i];
            }
        }
        return nullptr;
    }

    glm::vec3 materialBaseColor(const cgltf_material* mat)
    {
        if (mat && mat->has_pbr_metallic_roughness)
        {
            const auto& f = mat->pbr_metallic_roughness.base_color_factor;
            return { f[0], f[1], f[2] };
        }
        if (mat && mat->has_pbr_specular_glossiness)
        {
            const auto& f = mat->pbr_specular_glossiness.diffuse_factor;
            return { f[0], f[1], f[2] };
        }
        return glm::vec3(1.0f);
    }

    void appendPrimitive(Accum& acc, const cgltf_primitive& prim, const glm::mat4& nodeXform)
    {
        if (prim.type != cgltf_primitive_type_triangles)
        {
            std::cerr << "[ModelLoader] Skipping non-triangle primitive\n";
            return;
        }

        const cgltf_attribute* posAttr = findAttribute(prim, cgltf_attribute_type_position);
        if (!posAttr)
        {
            std::cerr << "[ModelLoader] Skipping primitive without POSITION\n";
            return;
        }
        if (!prim.indices)
        {
            std::cerr << "[ModelLoader] Skipping unindexed primitive\n";
            return;
        }

        const cgltf_attribute* normalAttr = findAttribute(prim, cgltf_attribute_type_normal);
        const cgltf_attribute* uvAttr     = findAttribute(prim, cgltf_attribute_type_texcoord);

        const cgltf_size vertexCount = posAttr->data->count;
        const uint32_t   baseVertex  = static_cast<uint32_t>(acc.vertices.size());

        // Normals transform by the inverse-transpose of the upper-3x3 to stay
        // correct under non-uniform scale.
        const glm::mat3 normalXform = glm::transpose(glm::inverse(glm::mat3(nodeXform)));

        // Bake the material's baseColor factor into vertex colour. Lets models
        // with no texture (just per-material colours) shade correctly under a
        // single merged-mesh draw call.
        const glm::vec3 matColor = materialBaseColor(prim.material);

        std::vector<float> posBuf(vertexCount * kFloatsPerVec3);
        cgltf_accessor_unpack_floats(posAttr->data, posBuf.data(), posBuf.size());

        std::vector<float> nrmBuf;
        if (normalAttr && normalAttr->data->count == vertexCount)
        {
            nrmBuf.resize(vertexCount * kFloatsPerVec3);
            cgltf_accessor_unpack_floats(normalAttr->data, nrmBuf.data(), nrmBuf.size());
        }

        std::vector<float> uvBuf;
        if (uvAttr && uvAttr->data->count == vertexCount)
        {
            uvBuf.resize(vertexCount * kFloatsPerVec2);
            cgltf_accessor_unpack_floats(uvAttr->data, uvBuf.data(), uvBuf.size());
        }

        acc.vertices.reserve(acc.vertices.size() + vertexCount);
        for (cgltf_size i = 0; i < vertexCount; ++i)
        {
            const glm::vec3 localPos = {
                posBuf[i * kFloatsPerVec3 + 0],
                posBuf[i * kFloatsPerVec3 + 1],
                posBuf[i * kFloatsPerVec3 + 2],
            };

            glm::vec3 localNrm = { 0.0f, 1.0f, 0.0f };
            if (!nrmBuf.empty())
            {
                localNrm = {
                    nrmBuf[i * kFloatsPerVec3 + 0],
                    nrmBuf[i * kFloatsPerVec3 + 1],
                    nrmBuf[i * kFloatsPerVec3 + 2],
                };
            }

            glm::vec2 uv = { 0.0f, 0.0f };
            if (!uvBuf.empty())
            {
                uv = {
                    uvBuf[i * kFloatsPerVec2 + 0],
                    uvBuf[i * kFloatsPerVec2 + 1],
                };
            }

            Vertex v{};
            v.position = glm::vec3(nodeXform * glm::vec4(localPos, 1.0f));
            v.normal   = glm::normalize(normalXform * localNrm);
            v.uv       = uv;
            v.color    = matColor;
            acc.vertices.push_back(v);
        }

        const cgltf_size indexCount = prim.indices->count;
        std::vector<uint32_t> localIndices(indexCount);
        cgltf_accessor_unpack_indices(
            prim.indices, localIndices.data(), sizeof(uint32_t), indexCount);

        acc.indices.reserve(acc.indices.size() + indexCount);
        for (uint32_t idx : localIndices)
        {
            acc.indices.push_back(idx + baseVertex);
        }
    }

    void processNode(Accum& acc, const cgltf_node* node)
    {
        if (node->mesh)
        {
            float mat[16];
            cgltf_node_transform_world(node, mat);
            const glm::mat4 xform = glm::make_mat4(mat);

            for (cgltf_size p = 0; p < node->mesh->primitives_count; ++p)
            {
                appendPrimitive(acc, node->mesh->primitives[p], xform);
            }
        }

        for (cgltf_size c = 0; c < node->children_count; ++c)
        {
            processNode(acc, node->children[c]);
        }
    }

    void dumpMaterialInfo(const cgltf_data& data)
    {
        std::cout << "[ModelLoader] images_count=" << data.images_count
                  << ", textures_count=" << data.textures_count << "\n";

        for (cgltf_size i = 0; i < data.materials_count; ++i)
        {
            const cgltf_material& m = data.materials[i];
            const char* name = m.name ? m.name : "<unnamed>";
            std::cout << "[ModelLoader]   material[" << i << "] '" << name << "'"
                      << " mr=" << (m.has_pbr_metallic_roughness ? 1 : 0)
                      << " sg=" << (m.has_pbr_specular_glossiness ? 1 : 0);
            if (m.has_pbr_metallic_roughness)
            {
                const auto& pbr = m.pbr_metallic_roughness;
                std::cout << " baseColorTex=" << (pbr.base_color_texture.texture ? 1 : 0)
                          << " baseColorFactor=("
                          << pbr.base_color_factor[0] << ","
                          << pbr.base_color_factor[1] << ","
                          << pbr.base_color_factor[2] << ","
                          << pbr.base_color_factor[3] << ")";
            }
            if (m.has_pbr_specular_glossiness)
            {
                const auto& sg = m.pbr_specular_glossiness;
                std::cout << " diffuseTex=" << (sg.diffuse_texture.texture ? 1 : 0);
            }
            std::cout << " normalTex="   << (m.normal_texture.texture   ? 1 : 0)
                      << " emissiveTex=" << (m.emissive_texture.texture ? 1 : 0)
                      << "\n";
        }
    }

    // Returns the first baseColor / diffuse image found across all materials, or nullptr.
    const cgltf_image* findFirstBaseColorImage(const cgltf_data& data)
    {
        for (cgltf_size i = 0; i < data.materials_count; ++i)
        {
            const cgltf_material& mat = data.materials[i];
            if (mat.has_pbr_metallic_roughness && mat.pbr_metallic_roughness.base_color_texture.texture)
            {
                const cgltf_texture* tex = mat.pbr_metallic_roughness.base_color_texture.texture;
                if (tex->image) return tex->image;
            }
            if (mat.has_pbr_specular_glossiness && mat.pbr_specular_glossiness.diffuse_texture.texture)
            {
                const cgltf_texture* tex = mat.pbr_specular_glossiness.diffuse_texture.texture;
                if (tex->image) return tex->image;
            }
        }
        // Fallback: if there are images in the file at all, try the first one.
        if (data.images_count > 0)
        {
            return &data.images[0];
        }
        return nullptr;
    }

    std::unique_ptr<Texture> loadEmbeddedImage(const cgltf_image& image)
    {
        if (!image.buffer_view || !image.buffer_view->buffer || !image.buffer_view->buffer->data)
        {
            std::cerr << "[ModelLoader] baseColor image is not embedded — external URIs not supported yet\n";
            return nullptr;
        }

        const uint8_t* base = static_cast<const uint8_t*>(image.buffer_view->buffer->data)
                            + image.buffer_view->offset;
        const int byteSize = static_cast<int>(image.buffer_view->size);

        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(1); // glTF uses top-left origin; OpenGL expects bottom-left.
        uint8_t* pixels = stbi_load_from_memory(base, byteSize, &width, &height, &channels, 0);
        if (!pixels)
        {
            std::cerr << "[ModelLoader] stb_image failed to decode embedded image: "
                      << stbi_failure_reason() << "\n";
            return nullptr;
        }

        auto tex = std::make_unique<Texture>(pixels, width, height, channels);
        stbi_image_free(pixels);

        std::cout << "[ModelLoader] Decoded baseColor texture: "
                  << width << "x" << height << " (" << channels << " ch)\n";
        return tex;
    }
}

namespace ModelLoader
{
    LoadedModel loadGltf(std::string_view path)
    {
        const std::string pathStr(path);

        cgltf_options options{};
        cgltf_data*   data = nullptr;
        if (cgltf_parse_file(&options, pathStr.c_str(), &data) != cgltf_result_success)
        {
            std::cerr << "[ModelLoader] Failed to parse: " << pathStr << "\n";
            return {};
        }

        if (cgltf_load_buffers(&options, data, pathStr.c_str()) != cgltf_result_success)
        {
            std::cerr << "[ModelLoader] Failed to load buffers for: " << pathStr << "\n";
            cgltf_free(data);
            return {};
        }

        Accum acc;

        if (data->scene && data->scene->nodes_count > 0)
        {
            for (cgltf_size i = 0; i < data->scene->nodes_count; ++i)
            {
                processNode(acc, data->scene->nodes[i]);
            }
        }
        else
        {
            for (cgltf_size i = 0; i < data->nodes_count; ++i)
            {
                if (data->nodes[i].parent == nullptr)
                {
                    processNode(acc, &data->nodes[i]);
                }
            }
        }

        std::cout << "[ModelLoader] Loaded " << pathStr
                  << " — meshes=" << data->meshes_count
                  << ", nodes=" << data->nodes_count
                  << ", materials=" << data->materials_count
                  << ", verts=" << acc.vertices.size()
                  << ", indices=" << acc.indices.size() << "\n";

        dumpMaterialInfo(*data);

        std::unique_ptr<Texture> baseColor;
        if (const cgltf_image* img = findFirstBaseColorImage(*data))
        {
            baseColor = loadEmbeddedImage(*img);
        }

        cgltf_free(data);

        LoadedModel result;
        if (acc.vertices.empty() || acc.indices.empty())
        {
            std::cerr << "[ModelLoader] No geometry extracted from: " << pathStr << "\n";
            return result;
        }

        result.mesh             = std::make_unique<Mesh>(acc.vertices, acc.indices);
        result.baseColorTexture = std::move(baseColor);
        return result;
    }
}
