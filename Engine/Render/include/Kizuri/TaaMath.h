#pragma once
#include <cmath>
namespace Kizuri {
struct TaaCameraBasis {
  float pos[3];
  float fwd[3];
  float right[3];
  float up[3];
  float tanW;
  float tanH;
};
inline void TaaStableWorldPosition(const TaaCameraBasis& cam, float u, float v, float viewDepth, float out[3]) {
  const float sx = u * 2.0f - 1.0f;
  const float sy = 1.0f - v * 2.0f;
  float dx = cam.fwd[0] + cam.right[0] * (sx * cam.tanW) + cam.up[0] * (sy * cam.tanH);
  float dy = cam.fwd[1] + cam.right[1] * (sx * cam.tanW) + cam.up[1] * (sy * cam.tanH);
  float dz = cam.fwd[2] + cam.right[2] * (sx * cam.tanW) + cam.up[2] * (sy * cam.tanH);
  float len = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (len < 1e-8f) {
    dx = cam.fwd[0];
    dy = cam.fwd[1];
    dz = cam.fwd[2];
    len = 1.0f;
  }
  dx /= len;
  dy /= len;
  dz /= len;
  float denom = dx * cam.fwd[0] + dy * cam.fwd[1] + dz * cam.fwd[2];
  if (denom < 1e-4f) {
    denom = 1e-4f;
  }
  const float t = viewDepth / denom;
  out[0] = cam.pos[0] + dx * t;
  out[1] = cam.pos[1] + dy * t;
  out[2] = cam.pos[2] + dz * t;
}
}
