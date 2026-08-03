#pragma once
#include <string>
#include <glad/glad.h>

#include "bitmap.h"

class Texture
{
protected:
    GLuint textureID;
    GLenum imageFormat;

public:
    Texture(const Bitmap *bmp);

    void bind(int index) const;

    GLuint getTextureID() const;
};

class UpdatingTexture : public Texture
{
protected:
    const Bitmap *bmp = nullptr;

public:
    UpdatingTexture(const Bitmap *bmp) : Texture(bmp), bmp(bmp) {}

    void updateFromBitmap();
};