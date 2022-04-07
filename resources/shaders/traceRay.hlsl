#ifndef __TRACE_RAY__
#define __TRACE_RAY__

#include "defs.hlsl"

StructuredBuffer<Vertex> vertexBuffer;
StructuredBuffer<uint> indexBuffer;
StructuredBuffer<ObjectAttrs> attrs;
RaytracingAccelerationStructure tlas;

Texture2D albedoTexs[NUM_ALBEDO_TEXS];
SamplerState tex_sampler;
StructuredBuffer<Material> materials;


struct HitInfo
{
    float t;
    float3 wsHitpoint;
    float3 wsN;
    float3 color;

};

bool traceRay(in Ray ray, out HitInfo info, RaytracingAccelerationStructure tlas)
{
    RayDesc rdesc;
    rdesc.Origin = ray.origin.xyz;
    rdesc.Direction = ray.direction;
    rdesc.TMin = ray.origin.w;
    rdesc.TMax = 100000000.0; // TODO: Change this to improve performance

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(tlas,RAY_FLAG_NONE,0xFF,rdesc);

    query.Proceed();
    bool hit = query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
    if(!hit) return false;
    uint instId = query.CommittedInstanceID();
    ObjectAttrs attr = attrs[instId];
    uint primitive = query.CommittedPrimitiveIndex();
    uint3 index = uint3(
        indexBuffer[attr.FirstIndex + primitive * 3],
        indexBuffer[attr.FirstIndex + primitive * 3 + 1],
        indexBuffer[attr.FirstIndex + primitive * 3 + 2]);

    //uint3 index = uint3(primitive * 3,primitive*3+1, primitive*3+2);
    
    Vertex v0 = vertexBuffer[index.x + attr.FirstVertex];
    Vertex v1 = vertexBuffer[index.y + attr.FirstVertex];
    Vertex v2 = vertexBuffer[index.z + attr.FirstVertex];

    float3 bary;
    bary.yz = query.CommittedTriangleBarycentrics();
    bary.x = 1.0 - bary.y - bary.z;

    float2 uv = float2(v0.uv.x, v0.uv.y) * bary.x + 
                float2(v1.uv.x, v1.uv.y) * bary.y +
                float2(v2.uv.x, v2.uv.y) * bary.z;
    
    info.wsN = float3(v0.normal.x,v0.normal.y,v0.normal.z) * bary.x +
                    float3(v1.normal.x, v1.normal.y,v1.normal.z)* bary.y +
                    float3(v2.normal.x, v2.normal.y, v2.normal.z)*bary.z;

    info.wsN = normalize(mul(info.wsN,(float3x3)attr.normalMat));

    info.wsHitpoint = query.WorldRayOrigin() + (query.CommittedRayT() * query.WorldRayDirection());
    info.t = query.CommittedRayT();
    Material mat = materials[attr.MaterialId];
    info.color = albedoTexs[NonUniformResourceIndex(mat.albedo)].SampleLevel(tex_sampler,uv,0).xyz;
    return true;
}

bool traceRaySimple(in Ray ray, RaytracingAccelerationStructure tlas)
{
    RayDesc rdesc;
    rdesc.Origin = ray.origin.xyz;
    rdesc.Direction = ray.direction;
    rdesc.TMin = ray.origin.w;
    rdesc.TMax = 10000000000.;
    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;
    query.TraceRayInline(tlas, RAY_FLAG_NONE,0xFF,rdesc);
    query.Proceed();
    return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

#endif