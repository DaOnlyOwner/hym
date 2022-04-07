#ifndef __DEFS__
#define __DEFS__

struct Ray 
{
    float4 origin;
    float3 direction;
};

#define M_PI 3.1415926535897932384626433832795

struct LightField
{
    int3 probeCounts;
    int raysPerProbe;
    float3 probeStartPosition;
    float normalBias;
    float3 probeStep;
    int irradianceTextureWidth;
    int irradianceTextureHeight;
    float irradianceProbeSideLength;
    int depthTextureWidth;
    int depthTextureHeight;
    float depthProbeSideLength;
    float padding1,padding2,padding3;
};

struct Vertex
{
    float3 pos;
    float padding;
    float3 normal;
    float padding2;
    float2 uv;
    float padding3,padding4;
};

struct Sun
{
    float3 direction;
    float padding;
    float3 color;
    float padding2;
};

struct RandomOrientation
{
    float3x3 mat;
};

struct ObjectAttrs
{
    float4x3 normalMat;
    uint FirstIndex;
    uint FirstVertex;
    uint MaterialId;
};

struct Material
{
    uint albedo;
    uint normal;
    uint roughMetal;
    uint emissive;
};


#endif 