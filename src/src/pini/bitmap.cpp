#include "bitmap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

void Bitmap::load()
{
    int width, height, numChannels;
    this->data = stbi_load(this->filepath.c_str(), &this->width, &this->height, &this->numChannels, 0);

    if(!this->data)
    {
        std::cerr << "Error loading image file!" << std::endl;
    }

    Resource::finishLoad(this->data != nullptr);
}

void Bitmap::unload()
{
    stbi_image_free(this->data);
    Resource::finishUnload();
}

void Bitmap::setPixel(int x, int y, ColorRGBA c)
{
    if(x < 0 || y < 0 || x >= width || y >= height)
        return;

    ColorRGBA * pixels = (ColorRGBA *)this->data;
    pixels[this->width * y + x] = c;
}

ColorRGBA Bitmap::getPixel(int x, int y)
{
    if(x < 0 || y < 0 || x >= width || y >= height)
        return ColorRGBA(0, 0, 0, 0);

    ColorRGBA * pixels = (ColorRGBA *)this->data;
    return pixels[this->width * y + x];
}