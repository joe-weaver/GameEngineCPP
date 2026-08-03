#pragma once
#include <string>

class Resource
{
protected:
    std::string filepath;
    bool loaded = false;
    bool error = false;

    Resource(std::string filepath)
    {
        this->filepath = std::string(RESOURCE_DIR) + std::string("/") + filepath;
    }

    ~Resource()
    {
        if(loaded)
        {
            unload();
        }
    }

    void finishLoad(bool success)
    {
        if(success)
        {
            loaded = true;
            error = false;
        }
        else
        {
            loaded = false;
            error = true;
        }
    }

    void finishUnload()
    {
        loaded = false;
    }

public:
    virtual void load() {};
    virtual void unload() {};
};