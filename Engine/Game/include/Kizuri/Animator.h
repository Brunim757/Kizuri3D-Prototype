#pragma once
#include "Kizuri/Animation.h"
#include "Kizuri/Assets/AssetDatabase.h"
#include "Kizuri/Scene.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
namespace Kizuri {
class DeferredRenderer;
class AnimatorRuntime {
public:
  struct BoneRange {
    uint32_t offset = 0xFFFFFFFFu;
    uint32_t count = 0;
  };
  void Reset();
  void BeginFrame();
  void SetSimulating(bool on);
  bool IsSimulating() const { return simulating; }
  void Update(const Scene& scene, const AssetDatabase& assets, float dt);
  bool Prepare(const Entity& entity, const AssetDatabase& assets);
  bool AddEntityBones(const Entity& entity, const AssetDatabase& assets, DeferredRenderer& renderer);
  BoneRange GetBoneRange(EntityId id, uint32_t skin) const;
  const std::vector<float>* BoneMatrices(EntityId id, uint32_t skin) const;
  static bool MeshIsSkinned(const MeshAssetData& data);
  static bool RecordIsSkinned(const AssetDatabase& assets, const std::string& guid);
private:
  struct MeshClipInfo {
    std::string name;
    float duration = 0.0f;
  };
  struct SkinAsset {
    Anim::Skeleton skeleton;
    std::vector<Anim::Clip> clips;
    std::vector<int> clipIndex;
    bool ready = false;
  };
  struct MeshCache {
    uint64_t dataRevision = 0;
    std::vector<MeshClipInfo> clips;
    std::vector<SkinAsset> skins;
  };
  struct FrameRange {
    BoneRange range;
    uint32_t frame = 0;
  };
  struct EntityCache {
    std::string meshGuid;
    uint64_t meshRevision = 0;
    float time = 0.0f;
    std::string clipName;
    int activeClip = -1;
    bool poseDirty = true;
    std::vector<Anim::Pose> poses;
    std::vector<std::vector<float>> modelMatrices;
    std::vector<std::vector<float>> boneMatrices;
    std::vector<FrameRange> frameRanges;
  };
  void BuildMeshCache(const MeshAssetData& data, uint64_t revision, MeshCache& out);
  void ResolveClip(EntityCache& cache, const MeshCache& mesh);
  void Evaluate(EntityCache& cache, const MeshCache& mesh, bool animated);
  std::unordered_map<std::string, MeshCache> meshes;
  std::unordered_map<EntityId, EntityCache, EntityIdHash> entities;
  Anim::ModelScratch scratch;
  uint32_t frameSerial = 1;
  bool simulating = true;
};
}
