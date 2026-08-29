#include "wfc.h"

#include <iomanip>
#include <sstream>

Random WFC_TriColor::PixelState::rand;

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

const struct {int x; int y;} offsets[4] = {{1, 0}, {0, -1}, {-1, 0}, {0, 1}};

WFC_Image::Kernel3x3::Kernel3x3(Bitmap *bmp, int x, int y)
{
    for(int r = 0; r < 3; r++)
    {
        for(int c = 0; c < 3; c++)
        {
            int localX = x - 1 + c;
            int localY = y - 1 + r;

            this->k[r*3 + c] = bmp->getPixel(localX, localY);
        }
    }
}

WFC_Image::Kernel3x3 WFC_Image::Kernel3x3::rotateCCW()
{
    Kernel3x3 other = Kernel3x3();
    other.k[0] = this->k[2];
    other.k[1] = this->k[5];
    other.k[2] = this->k[8];
    other.k[3] = this->k[1];
    other.k[4] = this->k[4];
    other.k[5] = this->k[7];
    other.k[6] = this->k[0];
    other.k[7] = this->k[3];
    other.k[8] = this->k[6];

    return other;
}

WFC_Image::Kernel3x3 WFC_Image::Kernel3x3::reflectX()
{
    Kernel3x3 other = Kernel3x3();
    other.k[0] = this->k[2];
    other.k[1] = this->k[1];
    other.k[2] = this->k[0];
    other.k[3] = this->k[5];
    other.k[4] = this->k[4];
    other.k[5] = this->k[3];
    other.k[6] = this->k[8];
    other.k[7] = this->k[7];
    other.k[8] = this->k[6];

    return other;
}

std::string WFC_Image::Kernel3x3::toString()
{
    std::map<ColorRGBA, char> ctoc;
    ctoc[ColorRGBA(155, 173, 183, 255)] = '.';
    ctoc[ColorRGBA(0, 0, 0, 255)] = '#';

    std::stringstream ss;
    ss << ctoc[k[0]] << ctoc[k[1]] << ctoc[k[2]] << std::endl
        << ctoc[k[3]] << ctoc[k[4]] << ctoc[k[5]] << std::endl
        << ctoc[k[6]] << ctoc[k[7]] << ctoc[k[8]] << std::endl;
    
    return ss.str();
}

bool WFC_Image::Kernel3x3::match(const Kernel3x3 * other, int offsetX, int offsetY) const 
{
    const int K = 3;

    // For "all" of our pixels
    for(int y = 0; y < K; y++)
    {
        for(int x = 0; x < K; x++)
        {
            // Check this pixel with a corresponding offset in the other kernel
            int otherX = x - offsetX;
            int otherY = y - offsetY;

            // Do a bounds check
            if(otherX < 0 || otherY < 0 || otherX >= K || otherY >= K)
                continue;
            
            if(this->k[y*K + x] != other->k[otherY*K + otherX])
                return false;
        }
    }

    // All matched
    return true;
}

WFC_Image::PixelState::PixelState(Random * rand, int numPossibilities) :
    rand(rand), np(numPossibilities), statesLeft(numPossibilities)
{
    this->p = new bool[this->np];
    for(int i = 0; i < this->np; i++)
        this->p[i] = true;
    
    this->intrinsicEntropy = rand->range(0.0, 0.1);
}

void WFC_Image::PixelState::clear()
{
    this->statesLeft = this->np;
    this->collapsed = false;
    this->impossible = false;
    this->index = -1;

    for(int i = 0; i < this->np; i++)
        this->p[i] = true;
    
    this->intrinsicEntropy = rand->range(0.0, 0.1);
}

double WFC_Image::PixelState::entropy()
{
    return this->statesLeft + this->intrinsicEntropy;
}

int WFC_Image::PixelState::chooseOne()
{
    int numLeft = 0;
    for(int i = 0; i < this->np; i++)
        numLeft += this->p[i] ? 1 : 0;

    if(numLeft == 0)
        return -1;
        
    int r = this->rand->range(0, numLeft - 1);
    for(int i = 0; i < this->np; i++)
    {
        if(this->p[i] && r-- == 0)
            return i;   // We choose the ith kernal
    }

    // This should never happen, error
    return -1;
}

