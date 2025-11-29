#pragma once
#include  "PostProcessing.h"

class FSceneView;
class FDepthOfFieldPass final : public IPostProcessPass
{
public:
    static const char* DepthOfFieldPSPath;
    static const char* GaussianBlurPSPath;
    static const char* DOF_CalcCOC_PSPath;
public:
    virtual void Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice) override;
};
