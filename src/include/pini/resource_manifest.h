#pragma once
#include <vector>
#include <string>

struct EmbeddedResource {
    std::string name;
    const unsigned char* data;
    unsigned int size;
};

const std::vector<EmbeddedResource>& getEmbeddedResources();