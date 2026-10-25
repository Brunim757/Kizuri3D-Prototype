Texture2D GodTX : register(t0);
Texture2D GodDepthTX : register(t1);
Texture2D PositionTX : register(t2);
Texture2D SceneTX : register(t3);
SamplerState LinearSampler : register(s0);
cbuffer GodUpCB : register(b0)
{
    float4 GodUpTexel;
    float4 GodUpCam;
    float4 GodUpFwd;
};
struct PSIn
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};
float4 main(PSIn pin) : SV_Target
{
    float3 Pc = PositionTX.Sample(LinearSampler, pin.uv).xyz;
    float dc = dot(Pc, Pc) < 1e-8 ? 1e9 : max(dot(Pc - GodUpCam.xyz, GodUpFwd.xyz), 0.0);
    float2 texel = GodUpTexel.xy;
    float3 acc = float3(0.0, 0.0, 0.0);
    float wsum = 0.0;
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            float2 suv = pin.uv + float2(float(ox), float(oy)) * texel;
            float d = GodDepthTX.SampleLevel(LinearSampler, suv, 0).r;
            float wgt = exp(-pow((d - dc) / max(dc * 0.05, 0.5), 2.0));
            acc += GodTX.SampleLevel(LinearSampler, suv, 0).rgb * wgt;
            wsum += wgt;
        }
    }
    float3 scene = SceneTX.SampleLevel(LinearSampler, pin.uv, 0).rgb;
    if (wsum < 1e-6) {
        return float4(scene + GodTX.SampleLevel(LinearSampler, pin.uv, 0).rgb, 1.0);
    }
    return float4(scene + acc / wsum, 1.0);
}
