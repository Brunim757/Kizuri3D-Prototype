#include "Kizuri/GameWorld.h"
#include "Kizuri/GameMaterial.h"
#include "Kizuri/SceneSerializer.h"
#include <DirectXMath.h>
#include <cmath>
#include <filesystem>
#include <unordered_map>
#include <vector>
namespace Kizuri {
static void UpdateVirtualTextureForDraw(AssetDatabase& assets, const DeferredMaterial* material, const float world[16], const float camPos[3]) {
  if (material == nullptr || material->vtGuid.empty()) {
    return;
  }
  const float dx = world[12] - camPos[0];
  const float dy = world[13] - camPos[1];
  const float dz = world[14] - camPos[2];
  assets.UpdateVirtualTexture(material->vtGuid, std::sqrt(dx * dx + dy * dy + dz * dz));
}
static void SubmitSceneFog(DeferredRenderer& renderer, const Scene& scene) {
  renderer.ClearFogVolumes();
  const std::vector<EntityId> ids = scene.All();
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasFog) {
      continue;
    }
    float bmin[3] = {
      e->transform.position[0] - e->fog.size[0] * 0.5f,
      e->transform.position[1] - e->fog.size[1] * 0.5f,
      e->transform.position[2] - e->fog.size[2] * 0.5f
    };
    float bmax[3] = {
      e->transform.position[0] + e->fog.size[0] * 0.5f,
      e->transform.position[1] + e->fog.size[1] * 0.5f,
      e->transform.position[2] + e->fog.size[2] * 0.5f
    };
    renderer.AddFogVolume(bmin, bmax, e->fog.density, e->fog.color);
  }
}
void GameWorld::SubmitScenePost(const std::vector<EntityId>& ids, const float camPos[3]) {
  const PostData* active = nullptr;
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasPost) {
      continue;
    }
    const float hx = e->post.size[0] * 0.5f;
    const float hy = e->post.size[1] * 0.5f;
    const float hz = e->post.size[2] * 0.5f;
    if (std::fabs(camPos[0] - e->transform.position[0]) <= hx &&
        std::fabs(camPos[1] - e->transform.position[1]) <= hy &&
        std::fabs(camPos[2] - e->transform.position[2]) <= hz) {
      active = &e->post;
      break;
    }
  }
  if (active == nullptr) {
    renderer.SetGrade(baseExposure, baseToneMode == 0);
    renderer.SetToneMapping(baseToneMode);
    renderer.SetGrading(baseContrast, baseSaturation, 0.0f, 0.0f);
    renderer.SetBloom(baseBloom);
    return;
  }
  renderer.SetGrade(active->exposure, active->toneMap == 0);
  renderer.SetToneMapping(active->toneMap);
  renderer.SetGrading(active->contrast, active->saturation, 0.0f, 0.0f);
  renderer.SetBloom(active->bloom);
}
static void SelectLodGuids(AssetDatabase& assets, const DeferredRenderer& renderer, const Entity& entity, const float camPos[3], std::string guids[2], float fades[2], int& count) {
  guids[0] = entity.meshGuid;
  guids[1].clear();
  fades[0] = 0.0f;
  fades[1] = 0.0f;
  count = 1;
  if (!entity.hasLod || entity.meshGuid.empty()) {
    return;
  }
  const MeshRecord* base = assets.GetByGuid(entity.meshGuid);
  if (base == nullptr) {
    return;
  }
  const float dx = entity.transform.position[0] - camPos[0];
  const float dy = entity.transform.position[1] - camPos[1];
  const float dz = entity.transform.position[2] - camPos[2];
  const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (entity.lod.farGuid.empty() && entity.lod.farDist > entity.lod.nearDist) {
    int tierA = 0;
    int tierB = -1;
    float fadeB = 0.0f;
    const float rx = base->data.aabbMax[0] - base->data.aabbMin[0];
    const float ry = base->data.aabbMax[1] - base->data.aabbMin[1];
    const float rz = base->data.aabbMax[2] - base->data.aabbMin[2];
    const float radius = 0.5f * std::sqrt(rx * rx + ry * ry + rz * rz);
    if (radius > 0.001f && renderer.MainFovY() > 0.001f && renderer.RenderHeight() > 0) {
      SelectAutoLodCoverage(distance, radius, static_cast<float>(renderer.RenderHeight()), renderer.MainFovY(), tierA, tierB, fadeB);
    } else {
      SelectAutoLod(distance, entity.lod.nearDist, entity.lod.farDist, tierA, tierB, fadeB);
    }
    std::string tierGuids[3];
    tierGuids[0] = entity.meshGuid;
    for (int tier = 1; tier <= 2; ++tier) {
      const std::string siblingPath = LodSiblingPath(base->meshPath, tier);
      if (siblingPath.empty()) {
        continue;
      }
      const MeshRecord* sibling = assets.GetByMeshPath(siblingPath);
      if (sibling != nullptr) {
        tierGuids[tier] = sibling->guid;
      }
    }
    std::string nearGuid = entity.meshGuid;
    for (int tier = tierA; tier >= 0; --tier) {
      if (!tierGuids[tier].empty()) {
        nearGuid = tierGuids[tier];
        break;
      }
    }
    guids[0] = nearGuid;
    if (tierB >= 0) {
      std::string farGuid = entity.meshGuid;
      for (int tier = tierB; tier >= 0; --tier) {
        if (!tierGuids[tier].empty()) {
          farGuid = tierGuids[tier];
          break;
        }
      }
      guids[1] = farGuid;
      fades[0] = 1.0f - fadeB;
      fades[1] = fadeB;
      count = 2;
    }
    return;
  }
  if (!entity.lod.farGuid.empty() && entity.lod.farGuid != entity.meshGuid && entity.lod.farDist > entity.lod.nearDist) {
    const float blend = (distance - entity.lod.nearDist) / (entity.lod.farDist - entity.lod.nearDist);
    if (blend >= 1.0f) {
      guids[0] = entity.lod.farGuid;
    } else if (blend > 0.0f) {
      guids[1] = entity.lod.farGuid;
      fades[0] = 1.0f - blend;
      fades[1] = blend;
      count = 2;
    }
  }
}
void GameWorld::SubmitDecals(const std::vector<EntityId>& ids, float dt) {
  std::string decalKey;
  std::vector<const TextureAssetData*> decalTextures;
  lastDecalGuids.clear();
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasDecal || e->decal.texGuid.empty()) {
      continue;
    }
    decalKey += e->decal.texGuid + "|";
    lastDecalGuids.push_back(e->decal.texGuid);
    const TextureRecord* tr = assets.GetTexByGuid(e->decal.texGuid);
    if (tr != nullptr) {
      decalKey += (tr->loaded ? "1|" : "0|");
    }
    decalTextures.push_back((tr != nullptr && tr->loaded) ? &tr->data : nullptr);
    if (decalTextures.size() >= 16) {
      break;
    }
  }
  if (decalKey != lastDecalKey) {
    lastDecalKey = decalKey;
    lastDecalRects.clear();
    RHITexture atlas = decalTextures.empty() ? 0 : assets.BuildDecalAtlas(decalTextures, lastDecalRects);
    renderer.SetDecalAtlas(atlas);
  }
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasDecal || e->decal.texGuid.empty()) {
      continue;
    }
    float age = 0.0f;
    std::unordered_map<EntityId, float, EntityIdHash>::const_iterator ageIt = decalAge.find(e->id);
    if (ageIt != decalAge.end()) {
      age = ageIt->second;
    }
    age += dt;
    decalAge[e->id] = age;
    float fade = 1.0f;
    if (e->decal.life > 0.0f) {
      if (age >= e->decal.life) {
        continue;
      }
      fade = 1.0f - age / e->decal.life;
    }
    RHITexture decalTex = 0;
    if (ResolveMaterialSlot(assets, e->decal.texGuid, 1u, decalTex) == 0) {
      continue;
    }
    Transform decalTransform = e->transform;
    decalTransform.scale[0] *= e->decal.size[0];
    decalTransform.scale[1] *= e->decal.size[1];
    decalTransform.scale[2] *= e->decal.size[2];
    float world[16];
    ComposeMatrix(decalTransform, world);
    float invWorld[16];
    if (!InvertMatrix4(world, invWorld)) {
      continue;
    }
    size_t rectIndex = lastDecalGuids.size();
    for (size_t g = 0; g < lastDecalGuids.size(); ++g) {
      if (lastDecalGuids[g] == e->decal.texGuid) {
        rectIndex = g;
        break;
      }
    }
    if (rectIndex * 4 + 3 < lastDecalRects.size()) {
      const float rect[4] = {
        lastDecalRects[rectIndex * 4 + 0],
        lastDecalRects[rectIndex * 4 + 1],
        lastDecalRects[rectIndex * 4 + 2],
        lastDecalRects[rectIndex * 4 + 3]
      };
      renderer.DrawDecalAtlas(world, invWorld, e->decal.size, fade, rect, e->decal.matMask, e->decal.matRough, e->decal.matMetal);
    } else {
      renderer.DrawDecal(world, invWorld, e->decal.size, fade, decalTex, e->decal.matMask, e->decal.matRough, e->decal.matMetal);
    }
  }
  for (std::unordered_map<EntityId, float, EntityIdHash>::iterator it = decalAge.begin(); it != decalAge.end();) {
    if (!scene.Has(it->first)) {
      it = decalAge.erase(it);
    } else {
      ++it;
    }
  }
}
void GameWorld::SubmitSceneCookies(const std::vector<EntityId>& ids) {
  size_t pointCount = 0;
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e != nullptr && e->hasLight && e->light.type == static_cast<int>(LightType::Point)) {
      ++pointCount;
    }
  }
  std::vector<const TextureAssetData*> slices(16, nullptr);
  std::string key = std::to_string(pointCount) + "|";
  size_t spotOrd = 0;
  bool anyLoaded = false;
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasLight || e->light.type != static_cast<int>(LightType::Spot)) {
      continue;
    }
    const size_t slice = pointCount + spotOrd;
    ++spotOrd;
    key += e->light.cookieGuid;
    key += "|";
    if (slice >= 16 || e->light.cookieGuid.empty()) {
      continue;
    }
    const TextureRecord* tr = assets.GetTexByGuid(e->light.cookieGuid);
    if (tr != nullptr && tr->loaded) {
      key += "1|";
      slices[slice] = &tr->data;
      anyLoaded = true;
    } else {
      key += "0|";
    }
  }
  if (key != lastCookieKey) {
    lastCookieKey = key;
    if (anyLoaded) {
      renderer.SetCookieArray(assets.BuildCookieArray(slices));
    } else {
      renderer.SetCookieArray(0);
    }
  }
  renderer.SetSpotCookieOn(anyLoaded);
}
bool SubmitSceneLights(DeferredRenderer& renderer, const Scene& scene, const EntityId& soloLight) {
  renderer.ClearLights();
  bool any = false;
  const std::vector<EntityId> ids = scene.All();
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasLight) {
      continue;
    }
    if (soloLight.IsValid()) {
      if (ids[i] != soloLight) {
        continue;
      }
    }
    any = true;
    if (e->light.type == static_cast<int>(LightType::Point)) {
      RenderPointLight pl;
      pl.pos[0] = e->transform.position[0];
      pl.pos[1] = e->transform.position[1];
      pl.pos[2] = e->transform.position[2];
      pl.color[0] = e->light.color[0];
      pl.color[1] = e->light.color[1];
      pl.color[2] = e->light.color[2];
      pl.intensity = e->light.intensity;
      pl.radius = e->light.radius;
      pl.castShadow = e->light.castShadow;
      pl.effectiveSize = e->light.softness * e->light.lightSize;
      pl.linkMask = static_cast<uint32_t>(e->light.linkMask);
      pl.layer = static_cast<uint32_t>(e->light.layer);
      renderer.AddPointLight(pl);
    } else if (e->light.type == static_cast<int>(LightType::Spot)) {
      RenderSpotLight sl;
      sl.pos[0] = e->transform.position[0];
      sl.pos[1] = e->transform.position[1];
      sl.pos[2] = e->transform.position[2];
      sl.color[0] = e->light.color[0];
      sl.color[1] = e->light.color[1];
      sl.color[2] = e->light.color[2];
      sl.intensity = e->light.intensity;
      sl.radius = e->light.radius;
      sl.angle = e->light.spotAngle;
      sl.falloff = e->light.falloff;
      sl.castShadow = e->light.castShadow;
      sl.effectiveSize = e->light.softness * e->light.lightSize;
      sl.shadowSize = e->light.shadowSize;
      sl.linkMask = static_cast<uint32_t>(e->light.linkMask);
      sl.layer = static_cast<uint32_t>(e->light.layer);
      EntityForward(e->transform, sl.dir);
      renderer.AddSpotLight(sl);
    } else if (e->light.type == static_cast<int>(LightType::Directional)) {
      RenderDirectionalLight dl;
      dl.color[0] = e->light.color[0];
      dl.color[1] = e->light.color[1];
      dl.color[2] = e->light.color[2];
      dl.intensity = e->light.intensity;
      dl.castShadow = e->light.castShadow;
      dl.effectiveSize = e->light.softness * e->light.lightSize;
      dl.shadowSize = e->light.shadowSize;
      dl.cascades = e->light.cascades;
      dl.lambda = e->light.lambda;
      dl.linkMask = static_cast<uint32_t>(e->light.linkMask);
      dl.layer = static_cast<uint32_t>(e->light.layer);
      EntityForward(e->transform, dl.dir);
      renderer.AddDirectionalLight(dl);
    } else if (e->light.type == static_cast<int>(LightType::Rect)) {
      RenderRectLight rl;
      rl.pos[0] = e->transform.position[0];
      rl.pos[1] = e->transform.position[1];
      rl.pos[2] = e->transform.position[2];
      rl.color[0] = e->light.color[0];
      rl.color[1] = e->light.color[1];
      rl.color[2] = e->light.color[2];
      rl.intensity = e->light.intensity;
      rl.halfW = e->light.areaW * 0.5f;
      rl.halfH = e->light.areaH * 0.5f;
      rl.range = e->light.radius;
      rl.linkMask = static_cast<uint32_t>(e->light.linkMask);
      rl.layer = static_cast<uint32_t>(e->light.layer);
      EntityForward(e->transform, rl.normal);
      renderer.AddRectLight(rl);
    }
  }
  return any;
}
GameWorld::GameWorld() {
  const EcsComponentMask bodyMask = EcsBit(kEcsTransform) | EcsBit(ComponentRegistry::kRigidbody);
  const EcsComponentMask audioMask = EcsBit(kEcsTransform) | EcsBit(ComponentRegistry::kAudioSource);
  ecsSystems.AddSystem("physics", bodyMask, bodyMask, [this](EcsWorld&, float dt) {
    SyncPhysics(dt);
  });
  ecsSystems.AddSystem("audio", audioMask, 0, [this](EcsWorld&, float dt) {
    SyncAudio(dt);
  });
}
GameWorld::~GameWorld() {
  Shutdown();
}
void ApplyQualityToRenderer(DeferredRenderer& renderer, const QualitySettings& q) {
  renderer.SetSsao(q.ssao, 1.0f, 1.0f);
  renderer.SetAoMode(q.aoMode);
  renderer.SetSsr(q.ssr);
  renderer.SetSsrTemporal(q.ssrTemporal, 0.9f);
  renderer.SetContact(q.contact);
  renderer.SetSsgi(q.ssgi);
  renderer.SetDof(q.dof, 10.0f, 10.0f);
  renderer.SetMotionBlur(q.motionBlur, 0.5f);
  renderer.SetFxaa(q.fxaa);
  renderer.SetTaa(q.taa);
  renderer.SetTaaSharpening(q.taaSharp);
  renderer.SetBloom(q.bloom);
  renderer.SetShadowFilter(q.shadowFilter);
  renderer.SetShadowRange(q.shadowRange);
  renderer.SetShadowAutoRes(q.shadowAuto);
  renderer.SetVsmQuality(q.vsmQuality);
  renderer.SetFogEnabled(q.fog);
  renderer.SetDynamicResolution(q.dynRes, 16.6f, 0.5f, 1.0f);
  renderer.SetOcclusionCulling(q.occ);
  renderer.SetGodRays(q.godRays, 0.5f, 0.9f, 0.2f);
  renderer.SetFsr(q.renderScale, 0.5f);
}
bool GameWorld::Initialize(IRHI* rhi, int width, int height, const std::string& shaderDir, const std::string& assetsDir, const GameWorldOptions& options) {
  if (initialized) {
    return true;
  }
  if (rhi == nullptr) {
    lastError = "RHI nulo";
    return false;
  }
  viewWidth = width > 0 ? width : 1280;
  viewHeight = height > 0 ? height : 720;
  assetsRoot = assetsDir;
  if (!renderer.Initialize(rhi, viewWidth, viewHeight, shaderDir.c_str())) {
    lastError = "Falha ao inicializar o renderer em " + shaderDir;
    return false;
  }
  if (options.lowSpec) {
    QualitySettings low = ApplyQualityPreset(QualityLevel::Low);
    low.renderScale = options.renderScale > 0.0f ? options.renderScale : 0.5f;
    ApplyQualityToRenderer(renderer, low);
  }
  baseExposure = renderer.Exposure();
  baseToneMode = renderer.ToneMappingMode();
  baseContrast = renderer.GradeContrast();
  baseSaturation = renderer.GradeSaturation();
  baseBloom = renderer.BloomStrength();
  if (options.loadAssets && !assetsDir.empty()) {
    assets.SetAssetsDir(assetsDir);
    assets.SetGpuRHI(rhi);
    assets.Scan();
    assets.DrainBlocking();
  }
  if (!jobs.Initialize()) {
    lastError = "Falha ao iniciar o Job System";
    return false;
  }
  if (options.enablePhysics && !physics.Initialize(&jobs)) {
    lastError = "Falha ao inicializar a fisica";
    return false;
  }
  if (options.enableAudio) {
    audio.Initialize(48000, 2);
  }
  camera.SetPosition(0.0f, 4.0f, -10.0f);
  camera.SetYawPitch(0.0f, 0.15f);
  initialized = true;
  return true;
}
void GameWorld::Shutdown() {
  if (!initialized) {
    return;
  }
  initialized = false;
  audioSources.clear();
  audioClipCache.clear();
  decalAge.clear();
  lastDecalKey.clear();
  lastDecalGuids.clear();
  lastDecalRects.clear();
  lastCookieKey.clear();
  lastDelta = 0.0f;
  assetsRoot.clear();
  audio.Shutdown();
  physics.Shutdown();
  physBodies.clear();
  jobs.Shutdown();
  ecs.Clear();
  assets.SetGpuRHI(nullptr);
  renderer.Shutdown();
}
bool GameWorld::LoadSceneFile(const std::string& path) {
  if (path.empty()) {
    return false;
  }
  if (!LoadSceneFromFile(scene, path, nullptr)) {
    lastError = "Falha ao carregar a cena " + path;
    return false;
  }
  for (std::unordered_map<EntityId, uint32_t, EntityIdHash>::const_iterator it = physBodies.begin(); it != physBodies.end(); ++it) {
    physics.DestroyBody(it->second);
  }
  physBodies.clear();
  ClearAudioSources();
  decalAge.clear();
  lastDecalKey.clear();
  lastDecalGuids.clear();
  lastDecalRects.clear();
  lastCookieKey.clear();
  SceneEcsBridge::Import(scene, ecs);
  return true;
}
void GameWorld::CreateFallbackScene() {
  EntityId id = scene.CreateEntity("Directional");
  Entity* e = scene.Get(id);
  if (e == nullptr) {
    return;
  }
  e->hasLight = true;
  MakeDefaultLight(e->light);
  e->light.type = static_cast<int>(LightType::Directional);
  e->light.intensity = 3.0f;
  e->light.color[0] = 1.0f;
  e->light.color[1] = 0.96f;
  e->light.color[2] = 0.9f;
  e->transform.rotation[1] = 45.0f;
  e->transform.rotation[0] = -35.0f;
  SceneEcsBridge::Import(scene, ecs);
}
void GameWorld::SetViewSize(int width, int height) {
  if (width > 0 && height > 0) {
    viewWidth = width;
    viewHeight = height;
  }
}
void GameWorld::SetCameraPosition(float x, float y, float z) {
  camera.SetPosition(x, y, z);
}
void GameWorld::SetCameraYawPitch(float yaw, float pitch) {
  camera.SetYawPitch(yaw, pitch);
}
void GameWorld::Update(float dt) {
  if (!initialized) {
    return;
  }
  lastDelta = dt < 0.0f ? 0.0f : dt;
  ecsSystems.Run(ecs, jobs, dt);
}
void GameWorld::SyncPhysics(float dt) {
  if (!initialized || !physics.IsInitialized()) {
    return;
  }
  if (ecs.Count() != scene.Count()) {
    SceneEcsBridge::Import(scene, ecs);
  }
  const EcsComponentMask bodyMask = EcsBit(kEcsTransform) | EcsBit(ComponentRegistry::kRigidbody);
  ecs.Query(bodyMask).Each([&](EcsQueryEntity entity) {
    Transform* transform = entity.Get<Transform>(kEcsTransform);
    RigidbodyData* rigidbody = entity.Get<RigidbodyData>(ComponentRegistry::kRigidbody);
    if (transform == nullptr || rigidbody == nullptr) {
      return;
    }
    float quat[4];
    EulerDegToQuat(transform->rotation, quat);
    uint32_t body = 0;
    std::unordered_map<EntityId, uint32_t, EntityIdHash>::const_iterator it = physBodies.find(entity.Id());
    if (it != physBodies.end() && physics.HasBody(it->second)) {
      body = it->second;
    }
    if (body == 0) {
      PhysBodyDesc desc;
      desc.motion = rigidbody->motionType == 0 ? PhysMotionType::Static : (rigidbody->motionType == 1 ? PhysMotionType::Kinematic : PhysMotionType::Dynamic);
      desc.mass = rigidbody->mass;
      desc.collider = rigidbody->colliderType == 1 ? PhysColliderType::Sphere : (rigidbody->colliderType == 2 ? PhysColliderType::Capsule : (rigidbody->colliderType == 3 ? PhysColliderType::Mesh : PhysColliderType::Box));
      desc.extents[0] = rigidbody->extents[0];
      desc.extents[1] = rigidbody->extents[1];
      desc.extents[2] = rigidbody->extents[2];
      desc.scale[0] = transform->scale[0];
      desc.scale[1] = transform->scale[1];
      desc.scale[2] = transform->scale[2];
      desc.friction = rigidbody->friction;
      desc.restitution = rigidbody->restitution;
      if (rigidbody->colliderType == 3) {
        const std::string* meshGuid = entity.Get<std::string>(ComponentRegistry::kMesh);
        const MeshRecord* meshRecord = meshGuid != nullptr && !meshGuid->empty() ? assets.GetByGuid(*meshGuid) : nullptr;
        if (meshRecord == nullptr || !meshRecord->loaded) {
          return;
        }
        desc.meshVerts = meshRecord->data.positions.data();
        desc.meshVertCount = meshRecord->data.positions.size() / 3;
        desc.meshIndices = meshRecord->data.indices.data();
        desc.meshIndexCount = meshRecord->data.indices.size();
      }
      const uint32_t created = physics.CreateBody(desc, transform->position, quat);
      if (created == 0xFFFFFFFFu) {
        return;
      }
      physBodies[entity.Id()] = created;
      body = created;
    }
    if (rigidbody->motionType != 2) {
      physics.TeleportBody(body, transform->position, quat);
    }
    if (rigidbody->motionType == 1) {
      physics.SetKinematicTarget(body, transform->position, quat);
    }
  });
  for (std::unordered_map<EntityId, uint32_t, EntityIdHash>::iterator it = physBodies.begin(); it != physBodies.end();) {
    if (!ecs.HasComponent(it->first, ComponentRegistry::kRigidbody)) {
      physics.DestroyBody(it->second);
      it = physBodies.erase(it);
    } else {
      ++it;
    }
  }
  physics.Step(dt);
  for (std::unordered_map<EntityId, uint32_t, EntityIdHash>::const_iterator it = physBodies.begin(); it != physBodies.end(); ++it) {
    Transform* transform = static_cast<Transform*>(ecs.GetComponent(it->first, kEcsTransform));
    RigidbodyData* rigidbody = static_cast<RigidbodyData*>(ecs.GetComponent(it->first, ComponentRegistry::kRigidbody));
    if (transform == nullptr || rigidbody == nullptr || rigidbody->motionType != 2) {
      continue;
    }
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    float quat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (!physics.GetPose(it->second, pos, quat)) {
      continue;
    }
    transform->position[0] = pos[0];
    transform->position[1] = pos[1];
    transform->position[2] = pos[2];
    QuatToEulerDeg(quat, transform->rotation);
  }
  SceneEcsBridge::ExportTransforms(ecs, scene);
}
void GameWorld::ClearAudioSources() {
  if (audio.IsInitialized()) {
    for (std::unordered_map<EntityId, AudioEntry, EntityIdHash>::const_iterator it = audioSources.begin(); it != audioSources.end(); ++it) {
      audio.RemoveSource(it->second.sourceId);
    }
  }
  audioSources.clear();
}
void GameWorld::SyncAudio(float dt) {
  (void)dt;
  if (!initialized || !audio.IsInitialized()) {
    return;
  }
  float camX = 0.0f;
  float camY = 0.0f;
  float camZ = 0.0f;
  camera.GetPosition(camX, camY, camZ);
  float yaw = 0.0f;
  float pitch = 0.0f;
  camera.GetYawPitch(yaw, pitch);
  const float listenerPos[3] = { camX, camY, camZ };
  const float listenerFwd[3] = { std::sin(yaw) * std::cos(pitch), -std::sin(pitch), std::cos(yaw) * std::cos(pitch) };
  const float listenerUp[3] = { 0.0f, 1.0f, 0.0f };
  audio.SetListener(listenerPos, listenerFwd, listenerUp);
  const std::vector<EntityId> all = scene.All();
  for (size_t i = 0; i < all.size(); ++i) {
    Entity* e = scene.Get(all[i]);
    if (e == nullptr || !e->hasAudioSource) {
      continue;
    }
    const AudioSourceData& a = e->audioSource;
    std::unordered_map<EntityId, AudioEntry, EntityIdHash>::iterator it = audioSources.find(e->id);
    if (it == audioSources.end()) {
      if (a.clipPath.empty()) {
        continue;
      }
      uint32_t clipId = 0xFFFFFFFFu;
      std::unordered_map<std::string, uint32_t>::const_iterator cached = audioClipCache.find(a.clipPath);
      if (cached != audioClipCache.end()) {
        clipId = cached->second;
      } else {
        std::string full = a.clipPath;
        if (!assetsRoot.empty() && !std::filesystem::path(a.clipPath).is_absolute()) {
          full = (std::filesystem::path(assetsRoot) / a.clipPath).string();
        }
        uint32_t loaded = 0xFFFFFFFFu;
        if (audio.LoadClip(full, loaded)) {
          audioClipCache[a.clipPath] = loaded;
          clipId = loaded;
        }
      }
      if (clipId == 0xFFFFFFFFu) {
        continue;
      }
      const uint32_t bus = static_cast<uint32_t>(a.bus < 0 ? 0 : a.bus);
      const uint32_t sourceId = audio.AddSource(clipId, bus);
      if (sourceId == 0) {
        continue;
      }
      AudioEntry entry;
      entry.sourceId = sourceId;
      entry.clipId = clipId;
      entry.clipPath = a.clipPath;
      entry.volume = a.volume;
      entry.minDistance = a.minDistance;
      entry.maxDistance = a.maxDistance;
      entry.rolloff = a.rolloff;
      entry.rolloffModel = a.rolloffModel;
      entry.playing = a.playing;
      entry.spatial = a.spatial;
      entry.loop = a.loop;
      entry.bus = a.bus;
      audio.SetSourceGain(sourceId, a.volume);
      audio.SetSourcePlaying(sourceId, a.playing);
      audio.SetSourceSpatial(sourceId, a.spatial);
      audio.SetSourceLoop(sourceId, a.loop);
      audio.SetSourceBus(sourceId, bus);
      audio.SetSourceDistance(sourceId, a.minDistance, a.maxDistance);
      const int model = a.rolloffModel < 0 ? 0 : (a.rolloffModel > 2 ? 2 : a.rolloffModel);
      audio.SetSourceRolloff(sourceId, static_cast<AudioRolloffModel>(model), a.rolloff);
      if (a.spatial) {
        audio.SetSourcePosition(sourceId, e->transform.position);
      }
      audioSources[e->id] = entry;
      it = audioSources.find(e->id);
    }
    AudioEntry& entry = it->second;
    if (entry.clipPath != a.clipPath) {
      audio.RemoveSource(entry.sourceId);
      audioSources.erase(it);
      continue;
    }
    if (entry.volume != a.volume) {
      audio.SetSourceGain(entry.sourceId, a.volume);
      entry.volume = a.volume;
    }
    if (entry.playing != a.playing) {
      audio.SetSourcePlaying(entry.sourceId, a.playing);
      entry.playing = a.playing;
    }
    if (entry.spatial != a.spatial) {
      audio.SetSourceSpatial(entry.sourceId, a.spatial);
      entry.spatial = a.spatial;
    }
    if (entry.loop != a.loop) {
      audio.SetSourceLoop(entry.sourceId, a.loop);
      entry.loop = a.loop;
    }
    if (entry.bus != a.bus) {
      audio.SetSourceBus(entry.sourceId, static_cast<uint32_t>(a.bus < 0 ? 0 : a.bus));
      entry.bus = a.bus;
    }
    if (entry.minDistance != a.minDistance || entry.maxDistance != a.maxDistance) {
      audio.SetSourceDistance(entry.sourceId, a.minDistance, a.maxDistance);
      entry.minDistance = a.minDistance;
      entry.maxDistance = a.maxDistance;
    }
    if (entry.rolloff != a.rolloff || entry.rolloffModel != a.rolloffModel) {
      const int model = a.rolloffModel < 0 ? 0 : (a.rolloffModel > 2 ? 2 : a.rolloffModel);
      audio.SetSourceRolloff(entry.sourceId, static_cast<AudioRolloffModel>(model), a.rolloff);
      entry.rolloff = a.rolloff;
      entry.rolloffModel = a.rolloffModel;
    }
    if (a.spatial) {
      audio.SetSourcePosition(entry.sourceId, e->transform.position);
    }
  }
  for (std::unordered_map<EntityId, AudioEntry, EntityIdHash>::iterator it = audioSources.begin(); it != audioSources.end();) {
    Entity* e = scene.Get(it->first);
    if (e == nullptr || !e->hasAudioSource) {
      audio.RemoveSource(it->second.sourceId);
      it = audioSources.erase(it);
    } else {
      ++it;
    }
  }
  audio.Update();
}
void GameWorld::Render(float alpha) {
  (void)alpha;
  if (!initialized) {
    return;
  }
  float view[16];
  float proj[16];
  const float aspect = static_cast<float>(viewWidth) / (viewHeight > 0 ? static_cast<float>(viewHeight) : 1.0f);
  DirectX::XMFLOAT4X4 viewMat;
  DirectX::XMFLOAT4X4 projMat;
  DirectX::XMStoreFloat4x4(&viewMat, camera.View());
  DirectX::XMStoreFloat4x4(&projMat, camera.Projection(aspect));
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      view[r * 4 + c] = viewMat.m[r][c];
      proj[r * 4 + c] = projMat.m[r][c];
    }
  }
  float camPos[3];
  camera.GetPosition(camPos[0], camPos[1], camPos[2]);
  MainCameraSetup mainCam;
  mainCam.camPos[0] = camPos[0];
  mainCam.camPos[1] = camPos[1];
  mainCam.camPos[2] = camPos[2];
  CameraBasisFromView(view, mainCam.camFwd, mainCam.camRight, mainCam.camUp);
  mainCam.fovY = camera.fovY;
  mainCam.aspect = aspect;
  mainCam.nearZ = camera.nearZ;
  mainCam.farZ = camera.farZ;
  renderer.SetMainCamera(mainCam);
  const std::vector<EntityId> ids = scene.All();
  SubmitSceneFog(renderer, scene);
  SubmitScenePost(ids, camPos);
  renderer.BeginObjects(view, proj);
  SubmitSceneCookies(ids);
  SubmitSceneLights(renderer, scene, EntityId::Invalid());
  float world[16];
  for (size_t i = 0; i < ids.size(); ++i) {
    const Entity* e = scene.Get(ids[i]);
    if (e == nullptr || !e->hasMesh || e->meshGuid.empty()) {
      continue;
    }
    ComposeMatrix(e->transform, world);
    std::string drawGuids[2];
    float drawFades[2];
    int drawCount = 1;
    SelectLodGuids(assets, renderer, *e, camPos, drawGuids, drawFades, drawCount);
    for (int li = 0; li < drawCount; ++li) {
      const MeshRecord* rec = assets.GetByGuid(drawGuids[li]);
      if (rec == nullptr || !rec->loaded) {
        continue;
      }
      if (rec->gpuVB == 0) {
        assets.EnsureMeshGpu(drawGuids[li], 64u * 1024u * 1024u);
        rec = assets.GetByGuid(drawGuids[li]);
        if (rec == nullptr || rec->gpuVB == 0) {
          continue;
        }
      }
      if (rec->data.parts.empty()) {
        DeferredMaterial material;
        const DeferredMaterial* useMat = FillDeferredForMesh(assets, *e, rec->data, material) ? &material : nullptr;
        UpdateVirtualTextureForDraw(assets, useMat, world, camPos);
        renderer.DrawObjectEx(world, rec->gpuVB, rec->gpuIB, 0, rec->gpuCount, useMat, drawFades[li], rec->data.aabbMin, rec->data.aabbMax);
        continue;
      }
      for (size_t p = 0; p < rec->data.parts.size(); ++p) {
        const MeshPartData& part = rec->data.parts[p];
        if (part.indexCount == 0 || static_cast<size_t>(part.indexOffset) + static_cast<size_t>(part.indexCount) > rec->data.indices.size()) {
          continue;
        }
        DeferredMaterial partMat;
        const DeferredMaterial* useMat = FillDeferredForPart(assets, *e, rec->data, p, partMat) ? &partMat : nullptr;
        UpdateVirtualTextureForDraw(assets, useMat, world, camPos);
        renderer.DrawObjectEx(world, rec->gpuVB, rec->gpuIB, part.indexOffset, part.indexCount, useMat, drawFades[li], rec->data.aabbMin, rec->data.aabbMax);
      }
    }
  }
  SubmitDecals(ids, lastDelta);
  renderer.EndObjectsToBackbuffer(camPos);
}
}