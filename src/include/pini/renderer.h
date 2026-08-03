#include "mesh.h"
#include "texture.h"
#include "shader.h"

class Renderer
{
public:
    static void draw(const Mesh * mesh, const Texture * texture, const Shader * shader);
};