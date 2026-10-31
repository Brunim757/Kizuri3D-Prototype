#include "Kizuri/PostProcess.h"
#include <cmath>
namespace Kizuri {
namespace {
float Clamp(float value, float low, float high) {
  if (value < low) {
    return low;
  }
  if (value > high) {
    return high;
  }
  return value;
}
int ClampToneMap(int value) {
  if (value < 0) {
    return 0;
  }
  if (value > 3) {
    return 3;
  }
  return value;
}
}
PostSettings MakePostSettings(float exposure, int toneMap, float contrast, float saturation, float temperature, float tint, float bloom) {
  PostSettings s;
  s.exposure = Clamp(exposure, 0.05f, 8.0f);
  s.toneMap = ClampToneMap(toneMap);
  s.contrast = Clamp(contrast, 0.0f, 2.0f);
  s.saturation = Clamp(saturation, 0.0f, 3.0f);
  s.temperature = Clamp(temperature, -1.0f, 1.0f);
  s.tint = Clamp(tint, -1.0f, 1.0f);
  s.bloom = Clamp(bloom, 0.0f, 2.0f);
  return s;
}
PostSettings CapturePostSettings(const DeferredRenderer& renderer) {
  PostSettings s;
  s.exposure = renderer.Exposure();
  s.toneMap = renderer.ToneMappingMode();
  s.contrast = renderer.GradeContrast();
  s.saturation = renderer.GradeSaturation();
  s.temperature = renderer.GradeTemperature();
  s.tint = renderer.GradeTint();
  s.bloom = renderer.BloomStrength();
  return s;
}
PostSettings ResolvePostSettings(const PostSettings& base, const PostData* volume) {
  PostSettings s = base;
  if (volume != nullptr) {
    s.exposure = volume->exposure;
    s.toneMap = volume->toneMap;
    s.contrast = volume->contrast;
    s.saturation = volume->saturation;
    s.temperature = base.temperature + volume->temperature;
    s.tint = base.tint + volume->tint;
    s.bloom = volume->bloom;
  }
  s.exposure = Clamp(s.exposure, 0.05f, 8.0f);
  s.toneMap = ClampToneMap(s.toneMap);
  s.contrast = Clamp(s.contrast, 0.0f, 2.0f);
  s.saturation = Clamp(s.saturation, 0.0f, 3.0f);
  s.temperature = Clamp(s.temperature, -1.0f, 1.0f);
  s.tint = Clamp(s.tint, -1.0f, 1.0f);
  s.bloom = Clamp(s.bloom, 0.0f, 2.0f);
  return s;
}
const PostData* FindActivePostVolume(const Scene& scene, const std::vector<EntityId>& ids, const float position[3]) {
  if (position == nullptr) {
    return nullptr;
  }
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasPost) {
      continue;
    }
    const float hx = e->post.size[0] * 0.5f;
    const float hy = e->post.size[1] * 0.5f;
    const float hz = e->post.size[2] * 0.5f;
    if (std::fabs(position[0] - e->transform.position[0]) <= hx &&
        std::fabs(position[1] - e->transform.position[1]) <= hy &&
        std::fabs(position[2] - e->transform.position[2]) <= hz) {
      return &e->post;
    }
  }
  return nullptr;
}
void ApplyPostSettings(DeferredRenderer& renderer, const PostSettings& settings) {
  renderer.SetGrade(settings.exposure, settings.toneMap == 0);
  renderer.SetToneMapping(settings.toneMap);
  renderer.SetGrading(settings.contrast, settings.saturation, settings.temperature, settings.tint);
  renderer.SetBloom(settings.bloom);
}
}
