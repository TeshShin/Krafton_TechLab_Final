Texture2D g_DepthTex : register(t0);
Texture2D g_SceneColorTex : register(t1);

SamplerState g_LinearClampSample : register(s0);
SamplerState g_PointClampSample : register(s1);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

cbuffer PostProcessCB : register(b0)
{
    float Near;
    float Far;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
}

cbuffer GaussianCB : register(b2)
{
    float Weight;
    uint Range;
    uint bHorizontal;
    float padding;
}

cbuffer ViewportConstants : register(b10)
{
    // x: Viewport TopLeftX, y: Viewport TopLeftY
    // z: Viewport Width,   w: Viewport Height
    float4 ViewportRect;
    
    // x: Screen Width      (전체 렌더 타겟 너비)
    // y: Screen Height     (전체 렌더 타겟 높이)
    // z: 1.0f / Screen W,  w: 1.0f / Screen H
    float4 ScreenSize;
}

//static const float GaussianKernel[9] =
//{
//    1.0f, 2.0f, 1.0f,
//    2.0f, 4.0f, 2.0f,
//    1.0f, 2.0f, 1.0f,
//};
//static const float2 GaussianUV[9] =
//{
//    float2(-1, -1), float2(0, -1), float2(1, -1),
//    float2(-1,  0), float2(0,  0), float2(1,  0),
//    float2(-1,  1), float2(0,  1), float2(1,  1),
//};

float GetGaussian(float dis)
{
    return exp(-dis * dis / (2 * Weight * Weight));
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    uint TexWidth, TexHeight;
    uint halfRange = Range / 2;
    g_SceneColorTex.GetDimensions(TexWidth, TexHeight);
    float2 InvTexSize = float2(1.0f / TexWidth, 1.0f / TexHeight);
    float3 TotalColor = float3(0, 0, 0);
    float TotalGaussian = 0;
    
    float2 UVDir = float2(0, 0);
    if (bHorizontal)
    {
        UVDir = float2(InvTexSize.x, 0);
    }
    else
    {
        UVDir = float2(0, InvTexSize.y);
    }
    for (int i = -halfRange; i <= halfRange; ++i)
    {
        float CurGaussian = GetGaussian(i);
        float3 SampleColor = g_SceneColorTex.Sample(g_LinearClampSample, input.texCoord + UVDir * i).rgb;
        TotalGaussian += CurGaussian;
        TotalColor += SampleColor;
    }
    float3 FinalColor = TotalColor / TotalGaussian;
    return float4(FinalColor.rgb, 1);
}
