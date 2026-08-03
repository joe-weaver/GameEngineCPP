#pragma once

#include <iostream>

#include "resource.h"

class ColorRGBA
{
public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;

    ColorRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : r(r), g(g), b(b), a(a) {}
    
};

// A wrapper around raw char* arrays loaded from stbi
class Bitmap : public Resource
{
private:
    int width = -1;
    int height = -1;
    int numChannels = -1;
    unsigned char * data = nullptr;

public:
    Bitmap(const char * filepath) : Resource(filepath)
    {
        this->load();
    }

    virtual void load() override;

    virtual void unload() override;

    void setPixel(int x, int y, ColorRGBA c);

    ColorRGBA getPixel(int x, int y);

    // Getters
    const int getWidth() const { return this->width; }
    const int getHeight() const { return this->height; }
    const int getNumChannels() const { return this->numChannels; }
    const unsigned char * getData() const { return data; }
};