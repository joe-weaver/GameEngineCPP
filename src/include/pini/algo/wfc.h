#pragma once

#include <vector>
#include <array>
#include <stack>
#include <queue>
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
    class PixelState
    {
    private:
        Random *rand = nullptr;

    public:
        bool *p = nullptr;
        bool isOwner = false;
        int np = 0;
        bool collapsed = false;
        bool impossible = false;
        int index = -1; // -1 if undecided or impossible [0, np) otherwise

        PixelState(Random * rand, int numPossibilities = 0) : rand(rand), np(numPossibilities), isOwner(true)
        {
            this->p = new bool[this->np];
            for(int i = 0; i < this->np; i++)
                this->p[i] = true;
        }

        PixelState(const PixelState &other): p(other.p), np(other.np),
            collapsed(other.collapsed), impossible(other.impossible), index(other.index), rand(other.rand)
        {}

        ~PixelState()
        {
            // Safe delete in case of copy constructors
            // TODO: Add this back in properly
            // if(this->isOwner)
            //     delete[] this->p;
        }

        float entropy();

        int chooseOne();

        void collapse();
    };

    class Kernel3x3
    {
    private:
        std::array<ColorRGBA, 9> k;
        Kernel3x3() {};
        
    public:
        Kernel3x3(Bitmap *bmp, int x, int y);
    
        Kernel3x3 rotateCCW();
        Kernel3x3 reflectX();

        // offsetX and offsetY are the offset (from top left to bottom right) of other
        bool match(const Kernel3x3 * other, int offsetX, int offsetY) const;

        ColorRGBA getCenter() { return k[4]; }

        std::string toString();
    };

private:
    std::vector<Kernel3x3> kernels; // A list of all kernels generated from the input image
    std::vector<PixelState> output;
    Bitmap * outputImage = nullptr;
    int N = 0;  // The size of the output
    int N2 = 0; // The square of the size of the output
    int K = 3; // The size of our kernel, currently hardcoded at 3
    int hK = 1; // Half the size of our kernel for cleaner math

    std::queue<std::pair<int, int>> needsPropagation; // A queue of items that need propagation, since they changed

    // Others
    Random rand;

public:
    WFC_Image(Bitmap * input, int N, bool generateTransformations = true, int seed = 0);

    bool step() override;

    void propagate(int x, int y);

    const Bitmap * getOutputImage() { return this->outputImage; }

    std::string getDebugOutput();
};