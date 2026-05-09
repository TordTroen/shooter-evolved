#include "Actor.h"
#include "rendering/Mesh.h"
#include "rendering/Shader.h"

Actor::Actor(const Mesh* mesh, glm::mat4 model)
    : mesh(mesh), model(model)
{
}

void Actor::draw(Shader& shader) const
{
    shader.setMat4("model", model);
    shader.setVec3("uColor", color);
    mesh->draw();
}
