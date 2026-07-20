#pragma once

#include "ModelLoader.h"
#include "../weapons/WeaponDef.h"

#include <array>
#include <memory>

class MeshRenderer;

// Loads and owns one MeshRenderer per registered weapon, keyed by WeaponId - replaces the
// single hard-wired Game::gunModel now that a player can hold/see more than one weapon
// (MultipleWeapons.md). Client-only: never linked into fps_core/fps_server.
class WeaponModelCache
{
public:
    // Declared here, defined in the .cpp: Entry holds unique_ptr<MeshRenderer> (and, via
    // LoadedModel, unique_ptr<Mesh>/unique_ptr<Texture>), all forward-declared in this
    // header, so the implicit destructor must not be instantiated inline (same pattern as
    // Game's own ~Game()/Mesh/Texture members).
    ~WeaponModelCache();

    // Loads every weapons::registry() entry's model once. No-op entries (failed load) leave
    // meshRenderer() returning nullptr for that id - callers must handle that (matches the
    // existing single-gun-model failure fallback in PlayingState/RemotePlayerRenderer).
    void load();

    // nullptr if the model failed to load or id is out of range.
    [[nodiscard]] MeshRenderer* meshRenderer(weapons::WeaponId id) const;

private:
    struct Entry
    {
        // Both declared here, defined in the .cpp. A defaulted-inline ctor/dtor would have
        // the compiler compute an implicit noexcept specification by inspecting
        // MeshRenderer/Mesh/Texture's destructors - which are only forward-declared in this
        // header - so it must be non-inline (same reasoning as ~WeaponModelCache() below).
        Entry();
        ~Entry();

        ModelLoader::LoadedModel      model;
        std::unique_ptr<MeshRenderer> meshRenderer;
    };

    std::array<Entry, static_cast<size_t>(weapons::WeaponId::Count)> m_entries;
};
