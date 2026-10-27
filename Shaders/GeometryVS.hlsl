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
    float4 tan : TANGENT;
    float2 uv2 : TEXCOORD1;
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
struct VSOut
{
    float4 pos : SV_Position;
    float3 wnrm : NORMAL0;
    float3 wpos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 wtan : TEXCOORD2;
    float3 vts : TEXCOORD3;
    float2 luv : TEXCOORD4;
    float2 uv2 : TEXCOORD5;
};
float3x3 NormalMatrix(float3x3 m)
{
    float3 c0 = cross(m[1], m[2]);
    float3 c1 = cross(m[2], m[0]);
    float3 c2 = cross(m[0], m[1]);
    float det = dot(m[0], c0);
    if (abs(det) < 1e-8) {
        return m;
    }
    return float3x3(c0, c1, c2) / det;
}
float4x4 LoadBoneMatrix(uint index)
{
    uint base = (BoneOffset + index) * 4;
    return float4x4(BoneRows[base], BoneRows[base + 1], BoneRows[base + 2], BoneRows[base + 3]);
}
void SkinVertex(VSInSkinned vin, out float3 pos, out float3 nrm, out float4 tan)
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
    float3x3 rot = (float3x3)skin;
    nrm = mul(vin.nrm, rot);
    tan = float4(mul(vin.tan.xyz, rot), vin.tan.w);
}
VSOut GeoMainVerts(float3 lpos, float3 lnrm, float4 ltan, float2 uv, float2 uv2, float4x4 W)
{
    VSOut vout;
    float4 wp = mul(float4(lpos, 1.0), W);
    vout.wpos = wp.xyz;
    float3x3 wrot = (float3x3)W;
    float3x3 nrot = NormalMatrix(wrot);
    vout.wnrm = normalize(mul(lnrm, nrot));
    float4 vp = mul(wp, View);
    vout.pos = mul(vp, Proj);
    vout.uv = uv;
    vout.luv = uv2;
    vout.uv2 = uv2;
    float3 wt = mul(ltan.xyz, wrot);
    wt -= vout.wnrm * dot(wt, vout.wnrm);
    float wtl = length(wt);
    if (wtl > 1e-6) {
        wt /= wtl;
    } else {
        wt = normalize(cross(vout.wnrm, abs(vout.wnrm.y) < 0.99 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0)));
    }
    vout.wtan = float4(wt, ltan.w);
    float3 wb = cross(vout.wnrm, wt) * ltan.w;
    float3 camW = -mul(View[3].xyz, transpose((float3x3)View));
    float3 toCam = camW - wp.xyz;
    vout.vts = float3(dot(toCam, wt), dot(toCam, wb), dot(toCam, vout.wnrm));
    return vout;
}
VSOut main(VSIn vin)
{
    return GeoMainVerts(vin.pos, vin.nrm, vin.tan, vin.uv, vin.uv2, World);
}
VSOut mainInst(VSIn vin, float4 i0 : INSTANCE0, float4 i1 : INSTANCE1, float4 i2 : INSTANCE2, float4 i3 : INSTANCE3)
{
    float4x4 iw = float4x4(i0, i1, i2, i3);
    return GeoMainVerts(vin.pos, vin.nrm, vin.tan, vin.uv, vin.uv2, iw);
}
VSOut SkinnedMain(VSInSkinned vin)
{
    float3 pos;
    float3 nrm;
    float4 tan;
    SkinVertex(vin, pos, nrm, tan);
    return GeoMainVerts(pos, nrm, tan, vin.uv, vin.uv2, World);
}
