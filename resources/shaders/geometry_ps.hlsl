struct PSInput
{
    float4 pos : SV_POSITION;
    float4 wpos : WORLD_POS;
    float3 normal : NORMAL;
    float2 uv : TEX_COORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET0;
    float4 Norm : SV_TARGET1;
};

void main(in PSInput PSIn, out PSOutput PSOut)
{
    PSOut.Color = float4(PSIn.uv,0,0);//float4(1,1,1,1);
    PSOut.Norm = float4(normalize(PSIn.normal.xyz),0);
}
