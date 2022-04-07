#ifndef __TRACE_RAY_SIMPLE__
#define __TRACE_RAY_SIMPLE__

#include "defs.hlsl"

RaytracingAccelerationStructure tlas;

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