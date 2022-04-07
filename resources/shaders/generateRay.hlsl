#ifndef __GEN_RAYS__
#define __GEN_RAYS__

#include "defs.hlsl"
#include "helpers.hlsl"

Ray generateRay(uint2 coord, in float3x3 randomOrientation, int raysPerProbe)
{
    uint probeId = coord.y;
    uint rayId = coord.x;
    float rayMinDst = 0.8;
    Ray r;
    r.direction = normalize(float3(mul(sphericalFibonacci(rayId,raysPerProbe),randomOrientation)));
    r.origin = float4(probeLocation(probeId),rayMinDst);
    return r;
}
#endif