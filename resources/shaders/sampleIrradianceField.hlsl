#ifndef __SAMPLE_IRR_FIELD__
#define __SAMPLE_IRR_FIELD__

#include "defs.hlsl"
#include "octahedral.hlsl"
#include "helpers.hlsl"

Texture2D irradianceTex;
SamplerState irradianceTex_sampler;
Texture2D weightTex;
SamplerState weightTex_sampler;

// This is basically from the supplemental material from the paper 
// Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields

float square(float f)
{
    return f * f;
}

float3 square(float3 f)
{
    return float3(f.x * f.x, f.y * f.y, f.z * f.z);
}

float2 textureCoordFromDirection(float3 dir, int probeIndex, int fullTextureWidth, int fullTextureHeight, int probeSideLength) {
    float2 normalizedOctCoord = octEncode(normalize(dir));
    float2 normalizedOctCoordZeroOne = (normalizedOctCoord + float2(1.0f,1.0f)) * 0.5f;

    // Length of a probe side, plus one pixel on each edge for the border
    float probeWithBorderSide = (float)probeSideLength + 2.0f;

    float2 octCoordNormalizedToTextureDimensions = (normalizedOctCoordZeroOne * (float)probeSideLength) / float2((float)fullTextureWidth, (float)fullTextureHeight);

    int probesPerRow = (fullTextureWidth - 2) / (int)probeWithBorderSide;

    // Add (2,2) back to texCoord within larger texture. Compensates for 1 pix 
    // border around texture and further 1 pix border around top left probe.
    float2 probeTopLeftPosition = float2((probeIndex % probesPerRow) * probeWithBorderSide,
        ((probeIndex / probesPerRow) * probeWithBorderSide)) + float2(2.0f, 2.0f);

    float2 normalizedProbeTopLeftPosition = float2(probeTopLeftPosition) / float2((float)fullTextureWidth, (float)fullTextureHeight);

    return float2(normalizedProbeTopLeftPosition + octCoordNormalizedToTextureDimensions);
}



