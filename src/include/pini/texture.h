#pragma once
#include <string>
#include <glad/glad.h>

#include <resource.h>

class Texture : public Resource
{
private:
    GLuint textureID;

public:
    Texture(const char * path, bool deferLoading = false);

    virtual void load() override;

    void bind(int index) const;

    GLuint getTextureID() const;
};