#pragma once
#include "Kizuri/Assets/AssetDatabase.h"
#include "Kizuri/DeferredRenderer.h"
#include "Kizuri/Scene.h"
#include <cstddef>
#include <cstdint>
namespace Kizuri {
uint32_t ResolveMaterialSlot(AssetDatabase& assets, const std::string& guid, uint32_t bit, RHITexture& out);
void FillDeferredFromEntity(AssetDatabase& assets, const MaterialData& m, DeferredMaterial& partMat);
void FillDeferredFromMesh(AssetDatabase& assets, const MeshMaterialData& mm, DeferredMaterial& partMat);
bool FillDeferredForMesh(AssetDatabase& assets, const Entity& entity, const MeshAssetData& mesh, DeferredMaterial& out);
bool FillDeferredForPart(AssetDatabase& assets, const Entity& entity, const MeshAssetData& mesh, size_t partIndex, DeferredMaterial& out);
}
