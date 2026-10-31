#pragma once
#include "Kizuri/DeferredRenderer.h"
#include "Kizuri/Scene.h"
#include <vector>
namespace Kizuri {
struct PostSettings {
  float exposure = 1.0f;
  int toneMap = 0;
  float contrast = 1.0f;
  float saturation = 1.0f;
  float temperature = 0.0f;
  float tint = 0.0f;
  float bloom = 0.5f;
};
PostSettings MakePostSettings(float exposure, int toneMap, float contrast, float saturation, float temperature, float tint, float bloom);
PostSettings CapturePostSettings(const DeferredRenderer& renderer);
PostSettings ResolvePostSettings(const PostSettings& base, const PostData* volume);
const PostData* FindActivePostVolume(const Scene& scene, const std::vector<EntityId>& ids, const float position[3]);
void ApplyPostSettings(DeferredRenderer& renderer, const PostSettings& settings);
}
