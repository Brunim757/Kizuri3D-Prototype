cbuffer TaaCB : register(b0)
{
    row_major float4x4 PrevVPU;
    row_major float4x4 CurrVPU;
    float4 CamPos;
    float4 CamFwd;
    float4 TaaMisc;
    float4 CamRight;
    float4 CamUp;
    float4 TanWH;
};
Texture2D CurrentTX : register(t0);
Texture2D HistoryTX : register(t1);
Texture2D PositionTX : register(t2);
Texture2D VelocityTX : register(t3);
SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);
struct PSIn
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};
float3 ClipAABB(float3 hist, float3 mn, float3 mx)
{
    float3 c = (mn + mx) * 0.5;
    float3 e = (mx - mn) * 0.5 + 1e-4;
    float3 v = hist - c;
    float3 av = abs(v);
    float m = max(max(av.x / e.x, av.y / e.y), av.z / e.z);
    if (m > 1.0) {
        v /= m;
    }
    return c + v;
}
float4 main(PSIn pin) : SV_Target
{
    float3 wpos = PositionTX.Sample(PointSampler, pin.uv).xyz;
    float3 curr = CurrentTX.Sample(LinearSampler, pin.uv).rgb;
    float valid = TaaMisc.x;
    float blend = TaaMisc.y;
    float isSky = dot(wpos, wpos) < 1e-8 ? 1.0 : 0.0;
    float cdepth = isSky > 0.5 ? 10000.0 : dot(wpos - CamPos.xyz, CamFwd.xyz);
    if (valid < 0.5) {
        return float4(curr, cdepth);
    }
    float2 px = TaaMisc.zw;
    float4 vel = VelocityTX.Sample(PointSampler, pin.uv);
    bool dynamic = vel.z > 0.5 && isSky < 0.5;
    float2 histUv = float2(-10.0, -10.0);
    float2 currUv = pin.uv;
    bool historyValid = false;
    if (dynamic) {
        if (vel.w > 0.5) {
            histUv = vel.xy;
            float4 ppCurrU = mul(float4(wpos, 1.0), CurrVPU);
            if (ppCurrU.w > 1e-5) {
                currUv = ppCurrU.xy / ppCurrU.w * float2(0.5, -0.5) + 0.5;
            }
            historyValid = true;
        }
    } else {
        float2 sndc = float2(pin.uv.x * 2.0 - 1.0, 1.0 - pin.uv.y * 2.0);
        float3 sray = CamFwd.xyz + CamRight.xyz * (sndc.x * TanWH.x) + CamUp.xyz * (sndc.y * TanWH.y);
        float3 sdir = normalize(sray);
        float3 reprojP;
        if (isSky > 0.5) {
            reprojP = CamPos.xyz + sdir * 10000.0;
        } else {
            float st = cdepth / max(dot(sdir, CamFwd.xyz), 1e-4);
            reprojP = CamPos.xyz + sdir * st;
        }
        float4 ppPrevU = mul(float4(reprojP, 1.0), PrevVPU);
        float4 ppCurrU = mul(float4(reprojP, 1.0), CurrVPU);
        if (ppPrevU.w > 1e-5) {
            histUv = ppPrevU.xy / ppPrevU.w * float2(0.5, -0.5) + 0.5;
            historyValid = true;
        }
        if (ppCurrU.w > 1e-5) {
            currUv = ppCurrU.xy / ppCurrU.w * float2(0.5, -0.5) + 0.5;
        }
    }
    float3 hist = curr;
    float hdepth = cdepth;
    bool sampled = false;
    if (historyValid && histUv.x >= 0.0 && histUv.x <= 1.0 && histUv.y >= 0.0 && histUv.y <= 1.0) {
        float4 hs = HistoryTX.Sample(LinearSampler, histUv);
        hist = hs.rgb;
        hdepth = hs.a;
        sampled = true;
    }
    float dth = max(abs(cdepth) * 0.03, 0.05);
    float useHist = sampled ? 1.0 : 0.0;
    if (sampled && abs(cdepth - hdepth) > dth) {
        useHist = 0.0;
    }
    float3 m1 = curr;
    float3 m2 = curr * curr;
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) {
                continue;
            }
            float3 s = CurrentTX.SampleLevel(LinearSampler, pin.uv + float2(float(ox) * px.x, float(oy) * px.y), 0).rgb;
            m1 += s;
            m2 += s * s;
        }
    }
    float3 mean = m1 / 9.0;
    float3 varc = max(m2 / 9.0 - mean * mean, 0.0);
    float3 sigma = sqrt(varc);
    float3 mn = mean - 1.25 * sigma;
    float3 mx = mean + 1.25 * sigma;
    float3 clipped = ClipAABB(hist, mn, mx);
    float2 motionPx = float2(0.0, 0.0);
    if (historyValid) {
        motionPx = (histUv - currUv) / max(px, float2(1e-6, 1e-6));
    }
    float adapt = saturate(length(motionPx) * 0.25);
    float b = lerp(1.0, blend, useHist * (1.0 - adapt));
    float3 outc = lerp(curr, clipped, 1.0 - b);
    return float4(outc, cdepth);
}
