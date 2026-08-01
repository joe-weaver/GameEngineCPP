#include <glm/glm.hpp>

using namespace glm;

class Sprite
{
public:
    vec2 position;
    vec2 scale;

    Sprite(vec2 position, vec2 scale) : position(position), scale(scale) {}
};