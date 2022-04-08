#include "traceRaySimple.hlsl"
#include "sampleIrradianceField.hlsl"

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEX_COORD;
};

struct View
{
    float4x4 VPInv;
    float3 eyePos;
    float padding;
};

ConstantBuffer<View> view;
ConstantBuffer<Sun> sun;
Texture2D GBuffer_Albedo;
Texture2D GBuffer_Normal;
Texture2D GBuffer_Depth;

// https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial22_HybridRendering/assets/Utils.fxh
float3 ScreenPosToWorldPos(float2 ScreenSpaceUV, float Depth, float4x4 ViewProjInv)
{
	float4 PosClipSpace;
    PosClipSpace.xy = ScreenSpaceUV * float2(2.0, -2.0) + float2(-1.0, 1.0);
    PosClipSpace.z = Depth;
    PosClipSpace.w = 1.0;
    float4 WorldPos = mul(PosClipSpace, ViewProjInv);
    return WorldPos.xyz / WorldPos.w;
}

float4 main(in PSInput PSIn, float4 pixelPos : SV_POSITION) : SV_TARGET
{
    uint2 dim;
    GBuffer_Albedo.GetDimensions(dim.x, dim.y);
    int3 texelPos = int3(
        PSIn.UV.x * dim.x,
        (1-PSIn.UV.y) * dim.y,
        0
    );


    float3 albedo = GBuffer_Albedo.Load(texelPos).xyz;
    float3 normal = GBuffer_Normal.Load(texelPos).xyz;
    float depth = GBuffer_Depth.Load(texelPos).r;
    float3 pos = ScreenPosToWorldPos(PSIn.Pos.xy / dim,depth,view.VPInv);
    Ray r;
    r.origin = float4(pos,0.1);
    r.direction	= -sun.direction;
    bool hit = traceRaySimple(r,tlas);
    int shadow = hit ? 0 : 1;
    
    float3 forward = normalize(pos-view.eyePos);
    float3 NdotL = max(dot(normal.xyz,-sun.direction),0.0);
    float3 direct = shadow * NdotL * sun.color;


    float3 viewVec = normalize(view.eyePos-pos);
    float3 indirect = sampleIrradianceField(pos,normal,1.0,viewVec);
    float3 allLight = (direct + indirect) * albedo.xyz;

    return float4(allLight,0);
}


