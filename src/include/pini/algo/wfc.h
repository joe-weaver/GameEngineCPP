#pragma once

#include <vector>
#include <stack>
#include <utility>
#include <map>

#include "random.h"
#include "bitmap.h"

#define BTOI(x) (x ? 1 : 0)

class WFC
{
protected:
    bool finished = false;
    bool error = false;

public:
    virtual bool step() = 0;
};

// The simplest example of wave function collapse.
// Red, green and blue, but neighboring cells can't be the same color
class WFC_TriColor : public WFC
{
public:
    // The state of our pixels. All start with a value of 3
    class PixelState
    {
    private:
        // To prevent ties, give each pixel a little push
        static Random rand;

    public:
        bool red = 1;
        bool green = 1;
        bool blue = 1;

        float noiseValue;
        bool collapsed = false;

        PixelState() : noiseValue(PixelState::rand.range(-0.2f, 0.0f)) {}

        float entropy()
        {
            // If collapsed, entropy is zero
            return !BTOI(collapsed) * (BTOI(red) + BTOI(green) + BTOI(blue) + noiseValue);
        }
    };

protected:
    int N;
    PixelState * output = nullptr; // An outputSize x outputSize square
    Random rand;
    std::vector<std::pair<int, int>> possibilities;
    std::stack<std::pair<int, int>> decided;
    int stepNumber = 0;

public:
    WFC_TriColor(int N) : N(N)
    {
        // Create our output array
        output = new PixelState[this->N * this->N];
    }

    ~WFC_TriColor()
    {
        delete[] output;
    }

    bool step() override;

    void propagate(int x, int y);
    void propagateOne(int x, int y, int color);

    const PixelState * getOutput()
    {
        return this->output;
    }

protected:
    PixelState * get(int x, int y)
    {
        if(x < 0 || y < 0 || x >= this->N || y >= this->N)
            return nullptr;
        return this->output + (y * this->N + x);
    }

    void set(int x, int y, int color)
    {
        if(x < 0 || y < 0 || x >= this->N || y >= this->N)
            return;
        
        if(color == 1) // Red
            this->output[y * this->N + x].red = false;
        else if(color == 2) // Green
            this->output[y * this->N + x].green = false;
        else if(color == 4)
            this->output[y * this->N + x].blue = false;
    }
};

// Generate a new image that is locally similar to an input image
class WFC_Image : WFC
{
public:
    // Limit to 8 possibilities for now
    class PixelState8
    {
    private:
        static Random rand;
        float intrinsicEntropy; // For breaking ties

    public:
        float probabilities[8] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
        bool collapsed = false;
        bool impossible = false;

        PixelState8() : intrinsicEntropy(PixelState8::rand.range(-0.01f, 0.01f)) {}

        float entropy();

        int chooseOne();

        void collapse(int index);

        u_char bitmask();
    };

    class Kernel3x3
    {
    private:
        u_char k[9];
        int center = -1;

    public:
        void init(Bitmap *bmp, const std::map<ColorRGBA, int> &colorToIndex, int x, int y);

        // Assume basis is padded, otherwise error
        int match(u_char * basis, int x, int y, int w, int h, bool ignoreCenter = true);

        int getCenter() { return this->center; }

        std::string toString();
    };

private:
    std::vector<Kernel3x3> kernels; // A list of all kernels generated from the input image
    u_char * output = nullptr; // Bordered with 0xFF for sanity
    PixelState8 * outputProbabilities; // An array containing the float probabilities for each color
    Bitmap * outputImage = nullptr; 
    int N = 0;  // The size of the output

    // Internel processing
    std::map<ColorRGBA, int> colorToIndex;
    std::vector<ColorRGBA> indexToColor;
    int numColors = 0;

    // Others
    Random rand;

public:
    WFC_Image(Bitmap * input, int N);

    bool step() override;

    void updateProbabilities(int x, int y);

    const Bitmap * getOutputImage() { return this->outputImage; }

    std::string getDebugOutput();
};

// class WFC_Tiles : WFC
// {
// public:
//     static class Rule1x2
//     {
//         int first = 0;
//         int second = 0;

//         Rule1x2(int first, int second) : first(first), second(second) {}
//     };

// protected:
//     int * output = nullptr;
//     int outputSize = 0; // The output is NxN right now
//     int numTiles = 0;
//     std::vector<Rule1x2> rules;

// public:
//     WFC_Tiles(int * output, int outputSize, int numTiles) :
//         output(output), outputSize(outputSize), numTiles(numTiles)
//     {}

//     // Adds a valid pair of neighbors
//     // direction: 1: first-second, 2: second-first, 4: first/second, 8: second/first
//     // By default all directions are allowed
//     void addPair(int first, int second, int direction = 15)
//     {
//         for(int i = 1; i < 16; i = i << 2)
//         {
//             if(direction | i)
//                 rules.push(Rules(first, second, i));
//         }
//     }

//     void step()
//     {
        
//     }
// };