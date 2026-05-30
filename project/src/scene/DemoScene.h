#pragma once

#include "Scene.h"

class Mesh;

class DemoScene : public Scene
{
public:
    DemoScene(const Mesh& planeMesh, const Mesh& boxMesh);

    void setup() override;

private:
    const Mesh& m_planeMesh;
    const Mesh& m_boxMesh;
};
