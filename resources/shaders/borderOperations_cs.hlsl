RWTexture2D<float4> tex;


bool isAtProbeBorder(uint2 pos, out bool isAtProbeX, out bool isAtProbeY)
{
    int gap = 2 + PROBE_SIDE_LENGTH;
    isAtProbeX = pos.x % gap == 0 || (pos.x-1) % gap == 0;
    isAtProbeY = pos.y % gap == 0 || (pos.y-1) % gap == 0;
    return isAtProbeX || isAtProbeY;
}

uint dstToMirrored(uint pos)
{
    uint gap = 2 + PROBE_SIDE_LENGTH;
    uint probe = (pos-2) / gap;
    uint startPoint = (probe * gap)+2;
    float midPoint = startPoint + (PROBE_SIDE_LENGTH-1) / 2.0;
    float dst = midPoint - pos;
    return dst * 2;
}



[numthreads(8,8,1)]
void mainWriteOnesToBorder(uint3 groupId : SV_GroupID,
        uint3 threadInGroup : SV_GroupThreadID)
{
    uint2 texelPos = uint2(
        groupId.x * 8 + threadInGroup.x,
        groupId.y * 8 + threadInGroup.y
    );

    uint2 dim; 
    tex.GetDimensions(dim.x,dim.y);

    if(texelPos.x >= dim.x || texelPos.y >= dim.y)
        return;

    bool x,y;

    if(isAtProbeBorder(texelPos,x,y))
    {
        tex[texelPos] = float4(1,1,1,1);
    }
}

[numthreads(8,8,1)]
void mainDuplicateProbeEdges(uint3 groupId : SV_GroupID,
        uint3 threadInGroup : SV_GroupThreadID)
{
    uint2 texelPos = uint2(
        groupId.x * 8 + threadInGroup.x,
        groupId.y * 8 + threadInGroup.y
    );

    bool isAtBorderX, isAtBorderY;

    if(isAtProbeBorder(texelPos,isAtBorderX,isAtBorderY))
    {
        if(isAtBorderX && isAtBorderY)
        {
            float4 texelToCopy = tex.Load(int3(texelPos.x + PROBE_SIDE_LENGTH,texelPos.y + PROBE_SIDE_LENGTH,0));
            tex[texelPos] = texelToCopy;
            return;
        }

        if(isAtBorderX)
        {
            uint dst = dstToMirrored(texelPos.y);
            tex[texelPos] = tex[uint2(texelPos.x,texelPos.y + dst)];
            return;
        }

        if(isAtBorderY)
        {
            uint dst = dstToMirrored(texelPos.x);
            tex[texelPos] = tex[uint2(texelPos.x + dst, texelPos.y)];
            return;
        }
    }
}




