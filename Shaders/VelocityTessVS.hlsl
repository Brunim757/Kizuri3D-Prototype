cbuffer GeoCB : register(b0)
{
    row_major float4x4 World;
    row_major float4x4 View;
    row_major float4x4 Proj;
    row_major float4x4 PrevWorld;
    row_major float4x4 PrevViewProj;
    float4 Motion;
};
struct VSIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
    float2 uv2 : TEXCOORD1;
};
struct VSOut
{
    float3 wpos : TEXCOORD0;
    float3 prevWpos : TEXCOORD1;
    float3 wnrm : NORMAL0;
    float3 prevNrm : NORMAL1;
    float2 uv : TEXCOORD2;
    float2 uv2 : TEXCOORD3;
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
VSOut main(VSIn vin)
{
    VSOut o;
    float4 wp = mul(float4(vin.pos, 1.0), World);
    float4 pwp = mul(float4(vin.pos, 1.0), PrevWorld);
    o.wpos = wp.xyz;
    o.prevWpos = pwp.xyz;
    float3x3 wrot = (float3x3)World;
    float3x3 pwrot = (float3x3)PrevWorld;
    o.wnrm = normalize(mul(vin.nrm, NormalMatrix(wrot)));
    o.prevNrm = normalize(mul(vin.nrm, NormalMatrix(pwrot)));
    o.uv = vin.uv;
    o.uv2 = vin.uv2;
    return o;
}
