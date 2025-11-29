#pragma once

#include "BoxElem.h"

USTRUCT()
struct FKAggregateGeom
{
    TArray<FKBoxElem> BoxElems;

    FKAggregateGeom()
    {
    }

    FKAggregateGeom(const FKAggregateGeom& Other)
    {
        CloneAgg(Other);
    }

    const FKAggregateGeom& operator=(const FKAggregateGeom& Other)
    {
        CloneAgg(Other);
        return *this;
    }

    int32 GetElementCount() const
    {
        return BoxElems.Num(); // + SphereElems.Num() ...
    }

    int32 GetElementCount(EAggCollisionShape Type) const;

    void EmptyElements()
    {
        BoxElems.Empty();
        // ...
    }

private:
    void CloneAgg(const FKAggregateGeom& Other)
    {
        BoxElems = Other.BoxElems;
    }
};
