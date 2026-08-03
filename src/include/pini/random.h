#pragma once

#include <random>
#include "bitmap.h"

class Random
{
private:
    std::mt19937 gen;

public:
    Random()
    {
        std::random_device rd;
        gen.seed(rd());
    }

    Random(int seed)
    {
        gen.seed(seed);
    }

    int range(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    float range(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    ColorRGBA colorRGBA(bool randomAlpha = false)
    {
        std::uniform_int_distribution<int> dist(0, 255);
        return ColorRGBA(dist(gen), dist(gen), dist(gen), randomAlpha ? dist(gen) : 255);
    }
};