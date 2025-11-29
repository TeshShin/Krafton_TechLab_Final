#include "pch.h"
#include "AggregateGeom.h"

int32 FKAggregateGeom::GetElementCount(EAggCollisionShape Type) const
{
    switch (Type)
    {
    case EAggCollisionShape::Box:
        return BoxElems.Num();
    case EAggCollisionShape::Convex:
        // return ConvexElems.Num();
    case EAggCollisionShape::Sphyl:
        // return SphylElems.Num();
    case EAggCollisionShape::Sphere:
        // return SphereElems.Num();
    case EAggCollisionShape::TaperedCapsule:
        // return TaperedCapsuleElems.Num();
    case EAggCollisionShape::LevelSet:
        // return LevelSetElems.Num();
    default:
        return 0;
    }
}
