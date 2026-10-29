#include "Kizuri/Animator.h"
#include "Kizuri/DeferredRenderer.h"
#include <cmath>
#include <cstring>
#include <utility>
namespace Kizuri {
namespace {
bool BuildSkeletonAsset(const MeshSkinData& source, Anim::Skeleton& out) {
  out.joints.clear();
  out.joints.reserve(source.joints.size());
  for (size_t i = 0; i < source.joints.size(); ++i) {
    const MeshSkinJointData& sourceJoint = source.joints[i];
    Anim::Joint joint;
    joint.name = sourceJoint.name;
    joint.parent = sourceJoint.parent;
    std::memcpy(joint.bindTranslation, sourceJoint.bindTranslation, sizeof(joint.bindTranslation));
    std::memcpy(joint.bindRotation, sourceJoint.bindRotation, sizeof(joint.bindRotation));
    std::memcpy(joint.bindScale, sourceJoint.bindScale, sizeof(joint.bindScale));
    std::memcpy(joint.inverseBind, sourceJoint.inverseBind, sizeof(joint.inverseBind));
    out.joints.push_back(std::move(joint));
  }
  return out.IsValid();
}
bool BuildSkinClip(const MeshAnimClipData& source, uint32_t skinIndex, Anim::Clip& out) {
  out.name = source.name;
  for (size_t i = 0; i < source.channels.size(); ++i) {
    const MeshAnimChannelData& sourceChannel = source.channels[i];
    if (sourceChannel.skin != skinIndex) {
      continue;
    }
    if (sourceChannel.path < 0 || sourceChannel.path > 2 || sourceChannel.interpolation < 0 || sourceChannel.interpolation > 1) {
      return false;
    }
    if (sourceChannel.keys.empty()) {
      continue;
    }
    Anim::Channel channel;
    channel.joint = static_cast<int>(sourceChannel.joint);
    channel.path = sourceChannel.path == 0 ? Anim::Path::Translation : (sourceChannel.path == 1 ? Anim::Path::Rotation : Anim::Path::Scale);
    channel.interpolation = sourceChannel.interpolation == 1 ? 1 : 0;
    channel.keys.reserve(sourceChannel.keys.size());
    for (size_t k = 0; k < sourceChannel.keys.size(); ++k) {
      Anim::Key key;
      key.time = sourceChannel.keys[k].time;
      std::memcpy(key.value, sourceChannel.keys[k].value, sizeof(key.value));
      channel.keys.push_back(key);
    }
    out.channels.push_back(std::move(channel));
  }
  return true;
}
float ClipDuration(const MeshAnimClipData& clip) {
  float duration = clip.duration;
  if (!(duration > 0.0f) || !std::isfinite(duration)) {
    duration = 0.0f;
    for (size_t i = 0; i < clip.channels.size(); ++i) {
      const std::vector<MeshAnimKeyData>& keys = clip.channels[i].keys;
      if (!keys.empty() && keys[keys.size() - 1].time > duration) {
        duration = keys[keys.size() - 1].time;
      }
    }
  }
  return std::isfinite(duration) && duration > 0.0f ? duration : 0.0f;
}
}
void AnimatorRuntime::Reset() {
  meshes.clear();
  entities.clear();
  scratch.state.clear();
  scratch.chain.clear();
  frameSerial = 1;
}
void AnimatorRuntime::BeginFrame() {
  ++frameSerial;
  if (frameSerial == 0) {
    frameSerial = 1;
    for (std::unordered_map<EntityId, EntityCache, EntityIdHash>::iterator it = entities.begin(); it != entities.end(); ++it) {
      for (size_t i = 0; i < it->second.frameRanges.size(); ++i) {
        it->second.frameRanges[i].range = BoneRange();
        it->second.frameRanges[i].frame = 0;
      }
    }
  }
}
void AnimatorRuntime::SetSimulating(bool on) {
  if (simulating == on) {
    return;
  }
  simulating = on;
  for (std::unordered_map<EntityId, EntityCache, EntityIdHash>::iterator it = entities.begin(); it != entities.end(); ++it) {
    it->second.time = 0.0f;
    it->second.poseDirty = true;
  }
}
bool AnimatorRuntime::MeshIsSkinned(const MeshAssetData& data) {
  return !data.skins.empty() || !data.partSkins.empty();
}
bool AnimatorRuntime::RecordIsSkinned(const AssetDatabase& assets, const std::string& guid) {
  const MeshRecord* record = assets.GetByGuid(guid);
  return record != nullptr && MeshIsSkinned(record->data);
}
void AnimatorRuntime::BuildMeshCache(const MeshAssetData& data, uint64_t revision, MeshCache& out) {
  out = MeshCache();
  out.dataRevision = revision;
  out.clips.reserve(data.animations.size());
  for (size_t i = 0; i < data.animations.size(); ++i) {
    MeshClipInfo info;
    info.name = data.animations[i].name;
    info.duration = ClipDuration(data.animations[i]);
    out.clips.push_back(std::move(info));
  }
  out.skins.resize(data.skins.size());
  std::vector<uint32_t> maxJoint(data.skins.size(), 0);
  bool jointsOk = data.partSkins.size() == data.parts.size();
  if (jointsOk) {
    for (size_t p = 0; p < data.parts.size() && jointsOk; ++p) {
      const uint32_t skinIndex = data.partSkins[p];
      if (skinIndex == 0xFFFFFFFFu) {
        continue;
      }
      if (skinIndex >= data.skins.size()) {
        jointsOk = false;
        break;
      }
      const MeshPartData& part = data.parts[p];
      if (part.indexCount == 0) {
        continue;
      }
      if (static_cast<size_t>(part.indexOffset) + static_cast<size_t>(part.indexCount) > data.indices.size()) {
        jointsOk = false;
        break;
      }
      for (uint32_t i = 0; i < part.indexCount; ++i) {
        const uint32_t vertex = data.indices[static_cast<size_t>(part.indexOffset) + i];
        const size_t base = static_cast<size_t>(vertex) * 4ull;
        if (base + 4ull > data.skinJoints.size()) {
          jointsOk = false;
          break;
        }
        for (int k = 0; k < 4; ++k) {
          const uint32_t joint = data.skinJoints[base + static_cast<size_t>(k)];
          if (joint > maxJoint[skinIndex]) {
            maxJoint[skinIndex] = joint;
          }
        }
      }
    }
  }
  for (size_t s = 0; s < out.skins.size(); ++s) {
    SkinAsset& skin = out.skins[s];
    skin.clipIndex.assign(data.animations.size(), -1);
    skin.ready = jointsOk && BuildSkeletonAsset(data.skins[s], skin.skeleton) && maxJoint[s] < skin.skeleton.joints.size();
    if (!skin.ready) {
      continue;
    }
    for (size_t c = 0; c < data.animations.size(); ++c) {
      Anim::Clip clip;
      if (!BuildSkinClip(data.animations[c], static_cast<uint32_t>(s), clip)) {
        continue;
      }
      if (!clip.IsValid(skin.skeleton)) {
        continue;
      }
      skin.clipIndex[c] = static_cast<int>(skin.clips.size());
      skin.clips.push_back(std::move(clip));
    }
  }
}
void AnimatorRuntime::ResolveClip(EntityCache& cache, const MeshCache& mesh) {
  cache.activeClip = -1;
  if (cache.clipName.empty()) {
    return;
  }
  for (size_t i = 0; i < mesh.clips.size(); ++i) {
    if (mesh.clips[i].name == cache.clipName) {
      cache.activeClip = static_cast<int>(i);
      return;
    }
  }
}
bool AnimatorRuntime::Prepare(const Entity& entity, const AssetDatabase& assets) {
  if (!entity.hasMesh || entity.meshGuid.empty()) {
    entities.erase(entity.id);
    return false;
  }
  const MeshRecord* record = assets.GetByGuid(entity.meshGuid);
  if (record == nullptr || record->data.skins.empty()) {
    entities.erase(entity.id);
    return false;
  }
  std::unordered_map<std::string, MeshCache>::iterator meshIt = meshes.find(entity.meshGuid);
  if (meshIt == meshes.end() || meshIt->second.dataRevision != record->dataRevision) {
    MeshCache built;
    BuildMeshCache(record->data, record->dataRevision, built);
    meshIt = meshes.insert_or_assign(entity.meshGuid, std::move(built)).first;
  }
  const MeshCache& mesh = meshIt->second;
  EntityCache& cache = entities[entity.id];
  bool rebind = cache.meshGuid != entity.meshGuid || cache.meshRevision != mesh.dataRevision || cache.poses.size() != mesh.skins.size();
  if (rebind) {
    cache.meshGuid = entity.meshGuid;
    cache.meshRevision = mesh.dataRevision;
    cache.clipName.clear();
    cache.poses.resize(mesh.skins.size());
    cache.modelMatrices.resize(mesh.skins.size());
    cache.boneMatrices.resize(mesh.skins.size());
    cache.frameRanges.assign(mesh.skins.size(), FrameRange());
    cache.time = 0.0f;
    cache.poseDirty = true;
  }
  const std::string desired = entity.hasAnimator ? entity.animator.clipName : std::string();
  if (rebind || cache.clipName != desired) {
    const bool clipChanged = cache.clipName != desired;
    cache.clipName = desired;
    ResolveClip(cache, mesh);
    if (clipChanged || rebind) {
      cache.time = 0.0f;
    }
    cache.poseDirty = true;
  }
  return true;
}
void AnimatorRuntime::Evaluate(EntityCache& cache, const MeshCache& mesh, bool animated) {
  if (!cache.poseDirty) {
    return;
  }
  const size_t skinCount = cache.poses.size() < mesh.skins.size() ? cache.poses.size() : mesh.skins.size();
  for (size_t s = 0; s < skinCount; ++s) {
    const SkinAsset& skin = mesh.skins[s];
    std::vector<float>& bones = cache.boneMatrices[s];
    bones.clear();
    if (!skin.ready || skin.skeleton.joints.empty()) {
      continue;
    }
    int clip = -1;
    if (animated && cache.activeClip >= 0 && static_cast<size_t>(cache.activeClip) < skin.clipIndex.size()) {
      clip = skin.clipIndex[static_cast<size_t>(cache.activeClip)];
    }
    bool sampled = false;
    if (clip >= 0 && static_cast<size_t>(clip) < skin.clips.size()) {
      sampled = Anim::SampleClipUnchecked(skin.skeleton, skin.clips[static_cast<size_t>(clip)], cache.time, cache.poses[s]);
    }
    if (!sampled && !Anim::ResetPose(skin.skeleton, cache.poses[s])) {
      continue;
    }
    if (!Anim::ComputeModelMatrices(skin.skeleton, cache.poses[s], cache.modelMatrices[s], scratch)) {
      continue;
    }
    const size_t jointCount = skin.skeleton.joints.size();
    bones.resize(jointCount * 16);
    for (size_t j = 0; j < jointCount; ++j) {
      Anim::MultiplyMatrix(skin.skeleton.joints[j].inverseBind, cache.modelMatrices[s].data() + j * 16, bones.data() + j * 16);
    }
  }
  cache.poseDirty = false;
}
void AnimatorRuntime::Update(const Scene& scene, const AssetDatabase& assets, float dt) {
  const std::vector<EntityId> ids = scene.All();
  if (simulating && dt > 0.0f && std::isfinite(dt)) {
    for (size_t i = 0; i < ids.size(); ++i) {
      const Entity* entity = scene.Get(ids[i]);
      if (entity == nullptr || !entity->hasAnimator || !entity->hasMesh || entity->meshGuid.empty()) {
        continue;
      }
      if (!Prepare(*entity, assets)) {
        continue;
      }
      std::unordered_map<EntityId, EntityCache, EntityIdHash>::iterator entityIt = entities.find(entity->id);
      if (entityIt == entities.end()) {
        continue;
      }
      EntityCache& cache = entityIt->second;
      std::unordered_map<std::string, MeshCache>::const_iterator meshIt = meshes.find(cache.meshGuid);
      if (meshIt == meshes.end()) {
        continue;
      }
      const AnimatorData& animator = entity->animator;
      if (!animator.playing || !std::isfinite(animator.speed) || animator.speed == 0.0f) {
        continue;
      }
      if (cache.activeClip < 0 || static_cast<size_t>(cache.activeClip) >= meshIt->second.clips.size()) {
        continue;
      }
      const float duration = meshIt->second.clips[static_cast<size_t>(cache.activeClip)].duration;
      if (!(duration > 0.0f)) {
        continue;
      }
      float next = cache.time + dt * animator.speed;
      if (!std::isfinite(next)) {
        next = 0.0f;
      } else if (animator.loop) {
        next = std::fmod(next, duration);
        if (next < 0.0f) {
          next += duration;
        }
      } else if (next < 0.0f) {
        next = 0.0f;
      } else if (next > duration) {
        next = duration;
      }
      if (next != cache.time) {
        cache.time = next;
        cache.poseDirty = true;
      }
    }
  }
  for (std::unordered_map<EntityId, EntityCache, EntityIdHash>::iterator it = entities.begin(); it != entities.end();) {
    const Entity* entity = scene.Get(it->first);
    if (entity == nullptr || !entity->hasMesh || entity->meshGuid.empty()) {
      it = entities.erase(it);
    } else {
      ++it;
    }
  }
  for (std::unordered_map<std::string, MeshCache>::iterator it = meshes.begin(); it != meshes.end();) {
    const MeshRecord* record = assets.GetByGuid(it->first);
    if (record == nullptr || record->data.skins.empty() || record->dataRevision != it->second.dataRevision) {
      it = meshes.erase(it);
    } else {
      ++it;
    }
  }
}
bool AnimatorRuntime::AddEntityBones(const Entity& entity, const AssetDatabase& assets, DeferredRenderer& renderer) {
  if (!Prepare(entity, assets)) {
    return false;
  }
  std::unordered_map<EntityId, EntityCache, EntityIdHash>::iterator entityIt = entities.find(entity.id);
  if (entityIt == entities.end()) {
    return false;
  }
  std::unordered_map<std::string, MeshCache>::const_iterator meshIt = meshes.find(entityIt->second.meshGuid);
  if (meshIt == meshes.end()) {
    return false;
  }
  EntityCache& cache = entityIt->second;
  const MeshCache& mesh = meshIt->second;
  Evaluate(cache, mesh, simulating && entity.hasAnimator);
  bool any = false;
  const size_t skinCount = cache.frameRanges.size() < mesh.skins.size() ? cache.frameRanges.size() : mesh.skins.size();
  for (size_t s = 0; s < skinCount; ++s) {
    FrameRange& frameRange = cache.frameRanges[s];
    if (frameRange.frame == frameSerial) {
      any = any || frameRange.range.count > 0;
      continue;
    }
    frameRange.frame = frameSerial;
    frameRange.range = BoneRange();
    const SkinAsset& skin = mesh.skins[s];
    const std::vector<float>& bones = cache.boneMatrices[s];
    if (!skin.ready || bones.empty() || skin.skeleton.joints.size() > 0xFFFFFFFFull) {
      continue;
    }
    const uint32_t count = static_cast<uint32_t>(skin.skeleton.joints.size());
    uint64_t motionKey = EntityHash64(entity.id, static_cast<uint32_t>(s));
    const uint32_t offset = renderer.AddBoneMatrices(bones.data(), count, motionKey);
    if (offset == 0xFFFFFFFFu) {
      continue;
    }
    frameRange.range.offset = offset;
    frameRange.range.count = count;
    any = true;
  }
  return any;
}
AnimatorRuntime::BoneRange AnimatorRuntime::GetBoneRange(EntityId id, uint32_t skin) const {
  std::unordered_map<EntityId, EntityCache, EntityIdHash>::const_iterator it = entities.find(id);
  if (it == entities.end() || skin >= it->second.frameRanges.size()) {
    return BoneRange();
  }
  const FrameRange& frameRange = it->second.frameRanges[skin];
  if (frameRange.frame != frameSerial) {
    return BoneRange();
  }
  return frameRange.range;
}
const std::vector<float>* AnimatorRuntime::BoneMatrices(EntityId id, uint32_t skin) const {
  std::unordered_map<EntityId, EntityCache, EntityIdHash>::const_iterator it = entities.find(id);
  if (it == entities.end() || skin >= it->second.boneMatrices.size() || it->second.boneMatrices[skin].empty()) {
    return nullptr;
  }
  return &it->second.boneMatrices[skin];
}
}
