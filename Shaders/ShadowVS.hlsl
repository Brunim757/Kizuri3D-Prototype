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
    float2 uv : TEXCOORD1;
    float2 uv2 : TEXCOORD5;
};
float4x4 LoadBoneMatrix(uint index)
{
    uint base = (BoneOffset + index) * 4;
    return float4x4(BoneRows[base], BoneRows[base + 1], BoneRows[base + 2], BoneRows[base + 3]);
}
VSOut BuildShadow(float3 lpos, float2 uv, float2 uv2)
{
    VSOut o;
    float4 wp = mul(float4(lpos, 1.0), World);
    float4 vp = mul(wp, View);
    o.pos = mul(vp, Proj);
    o.uv = uv;
    o.uv2 = uv2;
    return o;
}
VSOut main(VSIn vin)
{
    return BuildShadow(vin.pos, vin.uv, vin.uv2);
}
VSOut SkinnedMain(VSInSkinned vin)
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
    return BuildShadow(pos, vin.uv, vin.uv2);
}
