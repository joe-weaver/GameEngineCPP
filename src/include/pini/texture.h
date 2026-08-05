#pragma once
#include <string>
#include <glad/glad.h>
#include <imgui.h>

#include "bitmap.h"

class Texture
{
protected:
    GLuint textureID;
    GLenum imageFormat;

public:
    Texture(const Bitmap *bmp);

    void bind(int index) const;

    void generateMipmaps();

    GLuint getTextureID() const;

    // Library compatability
    ImTextureRef as_imgui()
    {
        return ImTextureRef(this->textureID);
    }
};

class UpdatingTexture : public Texture
{
protected:
    const Bitmap *bmp = nullptr;

public:
    UpdatingTexture(const Bitmap *bmp) : Texture(bmp), bmp(bmp) {}

    void updateFromBitmap();
};