void WFC_Image::PixelState::collapse(bool chooseFromMany)
{
    if(chooseFromMany)
    {
        int index = this->chooseOne();
        
        if(index == -1)
        {
            // Handle error state - make this impossible
            this->impossible = true;
            for(int i = 0; i < this->np; i++)
                this->p[i] = false;
            return;
        }

        for(int i = 0; i < this->np; i++)
        {
            if(i == index)
                this->p[i] = true;
            else
                this->p[i] = false;
        }

        this->index = index;
    }
    else
    {
        for(int i = 0; i < this->np; i++)
        {
            if(this->p[i])
            {
                this->index = i;
                break;
            }
        }
    }
    
    this->collapsed = true;
}

#define MATCH(i, j, idx) this->matches[i * 4 * this->numKernels + j * 4 + idx]

WFC_Image::WFC_Image(Bitmap * input, int N, bool generateTransformations, int seed, bool wrapOutput) :
    N(N), N2(N*N), K(3), hK(3/2), wrapOutput(wrapOutput), seed(seed)
{
    // Seed the random number generator
    if(seed != 0)
        rand.seed(seed);

    int w = input->getWidth();
    int h = input->getHeight();
    int ox = 0;
    int oy = 0;
    
    // Generate kernels for every KxK of the input image (in this case 3x3)
    if(input->getEdgeMode() == BitmapEdgeMode::None)
    {
        w -= this->K - 1;
        h -= this->K - 1;
        ox = this->hK;
        oy = this->hK;
    }

    for(int y = 0; y < h; y++)
    {
        for(int x = 0; x < w; x++)
        {
            bool allowDuplicates = false;

            auto addToKernel = [this, allowDuplicates](const Kernel3x3 &k) {
                if(!allowDuplicates)
                {
                    bool alreadyExists = false;
                    for(int i = 0; i < this->kernels.size(); i++)
                    {
                        if(this->kernels[i].match(&k, 0, 0))
                        {
                            alreadyExists = true;
                            break;
                        }
                    }

                    if(!alreadyExists)
                    this->kernels.push_back(k);
                }
                else
                    this->kernels.push_back(k);
            };

            Kernel3x3 k = Kernel3x3(input, x + ox, y + oy);
            addToKernel(k);

            if(generateTransformations)
            {
                Kernel3x3 k90 = k.rotateCCW();
                Kernel3x3 k180 = k90.rotateCCW();
                Kernel3x3 k270 = k180.rotateCCW();
                Kernel3x3 kx = k.reflectX();
                Kernel3x3 k90x = k90.reflectX();
                Kernel3x3 k180x = k180.reflectX();
                Kernel3x3 k270x = k270.reflectX();

                addToKernel(k90);
                addToKernel(k180);
                addToKernel(k270);
                addToKernel(kx);
                addToKernel(k90x);
                addToKernel(k180x);
                addToKernel(k270x);
            }
        }
    }

    this->numKernels = kernels.size();
    this->allowedKernels = new bool[this->numKernels];

    std::cout << "Num kernals: " << this->kernels.size() << std::endl;

    matches = new bool[this->numKernels * this->numKernels * 4];
    for(int i = 0; i < this->numKernels; i++)
    {
        for(int j = 0; j < this->numKernels; j++)
        {
            MATCH(i, j, 0) = this->kernels[i].match(&this->kernels[j], offsets[0].x, offsets[0].y);
            MATCH(i, j, 1) = this->kernels[i].match(&this->kernels[j], offsets[1].x, offsets[1].y);
            MATCH(i, j, 2) = this->kernels[i].match(&this->kernels[j], offsets[2].x, offsets[2].y);
            MATCH(i, j, 3) = this->kernels[i].match(&this->kernels[j], offsets[3].x, offsets[3].y);
        }
    }

    // Generate our initial state
    for(int i = 0; i < this->N2; i++)
        this->output.push_back(PixelState(&this->rand, this->kernels.size()));

    this->outputImage = new Bitmap(N, N);

    this->outputImage->fillFrom(Bitmap::DEFAULT_BITMAP, 8);
}

