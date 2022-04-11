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

// I have to declare as floats and not float3 because float3 is aligned to a float4 thus adding 1 padding which the Host Vertex definition doesn't reflect.
struct Vertex
{
    float posX,posY,posZ;
    //float padding;
    float normalX,normalY,normalZ;
    //float padding2;
    float uvX,uvY;
    //float padding3,padding4;
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
    float4x4 mat;
};

struct ObjectAttrs
{
    float4x4 normalMat;
    uint FirstIndex;
    uint FirstVertex;
    uint MaterialId;
    uint padding;
};

struct Material
{
    uint albedo;
    uint normal;
    uint roughMetal;
    uint emissive;
};


#endif 