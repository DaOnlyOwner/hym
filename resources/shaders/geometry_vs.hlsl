struct GeometryPassVSPerMeshConstants
{
    float4x4 model;
    float4x4 normal;
    float4x4 MVP;
    uint matIdx;
    float p1,p2,p3;
};

struct VSInput
{
    float3 pos : ATTRIB0;
    float3 normal : ATTRIB1;
    //float3 tangent : ATTRIB2;
    //float3 bitangent : ATTRIB3;
    float2 uv : ATTRIB2;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 wpos : WORLD_POS;
    float3 normal : NORMAL;
    float2 uv : TEX_COORD;
    nointerpolation uint matIdx : MAT_IDX;
};

ConstantBuffer<GeometryPassVSPerMeshConstants> geomConst;

void main(in VSInput VSIn, out PSInput PSIn)
{
    PSIn.wpos = mul(float4(VSIn.pos.xyz,1.0),geomConst.model);
    PSIn.pos = mul(float4(VSIn.pos.xyz,1.0),geomConst.MVP);
    PSIn.normal = mul(float4(VSIn.normal.xyz,1.0),geomConst.normal).xyz;
    PSIn.uv = VSIn.uv.xy;
    PSIn.matIdx = geomConst.matIdx;
}
