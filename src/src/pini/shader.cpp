#include "shader.h"

const Shader * Shader::DEFAULT_SHADER = new Shader();

void Shader::initPrimitives()
{
    const_cast<Shader *>(Shader::DEFAULT_SHADER)->fromFile(
        RESOURCE_DIR "/default.vert.glsl",
        RESOURCE_DIR "/default.frag.glsl");
}