#include "texture.h"

#include <iostream>
#include "bitmap.h"

Texture::Texture(const Bitmap *bmp)
{
    // Associate it with a texture
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    this->imageFormat = (bmp->getNumChannels() == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, this->imageFormat, bmp->getWidth(), bmp->getHeight(), 0, this->imageFormat, GL_UNSIGNED_BYTE, bmp->getData());
}

void Texture::bind(int index) const
{
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
}

unsigned int Texture::getTextureID() const { return this->textureID; }

void UpdatingTexture::updateFromBitmap()
{
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->bmp->getWidth(), bmp->getHeight(), this->imageFormat, GL_UNSIGNED_BYTE, bmp->getData());
}