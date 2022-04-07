
struct PSInput
{
    float4 pos : SV_POSITION;
    float4 wpos : WORLD_POS;
    float3 normal : NORMAL;
    float2 uv : TEX_COORD;
    nointerpolation uint matIdx : MAT_IDX;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
    float4 Norm : SV_TARGET1;
};

struct Material
{
    uint albedo;
    uint normal;
    uint roughMetal;
    uint emissive;
};

StructuredBuffer<Material> materials;
Texture2D albedoTexs[NUM_ALBEDO_TEXS];
SamplerState texSampler; 

void main(in PSInput PSIn, out PSOutput PSOut)
{
    Material mat = materials[PSIn.matIdx];
    PSOut.Color = albedoTexs[NonUniformResourceIndex(mat.albedo)].Sample(texSampler,PSIn.uv);//float4(1,1,1,1);
    PSOut.Norm = float4(normalize(PSIn.normal.xyz),0);
}
