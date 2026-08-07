#include "wfc.h"

#include <iomanip>
#include <sstream>

Random WFC_TriColor::PixelState::rand;
Random WFC_Image::PixelState8::rand;

bool WFC_TriColor::step()
{
    if(this->finished)
        return false;
        
    // std::cout << "step " << this->stepNumber++ << ": " << std::endl;

    // First step, find a pixels with minimal entropy (i.e. the fewest possibilities)
    int x, y;
    bool foundOne = false;

    // We might have pixels that were already decided, but need to propagate
    if(!decided.empty())
    {
        // We have to work on this this frame
        x = decided.top().first;
        y = decided.top().second;
        decided.pop();
        foundOne = true;
    }
    else
    {
        // We need to find a new tile to work on from our output
        float e_best = 1000.f;
        int x_best, y_best;

        for(int y = 0; y < this->N; y++)
        {
            for(int x = 0; x < this->N; x++)
            {
                float e = this->output[y*this->N + x].entropy();
                if(e > 0 && e < e_best)
                {
                    x_best = x;
                    y_best = y;
                    e_best = e;
                }
            }
        }

        if(e_best < 1000.f)
        {
            x = x_best;
            y = y_best;
            foundOne = true;
        }
    }

    // Check if we've finished our algorithm
    if(!foundOne)
    {
        this->finished = true;
        return false;
    }

    // Collapse this pixel
    PixelState * p = &this->output[y*this->N + x];
    float e = this->output[y*this->N + x].entropy();

    // std::cout << "Collapsing pixel (" << x << ", " << y << ") with e = " << e;
    if(e > 2.f)
    {
        // 3 Possiblities
        int idx = this->rand.range(0, 2);
        if(idx == 0)
        {
            // Red
            p->green = false;
            p->blue = false;
        }
        else if(idx == 1)
        {
            // Green
            p->red = false;
            p->blue = false;
        }
        else
        {
            // Blue
            p->red = false;
            p->green = false;
        }

        p->collapsed = true;
    }
    else if(e > 1.f)
    {
        // 2 Possiblities
        int idx = this->rand.range(0, 1);
        if(idx == 0)
        {
            if(p->red)  // Red
            {
                p->green = false;
                p->blue = false;
            }
            else    // Green
            {
                p->blue = false;
            }
        }
        else
        {
            if(p->red && p->green)  // Green
            {
                p->red = false;
            }
            else    // Blue
            {
                p->red = false;
                p->green = false;
            }
        }

        p->collapsed = true;
    }
    else
    {
        // Only one possibility
        p->collapsed = true;
    }

    this->propagate(x, y);

    // if(p->red) std::cout << " to red";
    // else if(p->green) std::cout << " to green";
    // else if(p->blue) std::cout << " to blue";
    // std::cout << std::endl;

    return true;
}

void WFC_TriColor::propagate(int x, int y)
{
    // Update state of neighboring cells
    PixelState * state = this->get(x, y);

    // Safe bounds checks are handled in set and get
    // Update neighbors
    int color;
    if(state->red)
        color = 1;
    else if(state->green)
        color = 2;
    else // Blue
        color = 4;

    // Check if neighbors are decided
    this->propagateOne(x + 1, y, color);
    this->propagateOne(x - 1, y, color);
    this->propagateOne(x, y + 1, color);
    this->propagateOne(x, y - 1, color);
}

void WFC_TriColor::propagateOne(int x, int y, int color)
{
    PixelState * state = this->get(x, y);

    if(state)
    {
        if(state->entropy() > 1)
        {
            // This pixel isn't yet decided 
            this->set(x, y, color);
            
            // Check if this was just decided
            if(state->entropy() < 1)
                this->decided.push({x, y});
        }
    }
}

void WFC_Image::Kernel3x3::init(Bitmap *bmp, const std::map<ColorRGBA, int> &colorToIndex, int x, int y)
{
    this->center = colorToIndex.at(bmp->getPixel(x, y));

    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            int localX = x - 1 + c;
            int localY = y - 1 + r;

            if(localX < 0 || localY < 0 || localX >= bmp->getWidth() || localY >= bmp->getHeight())
                this->k[r*3 + c] = 0xFF;
            else
                this->k[r*3 + c] = 1 << colorToIndex.at(bmp->getPixel(localX, localY));
        }
    }
}

