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
    int showHitLocations;
    int probeID;
    float indirectInt, directInt,p4;
};

ConstantBuffer<View> view;
ConstantBuffer<Sun> sun;
Texture2D GBuffer_Albedo;
Texture2D GBuffer_Normal;
Texture2D GBuffer_Depth;

Texture2D rayTest;
Texture2D rayHitLocations;


bool isNear(float3 v0, float3 v1, float rad)
{
    float le = length(v0-v1);
    return le < rad;
}

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

float4 main(in PSInput PSIn) : SV_TARGET
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
    int lit = hit ? 0 : 1;

    float3 NdotL = max(dot(normal.xyz,-sun.direction),0.0);
    float3 direct = NdotL * sun.color * lit;

    float3 viewVec = normalize(view.eyePos-pos);
    float3 indirect = sampleIrradianceField(pos,normal,1.0,viewVec);
    
    uint w,h;
    rayHitLocations.GetDimensions(w,h);
    //float3 dummy = (rayHitLocations.Load(int3(0,0,0)) + rayTest.Load(int3(0,0,0))) * 0.00001;
    float3 allLight = (direct * view.directInt + indirect*view.indirectInt) * albedo.xyz;// + dummy;
    bool quit = false;
    //allLight = allLight / (allLight + 1); // Reinhardt tonemap
    if(view.showHitLocations)
    {
        for(int ray = 0; ray<w && !quit; ray++)
        {
            int3 loc = int3(ray,view.probeID,0);
            float3 hitl = rayHitLocations.Load(loc).xyz;
            // if(isNear(pos,orig,0.05))
            // {
            //     allLight = float3(0,1,0);
            // }
            if(isNear(pos,hitl.xyz,0.05))
            {
                float3 col = (rayTest.Load(loc).xyz);
                if(length(col) < 0.995)
                {
                    col = float3(1,0,1);
                }
                allLight = (col+1)*0.5;
                quit=true;
                break;
            }
        }
    }
    return float4(allLight,0);
}


