#ifndef _HELPERS_
#define _HELPERS_

#include "defs.hlsl"

ConstantBuffer<LightField> L;

/**  Generate a spherical fibonacci point

    http://lgdv.cs.fau.de/publications/publication/Pub.2015.tech.IMMD.IMMD9.spheri/

    To generate a nearly uniform point distribution on the unit sphere of size N, do
    for (float i = 0.0; i < N; i += 1.0) {
        float3 point = sphericalFibonacci(i,N);
    }

    The points go from z = +1 down to z = -1 in a spiral. To generate samples on the +z hemisphere,
    just stop before i > N/2.

*/
float3 sphericalFibonacci(float i, float n) {
    const float PHI = sqrt(5) * 0.5 + 0.5;
#   define madfrac(A, B) ((A)*(B)-floor((A)*(B)))
    float phi = 2.0 * M_PI * madfrac(i, PHI - 1);
    float cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sinTheta = sqrt(saturate(1.0 - cosTheta * cosTheta));

    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);

#   undef madfrac
}

/*
    Compute 3D worldspace position from gridcoord based on starting
    location and distance between probes.
*/
float3 gridCoordToPosition(int3 c) {
    return L.probeStep * float3(c) + L.probeStartPosition;
}


int findMSB(int n)
{
    return (int)log2(n);
}

/*
    Compute the grid coordinate of the probe from the index
*/
int3 probeIndexToGridCoord(int index) {
    // Slow, but works for any # of probes
    int3 iPos;
    /*iPos.x = index % L.probeCounts.x;
    iPos.y = (index % (L.probeCounts.x * L.probeCounts.y)) / L.probeCounts.x;
    iPos.z = index / (L.probeCounts.x * L.probeCounts.y);
    */

    // Assumes probeCounts are powers of two.
    // Saves ~10ms compared to the divisions above
    // Precomputing the MSB actually slows this code down substantially
    
    iPos.x = index & (L.probeCounts.x - 1);
    iPos.y = (index & ((L.probeCounts.x * L.probeCounts.y) - 1)) >> findMSB(L.probeCounts.x);
    iPos.z = index >> findMSB(L.probeCounts.x * L.probeCounts.y);
    
    return iPos;
}

/*
    Compute the 3D probe location in world space from the probe index
*/
float3 probeLocation(int index) {
    return gridCoordToPosition(probeIndexToGridCoord(index));
}

int3 baseGridCoord(float3 X) {
    return clamp(int3((X - L.probeStartPosition) / L.probeStep),
                int3(0, 0, 0), 
                int3(L.probeCounts) - int3(1, 1, 1));
}

int gridCoordToProbeIndex(in int3 probeCoords) {
    return int(probeCoords.x + probeCoords.y * L.probeCounts.x + probeCoords.z * L.probeCounts.x * L.probeCounts.y);
}

#endif