std::string WFC_Image::Kernel3x3::toString()
{
    std::stringstream ss;
    ss << (int)k[0] << (int)k[1] << (int)k[2] << std::endl
        << (int)k[3] << (int)k[4] << (int)k[5] << std::endl
        << (int)k[6] << (int)k[7] << (int)k[8] << std::endl;
    
        return ss.str();
}

// Assume basis is padded, otherwise error
int WFC_Image::Kernel3x3::match(u_char * basis, int x, int y, int w, int h, bool ignoreCenter)
{
    int matchCount = 0;

    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            if(ignoreCenter && r == 1 && c == 1)
                continue;
            
            int localX = x + c - 1;
            int localY = y + r - 1;

            matchCount += ((this->k[r*3 + c] & basis[localY * w + localX]) != 0) ? 1 : 0;
        }
    }

    return matchCount;
}

float WFC_Image::PixelState8::entropy()
{
    float sum = 0.f;
    for(int i = 0; i < 8; i++)
    {
        float p = this->probabilities[i];
        sum -= (p == 0) ? 0 : (p * log(p));
    }

    return sum * (1.f + this->intrinsicEntropy);
}

int WFC_Image::PixelState8::chooseOne()
{
    float chance = rand.range(0.f, 0.999f); // Not exactly one to deal with float math
    float sum = 0.f;
    for(int i = 0; i < 8; i++)
    {
        sum += this->probabilities[i];
        if(sum >= chance)
            return i;
    }

    // We would only reach here in the case of really bad float math
    // So we can keep going, ignore probabilites and return a valid index
    for(int i = 0; i < 8; i++)
    {
        if(this->probabilities[i] > 0.0)
            return i;
    }

    // This is truly an error state
    return -1;
}

void WFC_Image::PixelState8::collapse(int index)
{
    for(int i = 0; i < 8; i++)
    {
        if(i == index)
            this->probabilities[i] = 1.f;
        else
            this->probabilities[i] = 0.f;
    }
    this->collapsed = true;
}

u_char WFC_Image::PixelState8::bitmask()
{
    u_char bits = 0;
    for(int i = 0; i < 8; i++)
    {
        if(this->probabilities[i] > 0.0)
            bits |= (1 << i);
    }

    return bits;
}

WFC_Image::WFC_Image(Bitmap * input, int N) :  N(N)
{
    int w = input->getWidth();
    int h = input->getHeight();
    
    // Find all different colors in the input image and create indices for them
    std::vector<int> counts;
    this->numColors = 0;

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            ColorRGBA color = input->getPixel(x, y);
            if(!colorToIndex.contains(color))
            {
                // This is a new color
                colorToIndex[color] = this->numColors;
                this->numColors += 1;

                indexToColor.push_back(color);
                counts.push_back(0);
            }

            counts[colorToIndex[color]] += 1;
        }
    }

    // Generate kernels for every pixel of the input image
    // Pixels extend their borders out of bounds
    kernels.resize(w * h);
    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            kernels[y*w + x].init(input, colorToIndex, x, y);
        }
    }

    std::cout << "Found " << this->numColors << " colors:" << std::endl;
    int idx = 0;
    for(auto c : indexToColor)
    {
        std::cout << "\tColor " << idx << ":" << std::endl;
        std::cout << "\t\t" << c << std::endl;
        std::cout << "\t\tcount: " << counts[idx++] << std::endl;
    }

    // Generate our initial state
    // Initialize bit output. Bits represent valid tile indices
    this->output = new u_char[(N + 2)*(N + 2)]; // Pad for ease of use with kernel
    for(int i = 0; i < (N + 2)*(N + 2); i++)
        this->output[i] = 0xff;
    
    this->outputImage = new Bitmap(N, N);
    this->outputImage->clear(ColorRGBA(255, 0, 0, 255));
    
    // Initialize initial probabilites
    this->outputProbabilities = new PixelState8[N*N];
    for(int i = 0; i < N*N; i++)
    {
        for(int j = 0; j < this->numColors; j++)
        {
            // Calcuate the probability for this color
            this->outputProbabilities[i].probabilities[j] = (float)counts[j] / (float)(w * h);
        }
    }
}

