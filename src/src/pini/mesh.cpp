#include "mesh.h"

const Mesh * Mesh::QUAD = nullptr;

void Mesh::initPrimitives()
{
    // Create our mesh
    std::vector<vertex>vertices(4);
    // Currently each vertex is: position:vec3, texture:vec2
    vertices[0] = vertex(0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    vertices[1] = vertex(1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    vertices[2] = vertex(1.0f, 1.0f, 0.0f, 1.0f, 1.0f);
    vertices[3] = vertex(0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

    std::vector<GLuint>indices(6);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;
    
    Mesh::QUAD = new Mesh(vertices, indices);
}

Mesh::Mesh(std::vector<vertex> vertices, std::vector<GLuint> indices)
{
    glGenVertexArrays(1, &this->VAO);
    glGenBuffers(1, &this->VBO);
    glGenBuffers(1, &this->EBO);

    glBindVertexArray(this->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Mesh::draw() const
{
    glBindVertexArray(this->VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}