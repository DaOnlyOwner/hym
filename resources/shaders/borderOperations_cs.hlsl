RWTexture2D<float4> tex;


bool isAtProbeBorder(uint2 pos, out bool isAtProbeX, out bool isAtProbeY)
{
    int gap = 2 + PROBE_SIDE_LENGTH;
    isAtProbeX = (pos.x % gap == 0) || ((pos.x-1) % gap == 0);
    isAtProbeY = (pos.y % gap == 0) || ((pos.y-1) % gap == 0);
    return isAtProbeX || isAtProbeY;
}

// uint mirrored(uint pos)
// {
//     uint gap = 2 + PROBE_SIDE_LENGTH;
//     uint probe = (pos-2) / gap;
//     uint startPoint = (probe * gap)+2;
//     float midPoint = startPoint + (PROBE_SIDE_LENGTH-1) / 2.0;
//     float dst = midPoint - pos;
//     return pos + dst * 2;
// }


/*
XXX|XXX
Xxx|xxX
Xxx|xxX
Xxx|xxX
Xxx|xxX
XXX|XXX <-- on this half?
^-- or on this half? 
*/
int leftOrRight(int pos)
{
    pos = pos - 1;
    uint probeWithBorderSize = 2 + PROBE_SIDE_LENGTH;
    uint posInProbe = pos % probeWithBorderSize;
    uint middle = probeWithBorderSize / 2;
    return posInProbe < middle ? 1 : -1;
}

int mirrored(int pos)
{
    pos = pos - 1;
    int probeWithBorderSize = 2 + PROBE_SIDE_LENGTH;
    int posInProbe = int(pos) % probeWithBorderSize;
    int probe = int(pos) / probeWithBorderSize;
    int probeStart = (probe * probeWithBorderSize) + 1;
    int middle = probeWithBorderSize / 2;
    int dstToMiddle = middle - posInProbe;
    return probeStart + middle + dstToMiddle - 1;
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

    else 
    {
        tex[texelPos] = float4(0,0,0,0);
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

    uint2 dim;
    tex.GetDimensions(dim.x,dim.y);
    if(texelPos.x >= dim.x || texelPos.y >= dim.y)
        return;

    bool isAtBorderX, isAtBorderY;

    if(isAtProbeBorder(texelPos,isAtBorderX,isAtBorderY))
    {
        if(isAtBorderX && isAtBorderY)
        {
            int3 offsetPos = int3(texelPos.x /*minus border around tex */ 
            + PROBE_SIDE_LENGTH * leftOrRight(texelPos.x),
            texelPos.y + PROBE_SIDE_LENGTH * leftOrRight(texelPos.y),0);
            float4 texelToCopy = tex.Load(offsetPos);
            tex[texelPos] = texelToCopy;
            //tex[texelPos] = float4(offsetPos,1);
            return;
        }

        if(isAtBorderX)
        {
            int pos = mirrored(texelPos.y);
            int toAdd = leftOrRight(texelPos.x);
            tex[texelPos] = tex.Load(int3(texelPos.x+toAdd,pos,0));
            //tex[texelPos] = float4(pos,0.4,0.4,0);
            return;
        }

        if(isAtBorderY)
        {
            int pos = mirrored(texelPos.x);
            int toAdd = leftOrRight(texelPos.y);
            tex[texelPos] = tex.Load(int3(pos, texelPos.y+toAdd,0));
            //tex[texelPos] = float4(pos,0.4,0.4,0);
            return;
        }
    }
}




