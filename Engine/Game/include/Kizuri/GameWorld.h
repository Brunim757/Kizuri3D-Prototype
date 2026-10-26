#pragma once
#include "Kizuri/Audio.h"
#include "Kizuri/Assets/AssetDatabase.h"
#include "Kizuri/Camera.h"
#include "Kizuri/DeferredRenderer.h"
#include "Kizuri/Ecs.h"
#include "Kizuri/JobSystem.h"
#include "Kizuri/PhysicsWorld.h"
#include "Kizuri/QualityPreset.h"
#include "Kizuri/Scene.h"
#include <string>
#include <unordered_map>
#include <vector>
namespace Kizuri {
bool SubmitSceneLights(DeferredRenderer& renderer, const Scene& scene, const EntityId& soloLight);
bool SubmitSceneLights(DeferredRenderer& renderer, const Scene& scene, const EntityId& soloLight, const std::vector<EntityId>& ids);
struct GameWorldOptions {
  bool loadAssets = true;
  bool enablePhysics = true;
  bool enableAudio = true;
  bool lowSpec = false;
  float renderScale = 1.0f;
  int shadowAtlasSize = 4096;
};
void ApplyQualityToRenderer(DeferredRenderer& renderer, const QualitySettings& q);
class GameWorld {
public:
  GameWorld();
  ~GameWorld();
  bool Initialize(IRHI* rhi, int width, int height, const std::string& shaderDir, const std::string& assetsDir, const GameWorldOptions& options);
  void Shutdown();
  bool LoadSceneFile(const std::string& path);
  void CreateFallbackScene();
  void SetViewSize(int width, int height);
  void SetCameraPosition(float x, float y, float z);
  void SetCameraYawPitch(float yaw, float pitch);
  void Update(float dt);
  void Render(float alpha);
  void SyncPhysics(float dt);
  void SyncAudio(float dt);
  Scene& GetScene() { return scene; }
  const Scene& GetScene() const { return scene; }
  EcsWorld& GetEcs() { return ecs; }
  const EcsWorld& GetEcs() const { return ecs; }
  DeferredRenderer& GetRenderer() { return renderer; }
  PhysicsWorld& GetPhysics() { return physics; }
  AudioEngine& GetAudio() { return audio; }
  FreeCamera& GetCamera() { return camera; }
  AssetDatabase& GetAssets() { return assets; }
  JobSystem& GetJobs() { return jobs; }
  const std::string& LastError() const { return lastError; }
  bool IsInitialized() const { return initialized; }
private:
  Scene scene;
  EcsWorld ecs;
  EcsSystemScheduler ecsSystems;
  DeferredRenderer renderer;
  AssetDatabase assets;
  JobSystem jobs;
  PhysicsWorld physics;
  AudioEngine audio;
  FreeCamera camera;
  std::string lastError;
  std::unordered_map<EntityId, uint32_t, EntityIdHash> physBodies;
  struct AudioEntry {
    uint32_t sourceId = 0;
    uint32_t clipId = 0xFFFFFFFFu;
    std::string clipPath;
    float volume = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 20.0f;
    float rolloff = 1.0f;
    int rolloffModel = 1;
    bool playing = true;
    bool spatial = true;
    bool loop = false;
    int bus = 0;
  };
  std::unordered_map<EntityId, AudioEntry, EntityIdHash> audioSources;
  std::unordered_map<std::string, uint32_t> audioClipCache;
  std::unordered_map<EntityId, float, EntityIdHash> decalAge;
  std::string lastDecalKey;
  std::vector<std::string> lastDecalGuids;
  std::vector<float> lastDecalRects;
  std::string lastCookieKey;
  float baseExposure = 1.0f;
  int baseToneMode = 0;
  float baseContrast = 1.0f;
  float baseSaturation = 1.0f;
  float baseTemp = 0.0f;
  float baseTint = 0.0f;
  float baseBloom = 0.5f;
  bool postVolumeActive = false;
  float lastDelta = 0.0f;
  std::string assetsRoot;
  void ClearAudioSources();
  void SubmitDecals(const std::vector<EntityId>& ids, float dt);
  void SubmitSceneCookies(const std::vector<EntityId>& ids);
  void SubmitScenePost(const std::vector<EntityId>& ids, const float camPos[3]);
  bool initialized = false;
  int viewWidth = 1280;
  int viewHeight = 720;
};
}