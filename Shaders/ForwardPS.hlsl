cbuffer LightCB : register(b0)
{
    float4 CamPos;
    float4 LightDir;
    float4 LightColor;
    float4 Counts;
    float4 CamFwd;
    float4 LightA[16];
    float4 LightB[16];
    float4 LightC[16];
    float4 LightD[16];
    row_major float4x4 CascadeVP[4];
    float4 CascadeSplit;
    float4 CascadeNear;
    float4 CascadeFar;
    float4 CascadeUV;
    float4 CascadeK;
    float4 CascadeSize;
    float4 CascadeLight;
    float4 ShadowInfo;
    row_major float4x4 SpotVP[4];
    float4 SpotMeta[4];
    float4 PointInfo;
    float4 PointMeta;
    float4 GradeInfo;
    float4 CamRight;
    float4 CamUp;
    float4 SkySun;
    float4 SkyColor;
    float4 SsaoInfo;
    row_major float4x4 SsaoVP;
    float4 FogInfo;
    float4 FogColor;
    float4 ProbeBoxMin[4];
    float4 ProbeBoxMax[4];
    float4 IblInfo;
    float4 ShadowCtl;
    float4 VsmInfo;
};
cbuffer MatCB : register(b1)
{
    float4 Albedo;
    float4 Params;
    float4 Extra;
};
cbuffer MatExt : register(b2)
{
    float4 ExtA;
    float4 ExtB;
    float4 ExtC;
    float4 ExtD;
    float4 ExtE;
    float4 ExtF;
    float4 ExtG;
    float4 ExtH;
    float4 ExtI;
    float4 ExtJ;
    float4 XfA[5];
    float4 XfB[5];
    float4 SssPrm;
};
cbuffer ScreenCB : register(b3)
{
    float4 ScreenWH;
    float4 FwdCount;
    float4 FwdIdx[4];
};
cbuffer AmbientCB : register(b7)
{
    float4 AmbientProbe;
};
cbuffer FogVolCB : register(b6)
{
    float4 FogVolMin[4];
    float4 FogVolMax[4];
    float4 FogVolCol[4];
};
cbuffer SpotCookieCB : register(b4)
{
    row_major float4x4 SpotCookieVP[16];
};
Texture2D AlbedoTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D OrmTex : register(t2);
Texture2D LightmapTex : register(t4);
Texture2D EmissiveTex : register(t5);
Texture2D TransTex : register(t6);
Texture2D ThickTex : register(t7);
Texture2D TransSrc : register(t16);
TextureCube EnvTX : register(t17);
Texture2D DetailFTex : register(t18);
Texture2D DetailFNormalTex : register(t19);
Texture2D ShadowAtlas : register(t8);
TextureCube PointCube : register(t9);
Texture2D MomentsTX : register(t10);
Texture2D CookieTX : register(t11);
Texture2DArray CookieArray : register(t12);
Texture2D CelTX : register(t28);
SamplerState GeoSamp : register(s0);
SamplerState ShadowSampler : register(s1);
SamplerState LinearSampler : register(s2);
static const float2 Poisson16[16] = {
    float2(-0.9428, -0.3997),
    float2(0.9456, -0.7689),
    float2(-0.0942, -0.9294),
    float2(0.3450, 0.2939),
    float2(-0.9159, 0.4577),
    float2(-0.8154, -0.8791),
    float2(-0.3828, 0.2768),
    float2(0.9748, 0.1661),
    float2(0.4435, -0.0843),
    float2(-0.3841, -0.1009),
    float2(0.4458, -0.7300),
    float2(0.3809, 0.8064),
    float2(0.3057, 0.1654),
    float2(-0.1006, 0.7641),
    float2(0.7675, -0.1007),
    float2(-0.0122, 0.1461)
};
struct PSIn
{
    float4 pos : SV_Position;
    float3 wnrm : NORMAL0;
    float3 wpos : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 wtan : TEXCOORD2;
    float3 vts : TEXCOORD3;
    float2 luv : TEXCOORD4;
    float2 uv2 : TEXCOORD5;
    bool front : SV_IsFrontFace;
};
float2 XformUv(float2 uv, float2 uv2, int slot)
{
    float4 a = XfA[slot];
    float4 b = XfB[slot];
    float2 base = b.y > 0.5 ? uv2 : uv;
    float2 scaled = base * a.zw;
    float c = cos(b.x);
    float s = sin(b.x);
    return float2(scaled.x * c - scaled.y * s, scaled.x * s + scaled.y * c) + a.xy;
}
float NdfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;
    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (3.14159265 * denom * denom + 1e-6);
}
float GaSchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k + 1e-6);
}
float GaSchlickGGX(float cosLi, float cosLo, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return GaSchlickG1(cosLi, k) * GaSchlickG1(cosLo, k);
}
float ApplyAttenuation(float dist, float range)
{
    if (Counts.y < 0.5) {
        return pow(saturate(1.0 - (dist * dist) / max(range * range, 1e-6)), 2.0);
    } else if (Counts.y < 1.5) {
        float phys = 1.0 / (dist * dist + 1.0);
        float win = saturate(1.0 - (dist * dist) / max(range * range, 1e-6));
        return phys * win;
    }
    return saturate(1.0 - dist / max(range, 1e-4));
}
float BlockerSearch2D(float2 uv, float recvDist, float nearZ, float farZ, float searchUV)
{
    float range = max(farZ - nearZ, 1e-4);
    float sum = 0.0;
    int n = 0;
    for (int k = 0; k < 16; ++k) {
        float d = ShadowAtlas.SampleLevel(ShadowSampler, uv + Poisson16[k] * searchUV, 0).r;
        float dist = nearZ + d * range;
        if (dist < recvDist - range * 0.001) {
            sum += dist;
            n += 1;
        }
    }
    if (n == 0) {
        return -1.0;
    }
    return sum / (float)n;
}
float PCSSFilter2D(float2 uv, float recvZ, float filterUV, float bias)
{
    float lit = 0.0;
    for (int k = 0; k < 16; ++k) {
        float d = ShadowAtlas.SampleLevel(ShadowSampler, uv + Poisson16[k] * filterUV, 0).r;
        lit += (recvZ - bias < d) ? 1.0 : 0.0;
    }
    return lit / 16.0;
}
float SampleTilePCSS(float3 wpos, float3 nrm, row_major float4x4 vp, float tile, float tileK, float nearZ, float farZ, float uvScale, float perspTanHalf, float effSize)
{
    float usInit = uvScale;
    float4 sp0 = mul(float4(wpos, 1.0), vp);
    float recv0 = nearZ;
    if (sp0.w > 1e-6) {
        recv0 = nearZ + (sp0.z / sp0.w) * max(farZ - nearZ, 1e-4);
    }
    if (perspTanHalf > 0.0 && recv0 > 1e-4) {
        usInit = 0.5 * clamp(tileK, 0.05, 1.0) / max(2.0 * perspTanHalf * recv0, 1e-6);
    }
    float nb = ShadowInfo.w / max(usInit, 1e-6) * ShadowCtl.y;
    float3 wposB2 = wpos + nrm * nb;
    float4 sp = mul(float4(wposB2, 1.0), vp);
    if (sp.w <= 0.0) {
        return 1.0;
    }
    sp.xyz /= sp.w;
    if (abs(sp.x) > 1.0 || abs(sp.y) > 1.0 || sp.z < 0.0 || sp.z > 1.0) {
        return 1.0;
    }
    float tx = fmod(tile, 2.0);
    float ty = floor(tile / 2.0);
    float k = clamp(tileK, 0.05, 1.0);
    float u = (sp.x * 0.5 + 0.5) * 0.5 * k + tx * 0.5;
    float v = (0.5 - sp.y * 0.5) * 0.5 * k + ty * 0.5;
    float2 uv = float2(u, v);
    float texel = ShadowInfo.w;
    float range = max(farZ - nearZ, 1e-4);
    float recvDist = nearZ + sp.z * range;
    float bias = ShadowCtl.z > 0.5 ? max(nb / range, texel * 0.25) : 0.0015;
    if (ShadowCtl.x > 0.5) {
        float pr = texel * max(ShadowCtl.w, 0.5);
        float lit = 0.0;
        for (int ay = -1; ay <= 1; ++ay) {
            for (int ax = -1; ax <= 1; ++ax) {
                float d = ShadowAtlas.SampleLevel(ShadowSampler, uv + float2((float)ax, (float)ay) * pr, 0).r;
                lit += (sp.z - bias < d) ? 1.0 : 0.0;
            }
        }
        return lit / 9.0;
    }
    if (effSize <= 0.0001) {
        float d0 = ShadowAtlas.SampleLevel(ShadowSampler, uv, 0).r;
        return (sp.z - bias < d0) ? 1.0 : 0.0;
    }
    float us = uvScale;
    if (perspTanHalf > 0.0 && recvDist > 1e-4) {
        us = 0.5 * k / max(2.0 * perspTanHalf * recvDist, 1e-6);
    }
    float maxR = max(0.25 * k - 4.0 * texel, texel);
    float searchUV = min(effSize * 0.5 * us, maxR);
    float blocker = BlockerSearch2D(uv, recvDist, nearZ, farZ, searchUV);
    if (blocker < 0.0) {
        return 1.0;
    }
    float penumbra = max((recvDist - blocker) * effSize / max(blocker, 1e-4), 0.0);
    float filterUV = min(penumbra * us, maxR);
    if (filterUV <= texel * 0.5) {
        float d1 = ShadowAtlas.SampleLevel(ShadowSampler, uv, 0).r;
        return (sp.z - bias < d1) ? 1.0 : 0.0;
    }
    return PCSSFilter2D(uv, sp.z, filterUV, bias);
}
float ChebyshevUpper(float m1, float m2, float z, float bleed)
{
    float var = max(m2 - m1 * m1, 1e-5);
    float d = max(z - m1, 0.0);
    float p = var / (var + d * d);
    return clamp((p - bleed) / max(1.0 - bleed, 1e-4), 0.0, 1.0);
}
float SampleTileVSM(float3 wpos, float3 nrm, row_major float4x4 vp, float tile, float tileK, float nearZ, float farZ, float uvScale, float perspTanHalf, bool evsm)
{
    if (VsmInfo.w > 0.0 && distance(wpos, CamPos.xyz) > VsmInfo.w) {
        return 1.0;
    }
    if (VsmInfo.x < 0.5) {
        return 1.0;
    }
    float usInit = uvScale;
    float4 sp0 = mul(float4(wpos, 1.0), vp);
    float recv0 = nearZ;
    if (sp0.w > 1e-6) {
        recv0 = nearZ + (sp0.z / sp0.w) * max(farZ - nearZ, 1e-4);
    }
    if (perspTanHalf > 0.0 && recv0 > 1e-4) {
        usInit = 0.5 * clamp(tileK, 0.05, 1.0) / max(2.0 * perspTanHalf * recv0, 1e-6);
    }
    float nb = ShadowInfo.w / max(usInit, 1e-6) * ShadowCtl.y;
    float3 wposB2 = wpos + nrm * nb;
    float4 sp = mul(float4(wposB2, 1.0), vp);
    if (sp.w <= 0.0) {
        return 1.0;
    }
    sp.xyz /= sp.w;
    if (abs(sp.x) > 1.0 || abs(sp.y) > 1.0 || sp.z < 0.0 || sp.z > 1.0) {
        return 1.0;
    }
    float tx = fmod(tile, 2.0);
    float ty = floor(tile / 2.0);
    float k = clamp(tileK, 0.05, 1.0);
    float u = (sp.x * 0.5 + 0.5) * 0.5 * k + tx * 0.5;
    float v = (0.5 - sp.y * 0.5) * 0.5 * k + ty * 0.5;
    float4 m = MomentsTX.SampleLevel(LinearSampler, float2(u, v), 0);
    float bleed = clamp(VsmInfo.y, 0.0, 0.9);
    float zb = sp.z - 0.0003;
    if (!evsm) {
        return (zb <= m.x) ? 1.0 : ChebyshevUpper(m.x, m.y, zb, bleed);
    }
    float c = clamp(VsmInfo.z, 0.5, 5.0);
    float pw = exp(c * zb);
    float nw = exp(-c * zb);
    float sPos = (pw <= m.x) ? 1.0 : ChebyshevUpper(m.x, m.y, pw, bleed);
    float sNeg = (nw <= m.z) ? 1.0 : ChebyshevUpper(m.z, m.w, nw, bleed);
    return min(sPos, sNeg);
}
float SampleTileShadow(float3 wpos, float3 nrm, row_major float4x4 vp, float tile, float tileK, float nearZ, float farZ, float uvScale, float perspTanHalf, float effSize)
{
    float lit = 1.0;
    if (ShadowCtl.x > 1.5) {
        lit = SampleTileVSM(wpos, nrm, vp, tile, tileK, nearZ, farZ, uvScale, perspTanHalf, ShadowCtl.x > 2.5);
    } else {
        lit = SampleTilePCSS(wpos, nrm, vp, tile, tileK, nearZ, farZ, uvScale, perspTanHalf, effSize);
    }
    return lit;
}
float LinearizeCube(float ndcZ, float nearZ, float farZ)
{
    float denom = 1.0 - ndcZ * (farZ - nearZ) / max(farZ, 1e-4);
    if (denom < 1e-6) {
        return farZ;
    }
    return nearZ / denom;
}
float SamplePointPCSS(float3 wpos, float3 nrm, float effSize)
{
    float nb = 0.02 * ShadowCtl.y;
    float3 wb = wpos + nrm * nb;
    float3 toFrag = wb - PointInfo.xyz;
    float dist = length(toFrag);
    if (dist > PointInfo.w || dist < 1e-4) {
        return 1.0;
    }
    float3 dir = toFrag / dist;
    float3 upRef = abs(dir.y) > 0.99 ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0);
    float3 t1 = normalize(cross(upRef, dir));
    float3 t2 = cross(dir, t1);
    float nearZ = PointMeta.z;
    float farZ = PointInfo.w;
    float bias = ShadowCtl.z > 0.5 ? max(nb * 2.0, 0.005) : 0.05;
    if (ShadowCtl.x > 0.5) {
        float ang = 0.02 * max(ShadowCtl.w, 0.5);
        float lit = 0.0;
        lit += (dist - bias < LinearizeCube(PointCube.SampleLevel(ShadowSampler, dir, 0).r, nearZ, farZ)) ? 1.0 : 0.0;
        lit += (dist - bias < LinearizeCube(PointCube.SampleLevel(ShadowSampler, normalize(dir + t1 * ang), 0).r, nearZ, farZ)) ? 1.0 : 0.0;
        lit += (dist - bias < LinearizeCube(PointCube.SampleLevel(ShadowSampler, normalize(dir - t1 * ang), 0).r, nearZ, farZ)) ? 1.0 : 0.0;
        lit += (dist - bias < LinearizeCube(PointCube.SampleLevel(ShadowSampler, normalize(dir + t2 * ang), 0).r, nearZ, farZ)) ? 1.0 : 0.0;
        lit += (dist - bias < LinearizeCube(PointCube.SampleLevel(ShadowSampler, normalize(dir - t2 * ang), 0).r, nearZ, farZ)) ? 1.0 : 0.0;
        return lit / 5.0;
    }
    if (effSize <= 0.0001) {
        float z0 = PointCube.SampleLevel(ShadowSampler, dir, 0).r;
        return (dist - bias < LinearizeCube(z0, nearZ, farZ)) ? 1.0 : 0.0;
    }
    float searchAng = min((effSize * 0.5) / max(dist, 1e-4), 0.05);
    float bsum = 0.0;
    int bn = 0;
    for (int k = 0; k < 16; ++k) {
        float3 sd = normalize(dir + (t1 * Poisson16[k].x + t2 * Poisson16[k].y) * searchAng);
        float bz = PointCube.SampleLevel(ShadowSampler, sd, 0).r;
        float blin = LinearizeCube(bz, nearZ, farZ);
        if (blin < dist - bias) {
            bsum += blin;
            bn += 1;
        }
    }
    if (bn == 0) {
        return 1.0;
    }
    float blocker = bsum / (float)bn;
    float penumbra = max((dist - blocker) * effSize / max(blocker, 1e-4), 0.0);
    float filterAng = min(penumbra / max(dist, 1e-4), 0.08);
    float lit = 0.0;
    for (int f = 0; f < 16; ++f) {
        float3 fd = normalize(dir + (t1 * Poisson16[f].x + t2 * Poisson16[f].y) * filterAng);
        float z = PointCube.SampleLevel(ShadowSampler, fd, 0).r;
        float lin = LinearizeCube(z, nearZ, farZ);
        lit += (dist - bias < lin) ? 1.0 : 0.0;
    }
    return lit / 16.0;
}
bool FogRayBox(float3 origin, float3 dir, float3 bmin, float3 bmax, out float t0, out float t1)
{
    float3 d = dir;
    d.x = abs(d.x) < 1e-6 ? 1e-6 : d.x;
    d.y = abs(d.y) < 1e-6 ? 1e-6 : d.y;
    d.z = abs(d.z) < 1e-6 ? 1e-6 : d.z;
    float3 inv = 1.0 / d;
    float3 ta = (bmin - origin) * inv;
    float3 tb = (bmax - origin) * inv;
    float3 tmin = min(ta, tb);
    float3 tmax = max(ta, tb);
    t0 = max(max(tmin.x, tmin.y), tmin.z);
    t1 = min(min(tmax.x, tmax.y), tmax.z);
    return t1 > max(t0, 0.0);
}
float3 ApplyFog(float3 col, float3 rayDir, float tMax)
{
    float dens = FogInfo.y;
    float3 toSun = normalize(SkySun.xyz);
    float3 sunCol = SkyColor.rgb * max(SkySun.w, 0.0);
    float cosT = max(dot(rayDir, toSun), 0.0);
    float ph = 0.15 + 0.85 * pow(cosT, 8.0);
    float heightScale = 1.0;
    if (FogInfo.z > 1e-6) {
        float avgY = CamPos.y + rayDir.y * tMax * 0.5;
        heightScale = exp(-max(avgY - FogInfo.w, 0.0) * FogInfo.z);
    }
    float globalDensity = dens * heightScale;
    float trans = exp(-globalDensity * tMax);
    float3 insc = FogColor.rgb * (1.0 - trans);
    insc += (sunCol * ph + AmbientProbe.rgb) * (1.0 - trans) * 0.12;
    for (int fv = 0; fv < 4; ++fv) {
        float vd = FogVolMin[fv].w;
        if (vd <= 0.0) {
            continue;
        }
        float3 bmin = FogVolMin[fv].xyz;
        float3 bmax = FogVolMax[fv].xyz;
        float t0;
        float t1;
        if (!FogRayBox(CamPos.xyz, rayDir, bmin, bmax, t0, t1)) {
            continue;
        }
        t0 = max(t0, 0.0);
        t1 = min(t1, tMax);
        if (t1 <= t0) {
            continue;
        }
        float3 center = (bmin + bmax) * 0.5;
        float3 halfSize = max((bmax - bmin) * 0.5, float3(1e-4, 1e-4, 1e-4));
        float3 volLight = FogVolCol[fv].rgb * (sunCol * ph * 0.65 + AmbientProbe.rgb * 0.35);
        float stepLen = (t1 - t0) / 8.0;
        float vTrans = 1.0;
        float3 vInsc = float3(0.0, 0.0, 0.0);
        for (int s = 0; s < 8; ++s) {
            float3 p = CamPos.xyz + rayDir * (t0 + ((float)s + 0.5) * stepLen);
            float3 q = abs(p - center) / halfSize;
            float edge = 1.0 - max(q.x, max(q.y, q.z));
            float localDensity = vd * smoothstep(0.0, 0.35, edge);
            vInsc += vTrans * volLight * localDensity * stepLen;
            vTrans *= exp(-localDensity * stepLen);
        }
        insc += vInsc * trans;
        trans *= vTrans;
    }
    return col * trans + insc;
}
float3 SkyGradient(float3 viewDir, float3 sunDirTo, float3 sunColor, float sunIntensity)
{
    float vl = max(length(viewDir), 1e-6);
    float3 d = viewDir / vl;
    float sl = max(length(sunDirTo), 1e-6);
    float3 s = sunDirTo / sl;
    float dayness = clamp(s.y * 3.0 + 0.3, 0.0, 1.0);
    float upness = clamp(d.y, 0.0, 1.0);
    float3 zen = float3(0.20, 0.38, 0.70);
    float3 hor = float3(0.65, 0.55, 0.45);
    float3 gnd = float3(0.10, 0.09, 0.08);
    float zb = 0.15 + 0.85 * dayness;
    float pw = pow(upness, 0.5);
    float3 sky = hor + (zen - hor) * pw;
    float at = abs(d.y);
    float hb = exp(-at * 6.0);
    sky += (hor - sky) * hb * 0.5;
    if (d.y < 0.0) {
        float gk = clamp(-d.y * 3.0, 0.0, 1.0);
        sky += (gnd - sky) * gk;
    }
    float dim = (0.2 + 0.8 * dayness) * zb;
    sky *= dim;
    float cosG = clamp(dot(d, s), 0.0, 1.0);
    float glow = pow(cosG, 900.0) * 4.0 + pow(cosG, 10.0) * 0.25;
    float sf = (0.15 + 0.85 * dayness) * sunIntensity;
    return sky + sunColor * glow * sf;
}
float3 ShadeCelFwd(float3 N, float3 V, float3 L, float3 lightColor, float3 albedo, float3 F0, float metallic, float4 cel)
{
    float ndl = saturate(dot(N, L));
    float bands = max(cel.y, 1.0);
    float band = floor(ndl * bands + 0.5) / bands;
    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), lerp(160.0, 8.0, saturate(cel.z)));
    float specMask = smoothstep(0.35, 0.55, spec);
    float3 specTint = lerp(float3(0.04, 0.04, 0.04), F0, metallic);
    float rim = pow(1.0 - saturate(dot(N, V)), 3.0);
    rim *= smoothstep(-0.1, 0.4, dot(N, L));
    float3 base = albedo * band;
    base += (specTint * 6.0 + 0.25) * specMask * cel.z;
    base += albedo * rim * cel.w * 0.5;
    return base * lightColor;
}
float3 ShadePbrFwd(float3 N, float3 V, float3 L, float3 lightColor, float3 albedo, float roughness, float metallic, float3 F0)
{
    float3 H = normalize(L + V);
    float cosLi = saturate(dot(N, L));
    float cosLo = saturate(dot(N, V));
    float cosLh = saturate(dot(N, H));
    float vh = saturate(dot(V, H));
    float3 Fb = lerp(F0, albedo, metallic);
    float3 F = Fb + (1.0 - Fb) * pow(1.0 - vh, 5.0);
    float D = NdfGGX(cosLh, roughness);
    float G = GaSchlickGGX(cosLi, cosLo, roughness);
    float3 spec = (F * D * G) / max(4.0 * cosLi * cosLo, 1e-5);
    float3 kd = (1.0 - F) * (1.0 - metallic);
    float wrapLi = saturate((dot(N, L) + 0.5) / 1.5);
    wrapLi *= wrapLi;
    return (kd * albedo / 3.14159265 * wrapLi + spec * cosLi) * lightColor;
}
float3 ShadeFwd(float3 N, float3 V, float3 L, float3 lightColor, float3 albedo, float roughness, float metallic, float3 F0, float4 cel)
{
    float3 result = float3(0.0, 0.0, 0.0);
    if (cel.x > 0.5) {
        result = ShadeCelFwd(N, V, L, lightColor, albedo, F0, metallic, cel);
    } else {
        result = ShadePbrFwd(N, V, L, lightColor, albedo, roughness, metallic, F0);
    }
    return result;
}
float3 ShadeSssFwd(float3 N, float3 L, float3 lightColor, float3 albedo, float3 tint, float amt, float wrap)
{
    if (amt < 0.001) {
        return float3(0.0, 0.0, 0.0);
    }
    float w = saturate((dot(N, L) + wrap) / max(1.0 + wrap, 1e-3));
    float prof = w * w * (3.0 - 2.0 * w);
    return albedo * tint * (lightColor * (prof * amt));
}
float3 ShadeHairFwd(float3 N, float3 T, float3 V, float3 L, float3 lightColor, float3 tint, float amt, float shift)
{
    if (amt < 0.001 || dot(T, T) < 1e-6) {
        return float3(0.0, 0.0, 0.0);
    }
    float3 Tn = normalize(T - N * dot(N, T));
    float3 H = normalize(L + V);
    float th = dot(Tn, H);
    float sinTH = sqrt(max(0.0, 1.0 - th * th));
    float3 T2 = normalize(Tn + N * shift);
    float th2 = dot(T2, H);
    float sinTH2 = sqrt(max(0.0, 1.0 - th2 * th2));
    float spec = pow(sinTH, 80.0) + pow(sinTH2, 24.0) * 0.5;
    return tint * (lightColor * (spec * amt));
}
float4 main(PSIn pin) : SV_Target
{
    uint flags = (uint) Params.z;
    float3 n = normalize(pin.wnrm);
    if (!pin.front && ExtB.y > 0.5) {
        n = -n;
    }
    float3 alb = Albedo.rgb;
    float alpha = Albedo.a;
    if ((flags & 1u) != 0u) {
        float4 albT = AlbedoTex.Sample(GeoSamp, XformUv(pin.uv, pin.uv2, 0));
        alb *= albT.rgb;
        alpha *= albT.a;
    }
    if (ExtB.x > 0.5 && ExtB.x < 1.5 && alpha < ExtA.w) {
        discard;
    }
    float rough = Params.x;
    float metal = Params.y;
    if ((flags & 2u) != 0u) {
        float2 nuv = XformUv(pin.uv, pin.uv2, 1);
        float3 t = normalize(pin.wtan.xyz);
        float3 b = cross(n, t) * pin.wtan.w;
        float2 nxy = NormalTex.Sample(GeoSamp, nuv).rg * 2.0 - 1.0;
        float nz = sqrt(saturate(1.0 - dot(nxy, nxy)));
        n = normalize(t * nxy.x + b * nxy.y + n * nz);
    }
    if ((flags & 4u) != 0u) {
        float3 orm = OrmTex.Sample(GeoSamp, XformUv(pin.uv, pin.uv2, 2)).rgb;
        rough = orm.g;
        metal = orm.b;
    }
    if ((flags & 1048576u) != 0u) {
        float3 det = DetailFTex.Sample(GeoSamp, pin.uv * max(ExtI.x, 0.01)).rgb;
        alb *= det * 2.0;
    }
    if ((flags & 2097152u) != 0u) {
        float3 t2 = normalize(pin.wtan.xyz);
        float3 b2 = cross(n, t2) * pin.wtan.w;
        float2 dnxy = DetailFNormalTex.Sample(GeoSamp, pin.uv * max(ExtI.x, 0.01)).rg * 2.0 - 1.0;
        float dnz = sqrt(saturate(1.0 - dot(dnxy, dnxy)));
        n = normalize(n + (t2 * dnxy.x + b2 * dnxy.y) * 0.5 + n * (dnz - 1.0) * 0.5);
    }
    if ((flags & 4194304u) != 0u) {
        float lw = saturate(ExtI.y);
        alb = lerp(alb, ExtJ.rgb, lw);
        rough = lerp(rough, ExtI.z, lw);
        metal = lerp(metal, ExtI.w, lw);
    }
    float iorSafe = max(ExtA.y, 1.01);
    float f0d = (iorSafe - 1.0) / (iorSafe + 1.0);
    f0d *= f0d;
    float3 F0 = float3(f0d, f0d, f0d);
    float3 emi = ExtE.rgb * ExtA.x;
    if ((flags & 32u) != 0u) {
        float3 emiFactor = ExtE.rgb;
        if (dot(emiFactor, float3(1.0, 1.0, 1.0)) <= 0.0) {
            emiFactor = float3(1.0, 1.0, 1.0);
        }
        emi = EmissiveTex.Sample(GeoSamp, XformUv(pin.uv, pin.uv2, 4)).rgb * emiFactor * ExtA.x;
    }
    float3 V = normalize(CamPos.xyz - pin.wpos);
    float3 wposB = pin.wpos + n * (ShadowInfo.w * 32.0 * ShadowCtl.y);
    float3 Lsun = normalize(-LightDir.xyz);
    float sssWrapF = clamp(AmbientProbe.a * 0.5, 0.0, 2.0);
    float isSkinF = ((flags & 16777216u) != 0u) ? 1.0 : 0.0;
    float isHairF = ((flags & 33554432u) != 0u) ? 1.0 : 0.0;
    float isEyeF = ((flags & 67108864u) != 0u) ? 1.0 : 0.0;
    float sssAmtF = SssPrm.a * (1.0 + isSkinF * 0.5);
    uint celRow = (uint) (ExtE.w + 0.5);
    if (celRow > 255u) {
        celRow = 255u;
    }
    float4 celRawF = CelTX.Load(int3((int) celRow, 0, 0));
    float4 celInfoF = float4(celRawF.r, 1.0 + celRawF.g * 7.0, celRawF.b, celRawF.a);
    float celOnF = celInfoF.x > 0.5 ? 1.0 : 0.0;
    float3 fwdTan = normalize(pin.wtan.xyz - n * dot(n, pin.wtan.xyz) + 1e-6);
    float3 col = ShadeFwd(n, V, Lsun, LightColor.rgb, alb, rough, metal, F0, celInfoF);
    col += ShadeSssFwd(n, Lsun, LightColor.rgb, alb, SssPrm.rgb, sssAmtF, sssWrapF) * (1.0 - celOnF);
    col += ShadeHairFwd(n, fwdTan, V, Lsun, LightColor.rgb, SssPrm.rgb, isHairF * sssAmtF, 0.15) * (1.0 - celOnF);
    int count = (int) Counts.x;
    if (count > 16) {
        count = 16;
    }
    int fcount = clamp((int)FwdCount.x, 0, 16);
    [loop]
    for (int k = 0; k < fcount; ++k) {
        int i = clamp((int)FwdIdx[k >> 2][k & 3], 0, 15);
        if (i >= count) {
            continue;
        }
        float kind = LightA[i].w;
        if (kind > 2.5) {
            float3 Rn = normalize(LightB[i].xyz);
            float3 rref = abs(Rn.y) > 0.99 ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0);
            float3 rright = normalize(cross(Rn, rref));
            float3 rup = cross(Rn, rright);
            float3 toC = pin.wpos - LightA[i].xyz;
            float2 ph = float2(dot(toC, rright), dot(toC, rup));
            float2 hh = float2(max(LightC[i].a, 0.01), max(LightD[i].x, 0.01));
            float2 cp = clamp(ph, -hh, hh);
            float3 closest = LightA[i].xyz + rright * cp.x + rup * cp.y;
            float3 toL = closest - pin.wpos;
            float rdist = length(toL);
            float3 Lr = toL / max(rdist, 1e-4);
            float rrange = max(LightB[i].w, 1e-3);
            float ratt = ApplyAttenuation(rdist, rrange) * saturate(dot(-Lr, Rn) * 0.5 + 0.5);
            col += ShadeFwd(n, V, Lr, LightC[i].rgb, alb, rough, metal, F0, celInfoF) * ratt;
            col += ShadeSssFwd(n, Lr, LightC[i].rgb, alb, SssPrm.rgb, sssAmtF, sssWrapF) * ratt * (1.0 - celOnF);
            col += ShadeHairFwd(n, fwdTan, V, Lr, LightC[i].rgb, SssPrm.rgb, isHairF * sssAmtF, 0.15) * ratt * (1.0 - celOnF);
        } else if (kind > 1.5) {
            float3 Ld = normalize(-LightA[i].xyz);
            float dirShadow = 1.0;
            if (ShadowInfo.y > 0.5) {
                float viewDepth = dot(pin.wpos - CamPos.xyz, CamFwd.xyz);
                [loop]
                for (int c = 0; c < 4; ++c) {
                    if (CascadeLight[c] == (float)i && viewDepth <= CascadeSplit[c]) {
                        dirShadow = SampleTileShadow(wposB, n, CascadeVP[c], (float)c, CascadeK[c], CascadeNear[c], CascadeFar[c], CascadeUV[c], 0.0, CascadeSize[c]);
                        break;
                    }
                }
            }
            col += ShadeFwd(n, V, Ld, LightC[i].rgb, alb, rough, metal, F0, celInfoF) * dirShadow;
            col += ShadeSssFwd(n, Ld, LightC[i].rgb, alb, SssPrm.rgb, sssAmtF, sssWrapF) * dirShadow * (1.0 - celOnF);
            col += ShadeHairFwd(n, fwdTan, V, Ld, LightC[i].rgb, SssPrm.rgb, isHairF * sssAmtF, 0.15) * dirShadow * (1.0 - celOnF);
        } else {
            float3 toLight = LightA[i].xyz - pin.wpos;
            float dist = length(toLight);
            float3 L = toLight / max(dist, 1e-4);
            float range = max(LightB[i].w, 1e-3);
            float att = ApplyAttenuation(dist, range);
            float spot = 1.0;
            float lightShadow = 1.0;
            float3 cookie = float3(1.0, 1.0, 1.0);
            if (kind > 0.5) {
                float cosT = dot(L, -LightB[i].xyz);
                spot = smoothstep(LightC[i].a, LightC[i].a + LightD[i].x, cosT);
                if (ShadowInfo.y > 0.5) {
                    [loop]
                    for (int s = 0; s < 4; ++s) {
                        if ((int)(SpotMeta[s].y + 0.5) == i && SpotMeta[s].x >= 0.0) {
                            float cosH = max(LightC[i].a, 0.05);
                            float tanH = sqrt(max(1.0 - cosH * cosH, 1e-6)) / cosH;
                            lightShadow = SampleTileShadow(wposB, n, SpotVP[s], SpotMeta[s].x, SpotMeta[s].w, 0.5, max(LightB[i].w, 1.0), 0.0, tanH, SpotMeta[s].z);
                        }
                    }
                }
                if (Counts.w > 0.5) {
                    float4 scc = mul(float4(wposB, 1.0), SpotCookieVP[i]);
                    if (scc.w > 1e-5) {
                        float2 cuv = scc.xy / scc.w * float2(0.5, -0.5) + 0.5;
                        if (cuv.x >= 0.0 && cuv.x <= 1.0 && cuv.y >= 0.0 && cuv.y <= 1.0) {
                            if (Counts.w > 1.5) {
                                int slice = i < 0 ? 0 : (i > 15 ? 15 : i);
                                cookie = CookieArray.SampleLevel(LinearSampler, float3(cuv, (float) slice), 0).rgb;
                            } else {
                                cookie = CookieTX.SampleLevel(LinearSampler, cuv, 0).rgb;
                            }
                        }
                    }
                }
            } else {
                if (ShadowInfo.y > 0.5 && PointMeta.y > 0.5 && (int)(PointMeta.x + 0.5) == i) {
                    lightShadow = SamplePointPCSS(wposB, n, PointMeta.w);
                }
            }
            col += ShadeFwd(n, V, L, LightC[i].rgb, alb, rough, metal, F0, celInfoF) * att * spot * lightShadow * cookie;
            col += ShadeSssFwd(n, L, LightC[i].rgb, alb, SssPrm.rgb, sssAmtF, sssWrapF) * att * spot * lightShadow * cookie * (1.0 - celOnF);
            col += ShadeHairFwd(n, fwdTan, V, L, LightC[i].rgb, SssPrm.rgb, isHairF * sssAmtF, 0.15) * att * spot * lightShadow * cookie * (1.0 - celOnF);
        }
    }
    float sint2 = max(SkySun.w, 1e-3);
    float3 hemi = SkyGradient(n, normalize(SkySun.xyz), SkyColor.rgb, sint2);
    float celAmbientF = 1.0;
    if (celOnF > 0.5) {
        float nduF = saturate(dot(n, float3(0.0, 1.0, 0.0)) * 0.5 + 0.5);
        float ambBandF = floor(nduF * celInfoF.y + 0.5) / celInfoF.y;
        celAmbientF = 0.3 + 0.9 * ambBandF;
    }
    if (isSkinF > 0.5 && celOnF < 0.5) {
        float rimF = pow(1.0 - saturate(dot(n, V)), 3.0);
        col += SssPrm.rgb * (rimF * sssAmtF * 0.35) * LightColor.rgb;
    }
    if (isEyeF > 0.5 && celOnF < 0.5) {
        float3 HeF = normalize(Lsun + V);
        float egF = pow(saturate(dot(n, HeF)), 400.0) * 2.0;
        float fresF = pow(1.0 - saturate(dot(n, V)), 5.0);
        col += (egF + fresF * 0.3) * sssAmtF * LightColor.rgb;
    }
    col += alb * hemi * 0.5 * celAmbientF;
    float3 R = reflect(-V, n);
    float3 cubeEnv = EnvTX.SampleLevel(GeoSamp, R, clamp(rough, 0.0, 1.0) * 6.0).rgb * GradeInfo.z;
    float3 F0e = lerp(F0, alb, metal);
    float3 fres = F0e + (1.0 - F0e) * pow(1.0 - saturate(dot(n, V)), 5.0);
    col += cubeEnv * fres * (1.0 - rough) * (1.0 - celOnF);
    float T = ExtF.w;
    T = clamp(T, 0.0, 1.0);
    float thick = ExtH.w;
    if ((flags & 131072u) != 0u) {
        T *= TransTex.Sample(GeoSamp, XformUv(pin.uv, pin.uv2, 0)).r;
    }
    if ((flags & 262144u) != 0u) {
        thick *= ThickTex.Sample(GeoSamp, XformUv(pin.uv, pin.uv2, 0)).g;
    }
    if (T > 0.0) {
        float eta = 1.0 / iorSafe;
        float3 refr = refract(-V, n, eta);
        float2 suv = pin.pos.xy / ScreenWH.xy;
        float2 off = refr.xy * thick * 0.5;
        float rad = rough * 6.0 / max(ScreenWH.x, 1.0);
        float3 scene = TransSrc.Sample(GeoSamp, suv + off).rgb * 0.4;
        scene += TransSrc.Sample(GeoSamp, suv + off + float2(rad, 0.0)).rgb * 0.15;
        scene += TransSrc.Sample(GeoSamp, suv + off - float2(rad, 0.0)).rgb * 0.15;
        scene += TransSrc.Sample(GeoSamp, suv + off + float2(0.0, rad)).rgb * 0.15;
        scene += TransSrc.Sample(GeoSamp, suv + off - float2(0.0, rad)).rgb * 0.15;
        float3 absorb = float3(1.0, 1.0, 1.0);
        if (ExtD.w > 1e-4) {
            float3 sigma = (float3(1.0, 1.0, 1.0) - ExtH.rgb) * thick / max(ExtD.w, 1e-4);
            absorb = exp(-max(sigma, 0.0));
        }
        float3 transmitted = scene * absorb * T;
        col = col * (1.0 - T) + transmitted;
    }
    col += emi;
    if ((flags & 16u) != 0u) {
        col += alb * LightmapTex.Sample(GeoSamp, pin.luv).rgb * Extra.x;
    }
    if (FogInfo.x > 0.5) {
        float3 toFrag = pin.wpos - CamPos.xyz;
        float fragDist = length(toFrag);
        if (fragDist > 1e-3) {
            col = ApplyFog(col, toFrag / fragDist, fragDist);
        }
    }
    float outA = alpha;
    if (T > 0.0) {
        outA = 1.0;
    }
    return float4(col, outA);
}
