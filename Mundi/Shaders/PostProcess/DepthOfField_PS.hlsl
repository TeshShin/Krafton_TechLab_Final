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

cbuffer DepthOfFieldCB : register(b2)
{
    
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

float GetLinearDepth(float NDCDepth)
{
    float ViewZ = -Far * Near / (NDCDepth * (Far - Near) - Far);
    return saturate((ViewZ - Near) / (Far - Near));
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    float FocusDepth = 10.0f;
    float FocusRange = 5.0f;
    float Depth = g_DepthTex.Sample(g_LinearClampSample, input.texCoord).r;
    float LinearDepth = GetLinearDepth(Depth);
    float4 SceneColor = g_SceneColorTex.Sample(g_LinearClampSample, input.texCoord);
    
    return float4(SceneColor, SceneColor, SceneColor, 1);
}