WFC_Image::~WFC_Image()
{
    for(int i = 0; i < this->N2; i++)
    {
        delete this->output[i].p;
        this->output[i].p = nullptr;
    }

    delete this->allowedKernels;
    delete this->outputImage;
}

void WFC_Image::resizeOutput(int N)
{
    for(int i = 0; i < this->N2; i++)
    {
        delete this->output[i].p;
        this->output[i].p = nullptr;
    }
    this->output.clear();

    this->N = N;
    this->N2 = N * N;

    for(int i = 0; i < this->N2; i++)
        this->output.push_back(PixelState(&this->rand, this->kernels.size()));

    delete this->outputImage;
    this->outputImage = new Bitmap(N, N);
    this->outputImage->fillFrom(Bitmap::DEFAULT_BITMAP, 8);
}

void WFC_Image::clear(int reseed)
{
    if(reseed)
        this->seed = reseed;
    
    this->rand.seed(this->seed);

    for(int i = 0; i < this->N2; i++)
        this->output[i].clear();
    
    this->outputImage->fillFrom(Bitmap::DEFAULT_BITMAP, 8);

    this->finished = false;
    this->error = false;
}

#define E_MAX 1000000.0
bool WFC_Image::step()
{
    if(this->finished)
        return false;

    // Find the output pixel with the lowest entropy (that isn't already determined or impossible)
    double e_best = E_MAX;
    int i_best;

    for(int i = 0; i < this->N2; i++)
    {
        // If already decided or impossible, ignore
        if(this->output[i].collapsed || this->output[i].impossible)
            continue;

        double e = this->output[i].entropy();

        if(e < e_best)
        {
            // New best entropy, update
            e_best = e;
            i_best = i;
        }
    }

    if(e_best == E_MAX)
    {
        // We're done, everything is already collapsed or impossible
        this->finished = true;
        std::cout << "Finished!" << std::endl;

        // Do some error correction
        for(int i = 0; i < this->N2; i++)
        {
            if(this->output[i].impossible)
            {
                int x = i % this->N;
                int y = i / this->N;

                if(this->errorCorrectionMode == 1)
                {
                    this->outputImage->setPixel(x, y, ColorRGBA(255, 0, 0, 255));
                    continue;
                }

                int dir = this->rand.range(0, 3);
                for(int j = 0; j < 4; j++)
                {
                    int idx = (j + dir) % 4;
                    int ox = x + offsets[idx].x;
                    int oy = y + offsets[idx].y;

                    if(this->outputImage->getPixel(ox, oy).a == 255)
                    {
                        std::cout << "Setting (" << x << ", " << y <<  ") -> (" << ox << ", " << oy << ")" << std::endl; 
                        this->outputImage->setPixel(x, y, this->outputImage->getPixel(ox, oy));
                        break;
                    }
                }
            }
        }

        std::cout << this->output[22 * this->N + 1].impossible << std::endl;
        std::cout << this->outputImage->getPixel(1, 22) << std::endl;

        return true;
    }

    // Get our x and y
    int x = i_best % this->N;
    int y = i_best / this->N;

    // Collapse this cell to one of it's options at random
    this->output[i_best].collapse();

    if(this->output[i_best].impossible)
    {
        this->finished = true;
        this->error = true;
        std::cout << "Got error!" << std::endl;
        std::cout << "Error cell: (" << x << ", " << y << ")" << std::endl;
        std::cout << "Neighbor cell patterns are: " << std::endl;

        for(int oy = -1; oy <= 1; oy++)
            for(int ox = -1; ox <= 1; ox++)
                if(x + ox >= 0 && y + oy >= 0 && x + ox < this->N && y + oy < this->N)
                    if(this->output[(y + oy) * this->N + x + ox].collapsed)
                    {
                        std::cout << "(" << (x + ox) << ", " << (y + oy) << ") - " << this->output[(y + oy) * this->N + x + ox].index << ":" << std::endl;
                        std::cout << this->kernels[this->output[(y + oy) * this->N + x + ox].index].toString();
                    }

        return false;
    }

    this->currX = x;
    this->currY = y;

    // While we have changes to propagate, loop
    this->numCollapsedThisStep = 1;
    this->numPropThisStep = 0;
    this->needsPropagation.push({x, y});
    while(!this->needsPropagation.empty())
    {
        auto coords = this->needsPropagation.front();
        this->needsPropagation.pop();
        if(this->output[coords.second * this->N + coords.first].statesLeft == 0)
        {
            this->output[coords.second * this->N + coords.first].impossible = true;
            continue;
        }
        this->propagate(coords.first, coords.second);
        this->numPropThisStep += 1;
    }
    
    // Fully propagated, look for cells we can collapse
    for(int i = 0; i < output.size(); i++)
    {
        if(output[i].statesLeft == 1 && !output[i].collapsed)
        {
            output[i].collapse(false);
            this->numCollapsedThisStep += 1;
        }
    }

    this->avgCollapsedPerStep = this->avgCollapsedPerStep * 0.9 + this->numCollapsedThisStep;
    this->avgPropPerStep = this->avgPropPerStep * 0.9 + this->numPropThisStep;

    // Update the output image accordingly
    for(int i = 0; i < this->output.size(); i++)
    {
        if(this->output[i].collapsed)
        {
            ColorRGBA color = this->kernels[this->output[i].index].getCenter();
            this->outputImage->setPixel(i % this->N, i / this->N, color);
        }
    }

    // We're done for this step
    return true;
}

