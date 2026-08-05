#pragma once

#include <iostream>

#include <imgui.h>

#include "resource.h"

class ColorRGBA
{
public:
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
    unsigned char a = 0;

    ColorRGBA() {}

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

    Bitmap(int width, int height);

    virtual void load() override;

    virtual void unload() override;

    void setPixel(int x, int y, ColorRGBA c);

    ColorRGBA getPixel(int x, int y);

    void clear(ColorRGBA c = ColorRGBA());

    // Getters
    const int getWidth() const { return this->width; }
    const int getHeight() const { return this->height; }
    const int getNumChannels() const { return this->numChannels; }
    const unsigned char * getData() const { return data; }

    // Library compatability
    ImVec2 getSize_imgui()
    {
        return ImVec2(width, height);
    }
};