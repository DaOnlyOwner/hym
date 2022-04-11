#include "octahedral.hlsl"

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 wpos : WORLD_POS;
    float3 normal : NORMAL;
    float2 uv : TEX_COORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

struct Uniforms
{
    int probeID;
    int sideLength;
    int debugProbeId;
    int padding1;
};

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

ConstantBuffer<Uniforms> uniforms;

Texture2D irradianceTex;
SamplerState irradianceTex_sampler;

void main(in PSInput PSIn, out PSOutput PSOut)
{
    if(uniforms.debugProbeId == uniforms.probeID)
    {
        PSOut.Color = float4(1,0,1,0);
        return;
    }
    uint w,h;
    irradianceTex.GetDimensions(w,h);
    float2 coord = textureCoordFromDirection(PSIn.normal,uniforms.probeID,w,h,uniforms.sideLength);
    PSOut.Color = irradianceTex.Sample(irradianceTex_sampler,coord);
}