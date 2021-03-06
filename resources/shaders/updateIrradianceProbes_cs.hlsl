#include "octahedral.hlsl"

RWTexture2D<float4> tex;
#if OUTPUT_IRRADIANCE
Texture2D rayHitRadiance;
#else
Texture2D rayHitLocations;
Texture2D rayHitNormals;
Texture2D rayOrigins;
#endif
Texture2D rayDirections;

struct Values
{
    float depthSharpness;
    float hysteresis;
    float maxDistance;
};

ConstantBuffer<Values> uniforms;

int probeID(float2 texelXY, uint width) {
    int probeWithBorderSide = PROBE_SIDE_LENGTH + 2;
    int probesPerSide = (width - 2) / probeWithBorderSide;
    return int(texelXY.x / probeWithBorderSide) + probesPerSide * int(texelXY.y / probeWithBorderSide);
}

// Compute normalized oct coord, mapping top left of top left pixel to (-1,-1)
float2 normalizedOctCoord(uint2 fragCoord) {
    int probeWithBorderSide = PROBE_SIDE_LENGTH + 2;

    float2 octFragCoord = int2((fragCoord.x - 2) % probeWithBorderSide, (fragCoord.y - 2) % probeWithBorderSide);
    // Add back the half pixel to get pixel center normalized coordinates
    return (float2(octFragCoord) + float2(0.5f,0.5f))*(2.0f / float(PROBE_SIDE_LENGTH)) - float2(1.0f, 1.0f);
}


float square(float f)
{
    return f * f;
}

// This is basically from the supplemental material from the paper 
// Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields
[numthreads(8,8,1)]
void main(uint3 groupId : SV_GroupID,
        uint3 threadInGroup : SV_GroupThreadID)
{
    float epsilon = 1e-6;
    uint2 texelPos = uint2(
        groupId.x * 8 + threadInGroup.x,
        groupId.y * 8 + threadInGroup.y
    );

    float4 result = float4(0,0,0,0);
    uint2 dim;
    tex.GetDimensions(dim.x,dim.y);

    if(texelPos.x >= dim.x -1 || texelPos.y >= dim.y -1 || texelPos.x < 1 || texelPos.y < 1) return;
    int relativeProbeID = probeID(texelPos.xy, dim.x);

    if (relativeProbeID == -1) {
        result = float4(0.0f,0.f,0.f,0.f);
        return;
    }

    float3 texelDirection = octDecode(normalizedOctCoord(texelPos));
    const float energyConservation = 0.95;

    // For each ray
	for (int r = 0; r < RAYS_PER_PROBE; ++r) {
		int3 C = int3(r, relativeProbeID,0);
		float3 rayDirection    = rayDirections.Load(C).xyz;
#if OUTPUT_IRRADIANCE
        float3  rayHitRadiance_  = rayHitRadiance.Load(C).xyz * energyConservation; // Energy is conserved in sampleIrradiance field. Is it needed here?
#else
		float3  rayHitLocation  = rayHitLocations.Load(C).xyz;

        float3 probeLocation = rayOrigins.Load(C).xyz;
        // Will be zero on a miss
		float3 rayHitNormal    = rayHitNormals.Load(C).xyz;

        rayHitLocation += rayHitNormal * 0.01f;

		float rayProbeDistance = min(uniforms.maxDistance, length(probeLocation - rayHitLocation));
        
        // Detect misses and force depth
		if (dot(rayHitNormal, rayHitNormal) < epsilon) {
            rayProbeDistance = uniforms.maxDistance;
        }
#endif


#if OUTPUT_IRRADIANCE
        float weight = max(0.0, dot(texelDirection, rayDirection));
#else
        float weight = pow(max(0.0, dot(texelDirection, rayDirection)), uniforms.depthSharpness);
#endif
        if (weight >= epsilon) {
            // Storing the sum of the weights in alpha temporarily
#               if OUTPUT_IRRADIANCE
            result += float4(rayHitRadiance_ * weight, weight);
#               else
            result += float4(rayProbeDistance * weight,
                square(rayProbeDistance) * weight,
                0.0,
                weight);
#               endif
        }
		
	}

    if (result.w > epsilon) {
        result.xyz /= result.w;
        //result.w = 1.0f - uniforms.hysteresis;
    } // if nonzero

    float4 old = tex.Load(int3(texelPos,0));
    tex[texelPos] = lerp(old,result,1.0f-uniforms.hysteresis);
}
