#include <glm/glm.hpp>
#include "texture.h"
#include "shader.h"
using namespace glm;

class Sprite
{
private:
    const Texture * texture = nullptr;
    const Shader * shader = nullptr;

public:
    vec2 position;
    vec2 size;
    bool disabled = false;

    Sprite(vec2 position, vec2 size, const Texture * texture = Texture::DEFAULT_TEXTURE, const Shader * shader = Shader::DEFAULT_SHADER) : 
        position(position), size(size), texture(texture), shader(shader) {}

    bool containsPoint(vec2 point)
    {
        bool xOverlaps = point.x >= position.x && point.x <= (position.x + size.x);
        bool yOverlaps = point.y >= position.y && point.y <= (position.y + size.y);

        return xOverlaps && yOverlaps;
    }

    void updateTexture(Texture * texture) { this->texture = texture; }

    void draw();

    void draw_DEBUG() {}
};