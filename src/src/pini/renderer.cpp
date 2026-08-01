#include <renderer.h>

void Renderer::draw(const Mesh * mesh, const Texture * texture, const Shader * shader)
{
    shader->use();
    texture->bind(0);
    mesh->draw();
}