#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "resource_manifest.h"

class ResourceManager
{
private:
    static inline const EmbeddedResource * findResource(const std::string & filepath);

public:
    static std::string loadTextFile(const std::string & filepath);

    static const unsigned char * loadDataFile(const std::string & filepath, int * size);

    static unsigned char * loadImage(const std::string & filepath, int * width, int * height, int * numChannels);
    static void unloadImage(unsigned char * data);
};

class Resource
{
protected:
    std::string filepath;
    bool loaded = false;
    bool error = false;

    Resource(std::string filepath)
    {
        this->filepath = filepath;
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