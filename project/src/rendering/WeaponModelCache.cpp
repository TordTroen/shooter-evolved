#include "WeaponModelCache.h"

#include "../actor/components/MeshRenderer.h"
#include "../rendering/Mesh.h"
#include "../rendering/Texture.h"
#include "../weapons/WeaponRegistry.h"

WeaponModelCache::Entry::Entry()  = default;
WeaponModelCache::Entry::~Entry() = default;
WeaponModelCache::~WeaponModelCache() = default;

void WeaponModelCache::load()
{
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        const auto id = static_cast<weapons::WeaponId>(i);
        const weapons::WeaponDef& def = weapons::registry().def(id);
        if (def.modelPath.empty()) { continue; } // out-of-range/unregistered id

        Entry& entry = m_entries[i];
        entry.model = ModelLoader::loadGltf(def.modelPath);
        if (!entry.model.mesh) { continue; }

        entry.meshRenderer          = std::make_unique<MeshRenderer>(entry.model.mesh.get(), glm::vec3(1.0f));
        entry.meshRenderer->texture = entry.model.baseColorTexture.get();
    }
}

MeshRenderer* WeaponModelCache::meshRenderer(weapons::WeaponId id) const
{
    const size_t idx = static_cast<size_t>(id);
    if (idx >= m_entries.size()) { return nullptr; }
    return m_entries[idx].meshRenderer.get();
}
