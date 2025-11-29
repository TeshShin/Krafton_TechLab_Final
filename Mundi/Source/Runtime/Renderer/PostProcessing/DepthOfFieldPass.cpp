#include "pch.h"
#include "DepthOfFieldPass.h"
#include "../SceneView.h"
#include "../../RHI/SwapGuard.h"

const char* FDepthOfFieldPass::DepthOfFieldPSPath = "Shaders/PostProcess/DepthOfField_PS.hlsl";
const char* FDepthOfFieldPass::GaussianBlurPSPath = "Shaders/PostProcess/GaussianBlur_PS.hlsl";
const char* FDepthOfFieldPass::DOF_CalcCOC_PSPath = "Shaders/PostProcess/DOF_CalcCOC_PS.hlsl";

void FDepthOfFieldPass::Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
    //Common
    FSwapGuard Swap(RHIDevice, /*FirstSlot*/0, /*NumSlotsToUnbind*/2);
    RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
    RHIDevice->OMSetBlendState(false);
    ID3D11SamplerState* LinearClampSamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
    ID3D11SamplerState* PointClampSamplerState = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);
    ID3D11SamplerState* Smps[2] = { LinearClampSamplerState, PointClampSamplerState };
    RHIDevice->GetDeviceContext()->PSSetSamplers(0, 2, Smps);
    ECameraProjectionMode ProjectionMode = View->ProjectionMode;
    RHIDevice->SetAndUpdateConstantBuffer(PostProcessBufferType(View->NearClip, View->FarClip, ProjectionMode == ECameraProjectionMode::Orthographic));
   
    
    // 3) 셰이더
    UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>(UResourceManager::FullScreenVSPath);
    UShader* BlitPS = UResourceManager::GetInstance().Load<UShader>(UResourceManager::BlitPSPath);
    UShader* COCPS = UResourceManager::GetInstance().Load<UShader>(DOF_CalcCOC_PSPath);
    UShader* GaussianPS = UResourceManager::GetInstance().Load<UShader>(GaussianBlurPSPath);

    //CalcCOC


    //Blur

    //Composite

    //RTV
    RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);
    //SRV
    ID3D11ShaderResourceView* DepthSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneDepth);
    ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
    ID3D11ShaderResourceView* Srvs[2] = { DepthSRV, SceneSRV };
    //
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, Srvs);
    
    RHIDevice->PrepareShader(FullScreenTriangleVS, COCPS);

    // 7) Draw
    RHIDevice->DrawFullScreenQuad();

    // 8) 확정
    Swap.Commit();





    // 8) 확정
    Swap.Commit();
}
