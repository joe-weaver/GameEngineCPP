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
    
    bool operator<(const ColorRGBA& other) const
    {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        if (b != other.b) return b < other.b;
        return a < other.a;
    }

    bool operator==(const ColorRGBA &other) const 
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    friend std::ostream& operator<<(std::ostream& os, const ColorRGBA& color);
};

enum BitmapEdgeMode
{
    None,   // Return transparent pixels when out of bounds
    Extend,   // Keep the edge pixel color going
    Wrap,   // Wrap the coordinates around to the other side of the bitmap
};

// A wrapper around raw char* arrays loaded from stbi
class Bitmap : public Resource
{
public :
    static Bitmap * DEFAULT_BITMAP;

    static void initPrimitives();

private:
    int width = -1;
    int height = -1;
    int numChannels = -1;
    unsigned char * data = nullptr;
    BitmapEdgeMode edgeMode;

public:
    Bitmap(const char * filepath, BitmapEdgeMode edgeMode = BitmapEdgeMode::None) : Resource(filepath), edgeMode(edgeMode)
    {
        this->load();
    }

    Bitmap(int width, int height, BitmapEdgeMode edgeMode = BitmapEdgeMode::None);

    virtual void load() override;

    virtual void unload() override;

    void setPixel(int x, int y, ColorRGBA c);

    ColorRGBA getPixel(int x, int y);

    void clear(ColorRGBA c = ColorRGBA());

    void fillFrom(Bitmap * other, float scale);

    // Getters
    int getWidth() const { return this->width; }
    int getHeight() const { return this->height; }
    int getNumChannels() const { return this->numChannels; }
    const unsigned char * getData() const { return data; }
    BitmapEdgeMode getEdgeMode() const { return this->edgeMode; }
    void setEdgeMode(BitmapEdgeMode edgeMode) {this->edgeMode = edgeMode; }

    // Library compatability
    ImVec2 getSize_imgui()
    {
        return ImVec2(width, height);
    }
};