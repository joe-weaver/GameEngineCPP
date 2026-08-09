#include "bitmap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

std::ostream& operator<<(std::ostream& os, const ColorRGBA &color) {
    os << "rgba(" << (int)(color.r) << ", "
                    << (int)(color.g) << ", "
                    << (int)(color.b) << ", "
                    << (int)(color.a) << ")";
    return os;
}

Bitmap::Bitmap(int width, int height, BitmapEdgeMode edgeMode) : Resource(""), width(width), height(height), numChannels(4), edgeMode(edgeMode)
{
    // Load an empty bitmap
    this->data = (unsigned char *)new ColorRGBA[this->width * this->height];
    this->clear();
    Resource::finishLoad(true);
}

void Bitmap::load()
{
    this->data = stbi_load(this->filepath.c_str(), &this->width, &this->height, &this->numChannels, 4);

    // TODO: Could handle a different number of channels but ignore for now
    this->numChannels = 4;

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
    if(this->edgeMode == BitmapEdgeMode::Wrap)
    {
        x = x % this->width;
        if(x < 0) x += this->width;
        y = y % this->height;
        if(y < 0) y += this->height;
    }
    else if(this->edgeMode == BitmapEdgeMode::Extend)
    {
        if(x < 0) x = 0;
        if(x >= width) x = width - 1;
        if(y < 0) y = 0;
        if(y >= height) y = height - 1;
    }
    else if(x < 0 || y < 0 || x >= width || y >= height) // BitmapEdgeMode::None and error cases
        return ColorRGBA(0, 0, 0, 0);

    ColorRGBA * pixels = (ColorRGBA *)this->data;
    return pixels[this->width * y + x];
}

void Bitmap::clear(ColorRGBA c)
{
    ColorRGBA * pixels = (ColorRGBA *)this->data;
    for(int i = 0; i < width * height; i++)
    {
        pixels[i] = c;
    }
}