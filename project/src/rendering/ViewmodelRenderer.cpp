#include "ViewmodelRenderer.h"

#include "actor/components/MeshRenderer.h"
#include "rendering/Shader.h"
#include "scene/Camera.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

void ViewmodelRenderer::render(Shader& shader, const Camera& camera, MeshRenderer& meshRenderer,
                               const weapons::WeaponDef& def) const
{
    const glm::vec3 camPos   = camera.position();
    const glm::vec3 camFront = camera.front();
    const glm::vec3 camRight = camera.right();
    const glm::vec3 camUp    = camera.up();

    const glm::vec3 gunPos = camPos
        + camRight * def.rightOffset
        + camUp    * def.downOffset
        + camFront * def.forwardOffset;

    // Build rotation from camera basis.
    // glTF convention is +Y up / -Z forward, so model's -Z maps to camFront.
    glm::mat4 rot(1.0f);
    rot[0] = glm::vec4(camRight,  0.0f);
    rot[1] = glm::vec4(camUp,     0.0f);
    rot[2] = glm::vec4(-camFront, 0.0f);

    // Yaw the model so its barrel (local -X) aligns with local -Z before the
    // camera basis rotation maps -Z to camera-forward.
    const glm::mat4 axisFix =
        glm::rotate(glm::mat4(1.0f), glm::radians(def.axisFixDegrees), glm::vec3(0.0f, 1.0f, 0.0f));

    const glm::mat4 modelMat =
        glm::translate(glm::mat4(1.0f), gunPos) *
        rot *
        axisFix *
        glm::scale(glm::mat4(1.0f), glm::vec3(def.scale));

    glClear(GL_DEPTH_BUFFER_BIT);
    meshRenderer.draw(shader, modelMat);
}

glm::vec3 ViewmodelRenderer::muzzleWorldPos(const Camera& camera, const weapons::WeaponDef& def,
                                            float forwardOffset) const
{
    return camera.position()
        + camera.right() * def.rightOffset
        + camera.up()    * def.downOffset
        + camera.front() * forwardOffset;
}