float3 sampleIrradianceField(float3 wsPosition, float3 wsN, float energyPreservation, in float3 viewVec)
{  
    // View vector
    float3 w_o = viewVec;

    // Glossy coefficient in BSDF (this code unpacks
    // G3D::UniversalBSDF's encoding)
    //float4  F0 = texelFetch(gbuffer_GLOSSY_buffer, C, 0);

    //float glossyExponent = smoothnessToBlinnPhongExponent(F0.a);

    float cos_o = dot(wsN, w_o);

    // Incoming reflection vector
    float3 w_mi = normalize(wsN * (2.0 * cos_o) - w_o);

    //E_glossyIndirect = computeGlossyEnvironmentMapLighting(w_mi, (F0.a == 1.0), glossyExponent, false);

    int3 baseGridCoord_ = baseGridCoord(wsPosition);
    float3 baseProbePos = gridCoordToPosition(baseGridCoord_);
    float3 sumIrradiance = float3(0,0,0);
    float sumWeight = 0.0;

    // alpha is how far from the floor(currentVertex) position. on [0, 1] for each axis.
    float3 alpha = clamp((wsPosition - baseProbePos) / L.probeStep, float3(0,0,0), float3(1,1,1));

    // Iterate over adjacent probe cage
    for (int i = 0; i < 8; ++i) {
        // Compute the offset grid coord and clamp to the probe grid boundary
        // Offset = 0 or 1 along each axis
        int3  offset = int3(i, i >> 1, i >> 2) & int3(1,1,1);
        int3  probeGridCoord = clamp(baseGridCoord_ + offset, int3(0,0,0), int3(L.probeCounts - 1));
        int p = gridCoordToProbeIndex(probeGridCoord);

        // Make cosine falloff in tangent plane with respect to the angle from the surface to the probe so that we never
        // test a probe that is *behind* the surface.
        // It doesn't have to be cosine, but that is efficient to compute and we must clip to the tangent plane.
        float3 probePos = gridCoordToPosition(probeGridCoord);

        // Bias the position at which visibility is computed; this
        // avoids performing a shadow test *at* a surface, which is a
        // dangerous location because that is exactly the line between
        // shadowed and unshadowed. If the normal bias is too small,
        // there will be light and dark leaks. If it is too large,
        // then samples can pass through thin occluders to the other
        // side (this can only happen if there are MULTIPLE occluders
        // near each other, a wall surface won't pass through itself.)
        float3 probeToPoint = wsPosition - probePos + (wsN + 3.0 * w_o) * L.normalBias;
        float3 dir = normalize(-probeToPoint);

        // Compute the trilinear weights based on the grid cell vertex to smoothly
        // transition between probes. Avoid ever going entirely to zero because that
        // will cause problems at the border probes. This isn't really a lerp. 
        // We're using 1-a when offset = 0 and a when offset = 1.
        float3 trilinear = lerp(1.0 - alpha, alpha, offset);
        float weight = 1.0;

        // Clamp all of the multiplies. We can't let the weight go to zero because then it would be 
        // possible for *all* weights to be equally low and get normalized
        // up to 1/n. We want to distinguish between weights that are 
        // low because of different factors.

        // Smooth backface test
        {
            // Computed without the biasing applied to the "dir" variable. 
            // This test can cause reflection-map looking errors in the image
            // (stuff looks shiny) if the transition is poor.
            float3 trueDirectionToProbe = normalize(probePos - wsPosition);

            // The naive soft backface weight would ignore a probe when
            // it is behind the surface. That's good for walls. But for small details inside of a
            // room, the normals on the details might rule out all of the probes that have mutual
            // visibility to the point. So, we instead use a "wrap shading" test below inspired by
            // NPR work.
            // weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

            // The small offset at the end reduces the "going to zero" impact
            // where this is really close to exactly opposite
            weight *= square(max(0.0001, (dot(trueDirectionToProbe, wsN) + 1.0) * 0.5)) + 0.2;
        }
        
        // Moment visibility test
        {
            float2 texCoord = textureCoordFromDirection(-dir,
                p,
                L.depthTextureWidth,
                L.depthTextureHeight,
                L.depthProbeSideLength);

            float distToProbe = length(probeToPoint);

            float2 temp = weightTex.SampleLevel(weightTex_sampler,texCoord,0).rg;
            float mean = temp.x;
            float variance = abs(temp.x * temp.x - temp.y);

            // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
            // Need the max in the denominator because biasing can cause a negative displacement
            //float f = max(distToProbe - mean, 0.0);
            float chebyshevWeight = variance / (variance + square(max(distToProbe - mean, 0.0)));// + L.chebBias;
                
            // Increase contrast in the weight 
            chebyshevWeight = max(pow(chebyshevWeight,3), 0.0) + L.chebBias;

            weight *= (distToProbe <= mean) ? 1.0 : chebyshevWeight;
        }

        // Avoid zero weight
        weight = max(0.000001, weight);
                 
        float3 irradianceDir = wsN;

        float2 texCoord = textureCoordFromDirection(normalize(irradianceDir),
            p,
            L.irradianceTextureWidth,
            L.irradianceTextureHeight,
            L.irradianceProbeSideLength);

        float3 probeIrradiance = irradianceTex.SampleLevel(irradianceTex_sampler,texCoord,0).rgb;

        // A tiny bit of light is really visible due to log perception, so
        // crush tiny weights but keep the curve continuous. This must be done
        // before the trilinear weights, because those should be preserved.
        const float crushThreshold = 0.2;
        if (weight < crushThreshold) {
            weight *= weight * weight * (1.0 / (crushThreshold * crushThreshold)); 
        }

        // Trilinear weights
        weight *= trilinear.x * trilinear.y * trilinear.z;

        // Weight in a more-perceptual brightness space instead of radiance space.
        // This softens the transitions between probes with respect to translation.
        // It makes little difference most of the time, but when there are radical transitions
        // between probes this helps soften the ramp.
#       if LINEAR_BLENDING == 0
            probeIrradiance = sqrt(probeIrradiance);
#       endif
        
        sumIrradiance += weight * probeIrradiance;
        sumWeight += weight;
    }

    float3 netIrradiance = sumIrradiance / sumWeight;

    // Go back to linear irradiance
#   if LINEAR_BLENDING == 0
        netIrradiance = square(netIrradiance);
#   endif
    netIrradiance *= energyPreservation;

    return 0.5 * M_PI * netIrradiance;

}
#endif