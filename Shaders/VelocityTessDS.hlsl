cbuffer GeoCB : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Proj;
    row_major float4x4 PrevWorld;
    row_major float4x4 PrevViewProj;
    float4 Motion;
};
cbuffer TessCB : register(b1)
{
    float4 TessInfo;
};
Texture2D HeightTex : register(t0);
SamplerState TessSamp : register(s0);
struct VSOut
{
    float3 wpos : TEXCOORD0;
    float3 prevWpos : TEXCOORD1;
    float3 wnrm : NORMAL0;
    float3 prevNrm : NORMAL1;
    float2 uv : TEXCOORD2;
    float2 uv2 : TEXCOORD3;
};
struct DSOut
{
    float4 pos : SV_Position;
    float2 prevUv : TEXCOORD0;
    float hasPrev : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float2 uv2 : TEXCOORD3;
};
struct HSTess
{
    float edge[3] : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};
[domain("tri")]
DSOut main(HSTess input, float3 bary : SV_DomainLocation, const OutputPatch<VSOut, 3> patch)
{
    DSOut o;
    float3 wpos = patch[0].wpos * bary.x + patch[1].wpos * bary.y + patch[2].wpos * bary.z;
    float3 prevWpos = patch[0].prevWpos * bary.x + patch[1].prevWpos * bary.y + patch[2].prevWpos * bary.z;
    float3 wnrm = normalize(patch[0].wnrm * bary.x + patch[1].wnrm * bary.y + patch[2].wnrm * bary.z);
    float3 prevNrm = normalize(patch[0].prevNrm * bary.x + patch[1].prevNrm * bary.y + patch[2].prevNrm * bary.z);
    float2 uv = patch[0].uv * bary.x + patch[1].uv * bary.y + patch[2].uv * bary.z;
    float h = HeightTex.SampleLevel(TessSamp, uv, 0).r;
    float hasH = TessInfo.z;
    h = lerp(0.5, h, hasH);
    float disp = (h - 0.5) * TessInfo.y;
    wpos += wnrm * disp;
    prevWpos += prevNrm * disp;
    float4 wp = float4(wpos, 1.0);
    float4 vp = mul(wp, View);
    o.pos = mul(vp, Proj);
    float4 pwp = float4(prevWpos, 1.0);
    float4 pclip = mul(pwp, PrevViewProj);
    o.prevUv = pclip.w > 1e-5 ? pclip.xy / pclip.w * float2(0.5, -0.5) + 0.5 : float2(-10.0, -10.0);
    o.hasPrev = Motion.x;
    o.uv = uv;
    o.uv2 = patch[0].uv2 * bary.x + patch[1].uv2 * bary.y + patch[2].uv2 * bary.z;
    return o;
}
