struct GeometryPassVSPerMeshConstants
{
    float4x4 model;
    float4x4 normal;
    float4x4 MVP;
};

struct VSInput
{
    float3 pos : ATTRIB0;
    //float padding : ATTRIB1;
    float3 normal : ATTRIB1;
    //float padding2 : ATTRIB3;
    //float3 tangent : ATTRIB2;
    //float3 bitangent : ATTRIB3;
    float2 uv : ATTRIB2;
    //float padding3: ATTRIB5;
    //float padding4: ATTRIB6;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 wpos : WORLD_POS;
    float3 normal : NORMAL;
    float2 uv : TEX_COORD;
};

ConstantBuffer<GeometryPassVSPerMeshConstants> geomConst;

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.wpos = mul(float4(VSIn.pos.xyz,1.0),geomConst.model);
    PSIn.pos = mul(float4(VSIn.pos.xyz,1.0),geomConst.MVP);
    PSIn.normal = mul(float4(VSIn.normal.xyz,1.0),geomConst.normal).xyz;
    PSIn.uv = VSIn.uv.xy;
}
