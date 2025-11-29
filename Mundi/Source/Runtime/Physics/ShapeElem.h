#pragma once
#include "UEContainer.h"

// Shape 타입 열거형
enum class EAggCollisionShape : uint8
{
    Sphere,
    Box,
    Sphyl,      // Capsule
    Convex,
    Unknown
};
