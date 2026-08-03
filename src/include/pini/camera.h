#pragma once

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#define CLARITY_FACTOR 0.000f

class Camera
{
private:
    vec2 position; // The center of the camera
    vec2 size;  // TODO: For now, I assume size == windowSize
    float zoomFactor = 1;

public:
    Camera(vec2 position, vec2 size) : position(position), size(size) {}

    float getZoomFactor() const { return zoomFactor; }

    vec2 screenToWorld(vec2 screenPos) const
    {
        vec2 normalized = (screenPos / size) - vec2(0.5f, 0.5f);
        return position + normalized * (size / zoomFactor);
    }

    void move(vec2 delta)
    {
        position += delta;
    }

    void zoom(float factor)
    {
        zoomFactor *= factor;
        
        float intPart;
        if(std::modf(zoomFactor, &intPart) < CLARITY_FACTOR)
            zoomFactor = intPart;
    }

    void zoom(float factor, vec2 focus)
    {
        vec2 originalPosition = screenToWorld(focus);

        zoomFactor *= factor;
        
        float intPart;
        if(std::modf(zoomFactor, &intPart) < CLARITY_FACTOR)
            zoomFactor = intPart;
        
        // Find out how far off we are based on our current zoom factor
        vec2 newPosition = screenToWorld(focus);
        move(originalPosition - newPosition);
    }

    mat4 getProjection() const
    {
        // Maps world coordinates onto the screen
        vec2 topLeft = position - size / zoomFactor / 2.f;
        vec2 bottomRight = position + size / zoomFactor / 2.f;

        return glm::ortho(topLeft.x, bottomRight.x, bottomRight.y, topLeft.y, -1.0f, 1.0f);
    }

    mat4 getView() const
    {
        return mat4(1.0f);
    }
};