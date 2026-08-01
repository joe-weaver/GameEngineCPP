#pragma once
#include <texture.h>
#include <shader.h>

#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using namespace glm;

class vertex
{
public:
    float x = 0;
    float y = 0;
    float z = 0;
    float u = 0;
    float v = 0;

    vertex() {}

    vertex(float x, float y, float z, float u, float v) : x(x), y(y), z(z), u(u), v(v) {}
};

class Mesh
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

public:
    const static Mesh * QUAD;
    static void initPrimitives();

    Mesh(std::vector<vertex> vertices, std::vector<GLuint> indices);

    void draw() const;
};