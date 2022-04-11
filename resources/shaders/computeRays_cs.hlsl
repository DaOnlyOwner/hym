#include "defs.hlsl"
#include "generateRay.hlsl"
#include "traceRay.hlsl"
#include "sampleIrradianceField.hlsl"


RWTexture2D<float4> rayDirections;
RWTexture2D<float4> rayHitLocations;
RWTexture2D<float4> rayHitRadiance;
RWTexture2D<float4> rayHitNormals;
RWTexture2D<float4> rayOrigins;
ConstantBuffer<RandomOrientation> randomOrientation;
ConstantBuffer<Sun> sun;



[numthreads(8,8,1)]
void main(uint3 groupId : SV_GroupID,
        uint3 threadInGroup : SV_GroupThreadID)
{
    uint2 texelPos = uint2(
        groupId.x * 8 + threadInGroup.x,
        groupId.y * 8 + threadInGroup.y
    );

    uint2 dim;
    rayDirections.GetDimensions(dim.x,dim.y);
    if(texelPos.x >= dim.x || texelPos.y >= dim.y) return;

    
    Ray r = generateRay(texelPos,(float3x3)randomOrientation.mat,L.raysPerProbe,L.minRayDst);
    HitInfo info;
    bool hit = traceRay(r,info,tlas);
    float3 allLight = float3(0,0,0);
    int lit = 0;
    if(hit)
    {
        float3 viewVec = normalize(r.origin.xyz-info.wsHitpoint);
        float3 indirectL = sampleIrradianceField(info.wsHitpoint,info.wsN,L.energyConservation,viewVec);
        Ray shadowRay;
        shadowRay.direction = -sun.direction;
        shadowRay.origin = float4(info.wsHitpoint,0.00 );
        bool sHit = traceRaySimple(shadowRay,tlas);
        lit = !sHit;//sHit ? 0 : 1;
        float3 directL = max(dot(-sun.direction,info.wsN),0.0) * sun.color * lit;
        allLight = (directL + indirectL) * info.color;
    }
    //float tw = L.depthProbeSideLength;
    float d = max(dot(-sun.direction,info.wsN),0);
    rayDirections[texelPos] = float4(r.direction,0);
    rayHitLocations[texelPos] = hit?float4(info.wsHitpoint,0):float4(0,0,0,0);
    rayHitRadiance[texelPos] = float4(allLight,0);
    rayHitNormals[texelPos] = hit?float4(info.wsN,0):float4(0,0,0,0);
    rayOrigins[texelPos] = r.origin;
}





