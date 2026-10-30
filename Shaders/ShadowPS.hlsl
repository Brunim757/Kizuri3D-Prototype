cbuffer ShadowCB : register(b0)
{
    float4 Misc;
    float4 UvA;
    float4 UvB;
    float4 Extra;
};
Texture2D AlbedoTex : register(t0);
Texture2D VtTail : register(t1);
Texture2D VtTable : register(t2);
SamplerState GeoSamp : register(s0);
struct PSIn
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD1;
    float2 uv2 : TEXCOORD5;
};
float2 XformUv(float2 uv, float2 uv2)
{
    float2 base = UvB.y > 0.5 ? uv2 : uv;
    float2 scaled = base * UvA.zw;
    float c = cos(UvB.x);
    float s = sin(UvB.x);
    float2 rotated = float2(scaled.x * c - scaled.y * s, scaled.x * s + scaled.y * c);
    return rotated + UvA.xy;
}
void main(PSIn pin)
{
    float fade = Extra.x;
    if (fade > 0.0 && fade < 1.0) {
        float2 cell = floor(pin.pos.xy);
        float h = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);
        if (h > fade) {
            discard;
        }
    }
    int alphaMode = (int) Misc.y;
    if (alphaMode == 1) {
        uint flags = asuint(Misc.z);
        float alpha = Misc.w;
        bool triOn = ((flags & 536870912u) != 0u);
        if ((flags & 1u) != 0u && !triOn) {
            float2 pov = XformUv(pin.uv, pin.uv2);
            if ((flags & 134217728u) != 0u) {
                float2 vtPage = floor(pov * 64.0);
                float2 vtFrac = frac(pov * 64.0);
                float4 ve = VtTable.Load(int3((int) min(vtPage.x, 63.0), (int) min(vtPage.y, 63.0), 0));
                if (ve.b > 0.5 && abs(ve.a * 255.0 - Extra.y) < 0.5) {
                    float2 phys = float2((ve.r * 255.0 * 34.0 + 1.0 + vtFrac.x * 32.0) / 2048.0, (ve.g * 255.0 * 34.0 + 1.0 + vtFrac.y * 32.0) / 1024.0);
                    alpha *= AlbedoTex.SampleLevel(GeoSamp, phys, 0).a;
                } else {
                    alpha *= VtTail.SampleLevel(GeoSamp, pov, 0).a;
                }
            } else {
                alpha *= AlbedoTex.Sample(GeoSamp, pov).a;
            }
        }
        if (alpha < Misc.x) {
            discard;
        }
    }
}
