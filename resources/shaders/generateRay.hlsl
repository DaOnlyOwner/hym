#ifndef __GEN_RAYS__
#define __GEN_RAYS__

#include "defs.hlsl"
#include "helpers.hlsl"

 float radicalInverse_VdC(uint bits) {
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
 }

  float2 hammersley2d(uint i, uint N) {
     return float2(float(i)/float(N), radicalInverse_VdC(i));
 }

 float3 hemisphereSample_uniform(float u, float v) {
     float phi = v * 2.0 * M_PI;
     float cosTheta = 1.0 - u;
     float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
     return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
 }
    
float3 uniform_sample_hemisphere(float u, float v, float m=1)
{
    float theta = acos(pow(1 - u, 1 / (1 + m)));
    float phi = 2 * M_PI * v;

    float x = sin(theta) * cos(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(theta);

    return float3(x,y,z);
}

Ray generateRay(uint2 coord, in float3x3 randomOrientation, int raysPerProbe, int minDst)
{
    uint probeId = coord.y;
    uint rayId = coord.x;
    float rayMinDst = minDst;
    Ray r;
    r.direction = normalize(float3(mul(sphericalFibonacci(rayId,raysPerProbe),randomOrientation)));
    r.origin = float4(probeLocation(probeId),rayMinDst);
    return r;
}
#endif