#include <texture.h>

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

Texture::Texture(const char * filepath, bool deferLoading) : Resource(filepath)
{
    if(!deferLoading)
    {
        this->load();
    }
}

void Texture::load()
{
    if(this->loaded || this->error) return;

    // Load the image
    int width, height, numChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(this->filepath.c_str(), &width, &height, &numChannels, 0);

    // Associate it with a texture
    glGenTextures(1, &this->textureID);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    bool loadSuccess = false;
    if(data){
        GLenum format = (numChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        loadSuccess = true;
    }
    else
    {
        std::cerr << "Failed to load texture" << std::endl;
    }

    // Now that we have our texture, free the image data;
    stbi_image_free(data);

    Resource::finishLoad(loadSuccess);
}

void Texture::bind(int index) const
{
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, this->textureID);
}

unsigned int Texture::getTextureID() const { return this->textureID; }