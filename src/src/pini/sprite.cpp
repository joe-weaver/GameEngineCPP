#include <sprite.h>
#include <glm/gtc/matrix_transform.hpp>
#include <mesh.h>

void Sprite::draw()
{
    if(this->disabled) return;

    // Get the model for this sprite
    mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, vec3(this->position.x, this->position.y, 0));
    model = glm::scale(model, vec3(this->size.x, this->size.y, 1));

    this->shader->setMat4("uModel", model);
    this->texture->bind(0);
    Mesh::QUAD->draw();
}