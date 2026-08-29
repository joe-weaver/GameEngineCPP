#include "resource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

inline const EmbeddedResource * ResourceManager::findResource(const std::string & filepath)
{
#ifdef USE_EMBEDDED_RESOURCES
    for (const auto & res : getEmbeddedResources())
    {
        if (res.name == filepath) return &res;
    }
#endif
    return nullptr;
}

std::string ResourceManager::loadTextFile(const std::string & filepath)
{
#ifdef USE_EMBEDDED_RESOURCES
    const EmbeddedResource * rsrc = ResourceManager::findResource(filepath);
    if(rsrc == nullptr)
        return std::string();

    return std::string((char *)rsrc->data, rsrc->size);
#else
    if (std::ifstream fin {std::string(RESOURCE_DIR) + std::string("/") + filepath, std::ifstream::in}) {
        std::ostringstream sout;
        sout << fin.rdbuf();
        return sout.str();
    } else {
        return nullptr;
    }
#endif
}

const unsigned char * ResourceManager::loadDataFile(const std::string & filepath, int * size)
{
#ifdef USE_EMBEDDED_RESOURCES
    const EmbeddedResource * rsrc = ResourceManager::findResource(filepath);
    *size = rsrc->size;
    return rsrc->data;
#else
    std::cerr << "NEEDS IMPLEMENTATION" << std::endl;
    return nullptr;
#endif
}

unsigned char * ResourceManager::loadImage(const std::string & filepath, int * width, int * height, int * numChannels)
{
#ifdef USE_EMBEDDED_RESOURCES
    const EmbeddedResource * rsrc = ResourceManager::findResource(filepath);
    if(rsrc)
        return stbi_load_from_memory(rsrc->data, rsrc->size, width, height, numChannels, 4);
    else
        return nullptr;
#else
    std::string filepathInRsrc = std::string(RESOURCE_DIR) + std::string("/") + filepath;
    return stbi_load(filepathInRsrc.c_str(), width, height, numChannels, 4);
#endif
}

void ResourceManager::unloadImage(unsigned char * data)
{
#ifndef USE_EMBEDDED_RESOURCES
    stbi_image_free(data);
#endif
}