bool WFC_Image::step()
{
    static int stepNumber = 0;
    if(this->finished)
    {
        return false;
    }
    else
    {
        std::cout << "Step " << stepNumber << ":" << std::endl;
    }

    // Find the output pixel with the lowest entropy (that isn't already determined or impossible)
    float e_best = 1000.f;
    int x_best, y_best;
    for(int y = 0; y < this->N; y++)
    {
        for(int x = 0; x < this->N; x++)
        {
            if(this->outputProbabilities[y*this->N + x].collapsed || this->outputProbabilities[y*this->N + x].impossible)
                continue;
            
            float e = this->outputProbabilities[y*this->N + x].entropy();
            if(e < e_best)
            {
                e_best = e;
                x_best = x;
                y_best = y;
            }
        }
    }

    if(e_best == 1000.f)
    {
        // We're done, everything is determined or impossible
        this->finished = true;
        std::cout << "Finished!" << std::endl;
        return false;
    }

    std::cout << "Best is: (" << x_best << ", " << y_best << ") with e: " << e_best << std::endl;

    // Collapse this cell to one of it's probabilities at random
    int index = this->outputProbabilities[y_best*this->N + x_best].chooseOne();
    this->outputProbabilities[y_best*this->N + x_best].collapse(index);
    this->output[(y_best + 1) * (this->N + 2) + x_best + 1] = 1 << index;
    this->outputImage->setPixel(x_best, y_best, this->indexToColor[index]);

    std::cout << "\tDecided on " << index << ": " << this->indexToColor[index] << std::endl;

    // TODO: Propagate values to neighbors
    for(int yOff = -1; yOff <= 1; yOff++)
        for(int xOff = -1; xOff <= 1; xOff++)
            if(!(xOff == 0 && yOff == 0))
                updateProbabilities(x_best + xOff, y_best + yOff);

    return true;
}

void WFC_Image::updateProbabilities(int x, int y)
{
    // Check if this cell is in bounds
    if(x < 0 || y < 0 || x >= this->N || y >= this->N)
        return;
    
    if(this->outputProbabilities[y*this->N + x].collapsed)
        return;

    // Compare our neighborhood to the kernels and calculate our likelihood for each pixel
    std::vector<int> counts(this->numColors);
    int total = 0;

    std::cout << "(" << x << ", " << y << "):";

    for(int i = 0; i < kernels.size(); i++)
    {
        int numMatchingPixels = kernels[i].match(this->output, x + 1, y + 1, this->N + 2, this->N + 2);
        bool fullMatch = 8 == numMatchingPixels;

        if(fullMatch)
        {
            // This color is still possible
            int index = kernels[i].getCenter();
            counts[index] += 1;
            total += 1;
        }
    }

    if(total == 0)
    {
        // There are no possiblities
        // TODO: Decide how to handle this case
        this->outputProbabilities[y*this->N + x].impossible = true;
        this->outputImage->setPixel(x, y, ColorRGBA(0, 255, 0, 255));
    }
    else
    {
        // Update our probability vector accordingly
        for(int i = 0; i < this->numColors; i++)
        {
            this->outputProbabilities[y*this->N + x].probabilities[i] = counts[i] / float(total);
        }
    }
}

std::string WFC_Image::getDebugOutput()
{
    std::stringstream ss;
    for(int y = 0; y < this->N; y++)
    {
        ss << "----------------------------------------" << std::endl;
        for(int x = 0; x < this->N; x++)
        {
            if(x != 0)
                ss << " | (";
            else
                ss << "(";
            
            ss << std::setprecision(2) << this->outputProbabilities[y*this->N + x].probabilities[0];
            ss << ", ";
            ss << std::setprecision(2) << this->outputProbabilities[y*this->N + x].probabilities[1];
            ss << ")";
        }
        ss << std::endl;
    }
    ss << "----------------------------------------" << std::endl;
    
    return ss.str();
}