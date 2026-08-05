#include "wfc.h"

Random WFC_TriColor::PixelState::rand;

bool WFC_TriColor::step()
{
    if(this->finished)
        return false;
        
    std::cout << "step " << this->stepNumber++ << ": " << std::endl;

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

    std::cout << "Collapsing pixel (" << x << ", " << y << ") with e = " << e;
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

    if(p->red) std::cout << " to red";
    else if(p->green) std::cout << " to green";
    else if(p->blue) std::cout << " to blue";

    std::cout << std::endl;

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

