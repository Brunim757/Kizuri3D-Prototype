cbuffer GeoCB : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Proj;
};
cbuffer BoneCB : register(b1)
{
    uint BoneOffset;
    uint3 BonePad;
};
StructuredBuffer<float4> BoneRows : register(t0);
struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
};
struct VSInSkinned
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float4 tan : TANGENT;
    float2 uv2 : TEXCOORD1;
    uint4 joints : BLENDINDICES;
    float4 weights : BLENDWEIGHT;
};
float4x4 LoadBoneMatrix(uint index)
{
    uint base = (BoneOffset + index) * 4;
    return float4x4(BoneRows[base], BoneRows[base + 1], BoneRows[base + 2], BoneRows[base + 3]);
}
float4 main(VSIn vin) : SV_Position
{
    float4 wp = mul(float4(vin.pos, 1.0), World);
    float4 vp = mul(wp, View);
    return mul(vp, Proj);
}
float4 SkinnedMain(VSInSkinned vin) : SV_Position
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
    float3 pos = mul(float4(vin.pos, 1.0), skin).xyz;
    float4 wp = mul(float4(pos, 1.0), World);
    float4 vp = mul(wp, View);
    return mul(vp, Proj);
}