#define PROP_DIST(x, y, ox, oy) (abs(x - ox) + abs(y - oy))

void WFC_Image::propagate(int x, int y)
{
    const PixelState ourState = this->output[y*this->N + x];

    // Propagate to all of our neighbors
    for(int oi = 0; oi < 4; oi++)
    {
        int ox = x + offsets[oi].x;
        int oy = y + offsets[oi].y;

        double dist = sqrt((ox - this->currX)*(ox - this->currX) + (oy - this->currY)*(oy - this->currY));
        if(dist > maxDistance)
            continue;

        // Ignore out of bounds pixels
        if(this->wrapOutput)
        {
            ox = ox % this->N;
            if(ox < 0) ox += this->N;
            oy = oy % this->N;
            if(oy < 0) oy += this->N;
        }
        else if(ox < 0 || ox >= this->N || oy < 0 || oy >= this->N) continue;

        // Get our neighbor
        PixelState * ps = &this->output[(oy)*this->N + ox];

        // Ignore if our neighbor is already decided
        if(ps->collapsed || ps->impossible) continue;
            
        // Initialize to false
        for(int i = 0; i < this->numKernels; i++)
            this->allowedKernels[i] = false;

        // For all of our kernels, check which kernels our neighbor could be
        for(int i = 0; i < ourState.np; i++)
        {
            if(!ourState.p[i]) continue;

            for(int j = 0; j < this->numKernels; j++)
            {
                // Does our possible state match with this kernel?
                // If yes, allow it in our neighbor
                allowedKernels[j] = allowedKernels[j] || MATCH(i, j, oi);
            }
        }

        // We have our allowed kernals, time to update our neighbor
        bool changedNeighbor = false;   // For logging, check if this neighbor changed
        for(int i = 0; i < this->numKernels; i++)
        {
            // What is our neighbor's current pixel state?
            bool curr = ps->p[i];

            // If this kernel is not allowed, but was previously...
            if(!allowedKernels[i] && curr)
            {
                // Ban it - remove the possiblity
                ps->p[i] = false;
                ps->statesLeft -= 1;
                changedNeighbor = true;
            }
        }

        // If we changed our neighbor, we need to propagate its changes to its neighbors
        if(changedNeighbor)
        {   
            this->needsPropagation.push({ox, oy});
        }
    }
}

std::string WFC_Image::getDebugOutput()
{
    std::stringstream ss;
    for(int y = 0; y < this->N; y++)
    {
        for(int x = 0; x < this->N; x++)
        {
            if(x != 0)
                ss << " | ";
            
            if(this->output[y*this->N + x].collapsed)
                ss << "XX";
            else
            {
                int e = this->output[y*this->N + x].statesLeft;
                if(e == 0)
                    ss << "__";
                else
                    ss << std::setfill('0') << std::setw(2) << e;
            }
        }
        ss << std::endl;
    }
    
    return ss.str();
}