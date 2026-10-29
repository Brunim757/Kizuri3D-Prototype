cbuffer GeoCB : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Proj;
    row_major float4x4 PrevWorld;
    row_major float4x4 PrevViewProj;
    float4 Motion;
};
cbuffer BoneCB : register(b1)
{
    uint BoneOffset;
    uint3 BonePad;
};
StructuredBuffer<float4> BoneRows : register(t0);
StructuredBuffer<float4> PrevBoneRows : register(t1);
struct VSIn
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
};
struct VSInSkinned
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
    uint4 joints : BLENDINDICES;
    float4 weights : BLENDWEIGHT;
};
struct VSOut
{
    float4 pos : SV_Position;
    float2 prevUv : TEXCOORD0;
    float hasPrev : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float2 uv2 : TEXCOORD3;
};
float4x4 LoadBoneMatrix(uint index)
{
    uint base = (BoneOffset + index) * 4;
    return float4x4(BoneRows[base], BoneRows[base + 1], BoneRows[base + 2], BoneRows[base + 3]);
}
float4x4 LoadPrevBoneMatrix(uint index)
{
    uint base = (BoneOffset + index) * 4;
    return float4x4(PrevBoneRows[base], PrevBoneRows[base + 1], PrevBoneRows[base + 2], PrevBoneRows[base + 3]);
}
void SkinVertex(VSInSkinned vin, out float3 pos, out float3 ppos)
{
    float4 w = vin.weights;
    float total = w.x + w.y + w.z + w.w;
    if (total < 1e-6) {
        w = float4(1.0, 0.0, 0.0, 0.0);
    } else {
        w /= total;
    }
    float4x4 skin = LoadBoneMatrix(vin.joints.x) * w.x;
    skin += LoadBoneMatrix(vin.joints.y) * w.y;
    skin += LoadBoneMatrix(vin.joints.z) * w.z;
    skin += LoadBoneMatrix(vin.joints.w) * w.w;
    pos = mul(float4(vin.pos, 1.0), skin).xyz;
    float4x4 prevSkin = LoadPrevBoneMatrix(vin.joints.x) * w.x;
    prevSkin += LoadPrevBoneMatrix(vin.joints.y) * w.y;
    prevSkin += LoadPrevBoneMatrix(vin.joints.z) * w.z;
    prevSkin += LoadPrevBoneMatrix(vin.joints.w) * w.w;
    ppos = mul(float4(vin.pos, 1.0), prevSkin).xyz;
}
VSOut BuildVelocity(float3 lpos, float3 plpos, float2 uv, float2 uv2, float4x4 W, float4x4 PW)
{
    VSOut vout;
    float4 wp = mul(float4(lpos, 1.0), W);
    float4 vp = mul(wp, View);
    vout.pos = mul(vp, Proj);
    float4 pwp = mul(float4(plpos, 1.0), PW);
    float4 pclip = mul(pwp, PrevViewProj);
    vout.prevUv = pclip.w > 1e-5 ? pclip.xy / pclip.w * float2(0.5, -0.5) + 0.5 : float2(-10.0, -10.0);
    vout.hasPrev = Motion.x;
    vout.uv = uv;
    vout.uv2 = uv2;
    return vout;
}
VSOut main(VSIn vin)
{
    return BuildVelocity(vin.pos, vin.pos, vin.uv, vin.uv2, World, PrevWorld);
}
VSOut SkinnedMain(VSInSkinned vin)
{
    float3 pos;
    float3 ppos;
    SkinVertex(vin, pos, ppos);
    return BuildVelocity(pos, ppos, vin.uv, vin.uv2, World, PrevWorld);
}
