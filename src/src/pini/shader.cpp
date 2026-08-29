#include "shader.h"

const Shader * Shader::DEFAULT_SHADER = new Shader();

void Shader::initPrimitives()
{
    const_cast<Shader *>(Shader::DEFAULT_SHADER)->fromFile("default.vert.glsl", "default.frag.glsl");
}