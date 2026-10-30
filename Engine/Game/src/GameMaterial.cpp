#include "Kizuri/GameMaterial.h"
#include <string>
namespace Kizuri {
uint32_t ResolveMaterialSlot(AssetDatabase& assets, const std::string& guid, uint32_t bit, RHITexture& out) {
  out = 0;
  if (guid.empty()) {
    return 0;
  }
  const TextureRecord* tr = assets.GetTexByGuid(guid);
  if (tr == nullptr || !tr->loaded || tr->gpu == 0) {
    return 0;
  }
  if (tr->selectedLevels <= 0 || tr->residentLevels < tr->selectedLevels) {
    return 0;
  }
  out = tr->gpu;
  return bit;
}
void MaterialDataFromMesh(const MeshMaterialData& mm, MaterialData& m) {
  m.albedo[0] = mm.albedo[0];
  m.albedo[1] = mm.albedo[1];
  m.albedo[2] = mm.albedo[2];
  m.alpha = mm.alpha;
  m.metallic = mm.metallic;
  m.roughness = mm.roughness;
  m.albedoTex = mm.albedoTexGuid;
  m.normalTex = mm.normalTexGuid;
  m.ormTex = mm.ormTexGuid;
  m.heightTex = mm.heightTexGuid;
  m.emissive[0] = mm.emissive[0];
  m.emissive[1] = mm.emissive[1];
  m.emissive[2] = mm.emissive[2];
  m.emissiveStrength = mm.emissiveStrength;
  m.emissiveTex = mm.emissiveTexGuid;
  m.unlit = mm.unlit;
  m.doubleSided = mm.doubleSided;
  m.alphaMode = mm.alphaMode;
  m.alphaCutoff = mm.alphaCutoff;
  for (int s = 0; s < 5; ++s) {
    m.xforms[s].offsetU = mm.xforms[s].offsetU;
    m.xforms[s].offsetV = mm.xforms[s].offsetV;
    m.xforms[s].scaleU = mm.xforms[s].scaleU;
    m.xforms[s].scaleV = mm.xforms[s].scaleV;
    m.xforms[s].rotation = mm.xforms[s].rotation;
    m.xforms[s].uvSet = mm.xforms[s].uvSet;
  }
  m.ior = mm.ior;
  m.hasSpecular = mm.hasSpecular;
  m.specularColor[0] = mm.specularColor[0];
  m.specularColor[1] = mm.specularColor[1];
  m.specularColor[2] = mm.specularColor[2];
  m.specularStrength = mm.specularStrength;
  m.specularTex = mm.specularTexGuid;
  m.specularColorTex = mm.specularColorTexGuid;
  m.clearcoat = mm.clearcoat;
  m.clearcoatRoughness = mm.clearcoatRoughness;
  m.clearcoatTex = mm.clearcoatTexGuid;
  m.clearcoatRoughnessTex = mm.clearcoatRoughnessTexGuid;
  m.clearcoatNormalTex = mm.clearcoatNormalTexGuid;
  m.sheenColor[0] = mm.sheenColor[0];
  m.sheenColor[1] = mm.sheenColor[1];
  m.sheenColor[2] = mm.sheenColor[2];
  m.sheenRoughness = mm.sheenRoughness;
  m.sheenColorTex = mm.sheenColorTexGuid;
  m.sheenRoughnessTex = mm.sheenRoughnessTexGuid;
  m.anisotropy = mm.anisotropy;
  m.anisotropyRotation = mm.anisotropyRotation;
  m.anisotropyTex = mm.anisotropyTexGuid;
  m.iridescence = mm.iridescence;
  m.iridescenceIor = mm.iridescenceIor;
  m.iridescenceThicknessMin = mm.iridescenceThicknessMin;
  m.iridescenceThicknessMax = mm.iridescenceThicknessMax;
  m.iridescenceTex = mm.iridescenceTexGuid;
  m.iridescenceThicknessTex = mm.iridescenceThicknessTexGuid;
  m.transmission = mm.transmission;
  m.transmissionTex = mm.transmissionTexGuid;
  m.thickness = mm.thickness;
  m.thicknessTex = mm.thicknessTexGuid;
  m.attenuationColor[0] = mm.attenuationColor[0];
  m.attenuationColor[1] = mm.attenuationColor[1];
  m.attenuationColor[2] = mm.attenuationColor[2];
  m.attenuationDistance = mm.attenuationDistance;
  m.dispersion = mm.dispersion;
}
uint32_t ResolveMeshMaterialIndex(const MeshAssetData& mesh, const Entity& entity, size_t partIndex) {
  if (partIndex >= mesh.parts.size()) {
    return 0xFFFFFFFFu;
  }
  uint32_t materialIndex = mesh.parts[partIndex].material;
  if (entity.variantIndex >= 0) {
    for (size_t i = 0; i < mesh.variantAssigns.size(); ++i) {
      const MeshVariantAssign& assign = mesh.variantAssigns[i];
      if (assign.part == static_cast<uint32_t>(partIndex) && static_cast<int>(assign.variant) == entity.variantIndex && assign.material < mesh.materials.size()) {
        materialIndex = assign.material;
        break;
      }
    }
  }
  if (materialIndex >= mesh.materials.size()) {
    return 0xFFFFFFFFu;
  }
  return materialIndex;
}
std::unique_ptr<Command> BuildAddMaterialCommand(EntityId target, const Entity& entity, const MeshAssetData& mesh) {
  uint32_t baseIndex = ResolveMeshMaterialIndex(mesh, entity, 0);
  if (baseIndex == 0xFFFFFFFFu && !mesh.materials.empty()) {
    baseIndex = 0;
  }
  MaterialData initial;
  MakeDefaultMaterial(initial);
  if (baseIndex != 0xFFFFFFFFu) {
    MaterialDataFromMesh(mesh.materials[baseIndex], initial);
  }
  std::unique_ptr<CompositeCmd> composite(new CompositeCmd("Add Material"));
  composite->Add(std::unique_ptr<Command>(new AddComponentCmd(target, ComponentRegistry::kMaterial, &initial)));
  for (size_t p = 0; p < mesh.parts.size(); ++p) {
    bool hasOverride = false;
    for (size_t k = 0; k < entity.partMaterials.size(); ++k) {
      if (entity.partMaterials[k].part == static_cast<uint32_t>(p)) {
        hasOverride = true;
        break;
      }
    }
    if (hasOverride) {
      continue;
    }
    const uint32_t materialIndex = ResolveMeshMaterialIndex(mesh, entity, p);
    if (materialIndex == 0xFFFFFFFFu || materialIndex == baseIndex) {
      continue;
    }
    MaterialData partInitial;
    MakeDefaultMaterial(partInitial);
    MaterialDataFromMesh(mesh.materials[materialIndex], partInitial);
    if (MaterialEqual(initial, partInitial)) {
      continue;
    }
    composite->Add(std::unique_ptr<Command>(new AddPartMaterialCmd(target, static_cast<uint32_t>(p), partInitial)));
  }
  return std::unique_ptr<Command>(composite.release());
}
void FillDeferredFromEntity(AssetDatabase& assets, const MaterialData& m, DeferredMaterial& partMat) {
  partMat.linkGroup = static_cast<uint32_t>(m.linkMask < 0 ? 0 : (m.linkMask > 255 ? 255 : m.linkMask));
  std::string nodeAlbedoTex = m.albedoTex;
  float nodeAlbedo[3] = { m.albedo[0], m.albedo[1], m.albedo[2] };
  float nodeRough = m.roughness;
  float nodeMetal = m.metallic;
  float nodeEmissive[3] = { m.emissive[0], m.emissive[1], m.emissive[2] };
  if (m.useNodes && !m.graph.nodes.empty()) {
    std::string bakedTex;
    if (EvaluateMaterialGraph(m.graph, nodeAlbedo, nodeRough, nodeMetal, nodeEmissive, bakedTex)) {
      if (!bakedTex.empty()) {
        nodeAlbedoTex = bakedTex;
      }
    }
  }
  partMat.albedo[0] = nodeAlbedo[0];
  partMat.albedo[1] = nodeAlbedo[1];
  partMat.albedo[2] = nodeAlbedo[2];
  partMat.alpha = m.alpha;
  partMat.roughness = nodeRough < 0.0f ? 0.0f : (nodeRough > 1.0f ? 1.0f : nodeRough);
  partMat.metallic = nodeMetal < 0.0f ? 0.0f : (nodeMetal > 1.0f ? 1.0f : nodeMetal);
  partMat.emissive[0] = nodeEmissive[0];
  partMat.emissive[1] = nodeEmissive[1];
  partMat.emissive[2] = nodeEmissive[2];
  partMat.emissiveStrength = m.emissiveStrength;
  partMat.unlit = m.unlit;
  partMat.doubleSided = m.doubleSided;
  partMat.alphaMode = m.alphaMode;
  partMat.alphaCutoff = m.alphaCutoff;
  partMat.celShaded = m.celShaded;
  partMat.celBands = m.celBands < 1.0f ? 1.0f : (m.celBands > 8.0f ? 8.0f : m.celBands);
  partMat.celSpecular = m.celSpecular < 0.0f ? 0.0f : (m.celSpecular > 1.0f ? 1.0f : m.celSpecular);
  partMat.celRim = m.celRim < 0.0f ? 0.0f : (m.celRim > 1.0f ? 1.0f : m.celRim);
  partMat.celOutline = m.celOutline < 0.0f ? 0.0f : (m.celOutline > 1.0f ? 1.0f : m.celOutline);
  partMat.celOutlineWidth = m.celOutlineWidth < 0.5f ? 0.5f : (m.celOutlineWidth > 4.0f ? 4.0f : m.celOutlineWidth);
  partMat.celOutlineColor[0] = m.celOutlineColor[0] < 0.0f ? 0.0f : (m.celOutlineColor[0] > 1.0f ? 1.0f : m.celOutlineColor[0]);
  partMat.celOutlineColor[1] = m.celOutlineColor[1] < 0.0f ? 0.0f : (m.celOutlineColor[1] > 1.0f ? 1.0f : m.celOutlineColor[1]);
  partMat.celOutlineColor[2] = m.celOutlineColor[2] < 0.0f ? 0.0f : (m.celOutlineColor[2] > 1.0f ? 1.0f : m.celOutlineColor[2]);
  for (int s = 0; s < 5; ++s) {
    partMat.xforms[s].offsetU = m.xforms[s].offsetU;
    partMat.xforms[s].offsetV = m.xforms[s].offsetV;
    partMat.xforms[s].scaleU = m.xforms[s].scaleU;
    partMat.xforms[s].scaleV = m.xforms[s].scaleV;
    partMat.xforms[s].rotation = m.xforms[s].rotation;
    partMat.xforms[s].uvSet = m.xforms[s].uvSet;
  }
  partMat.ior = m.ior;
  partMat.hasSpecular = m.hasSpecular;
  partMat.specularColor[0] = m.specularColor[0];
  partMat.specularColor[1] = m.specularColor[1];
  partMat.specularColor[2] = m.specularColor[2];
  partMat.specularStrength = m.specularStrength;
  partMat.clearcoat = m.clearcoat;
  partMat.clearcoatRoughness = m.clearcoatRoughness;
  partMat.sheenColor[0] = m.sheenColor[0];
  partMat.sheenColor[1] = m.sheenColor[1];
  partMat.sheenColor[2] = m.sheenColor[2];
  partMat.sheenRoughness = m.sheenRoughness;
  partMat.anisotropy = m.anisotropy;
  partMat.anisotropyRotation = m.anisotropyRotation;
  partMat.iridescence = m.iridescence;
  partMat.iridescenceIor = m.iridescenceIor;
  partMat.iridescenceThicknessMin = m.iridescenceThicknessMin;
  partMat.iridescenceThicknessMax = m.iridescenceThicknessMax;
  partMat.transmission = m.transmission;
  partMat.thickness = m.thickness;
  partMat.attenuationColor[0] = m.attenuationColor[0];
  partMat.attenuationColor[1] = m.attenuationColor[1];
  partMat.attenuationColor[2] = m.attenuationColor[2];
  partMat.attenuationDistance = m.attenuationDistance;
  partMat.dispersion = m.dispersion;
  partMat.detailTiling = m.detailTiling;
  partMat.layerWeight = m.layerWeight;
  partMat.layerAlbedo[0] = m.layerAlbedo[0];
  partMat.layerAlbedo[1] = m.layerAlbedo[1];
  partMat.layerAlbedo[2] = m.layerAlbedo[2];
  partMat.layerRoughness = m.layerRoughness;
  partMat.layerMetallic = m.layerMetallic;
  partMat.layerHeight = m.layerHeight;
  partMat.layerHeightBlend = m.layerHeightBlend;
  partMat.sssColor[0] = m.sssColor[0];
  partMat.sssColor[1] = m.sssColor[1];
  partMat.sssColor[2] = m.sssColor[2];
  partMat.sssIntensity = m.sssIntensity;
  partMat.sssKind = m.sssKind < 0 ? 0 : (m.sssKind > 3 ? 3 : m.sssKind);
  partMat.triplanar = m.triplanar;
  partMat.triplanarScale = m.triplanarScale < 0.001f ? 0.001f : m.triplanarScale;
  uint32_t flags = 0;
  flags |= ResolveMaterialSlot(assets, nodeAlbedoTex, 1u, partMat.albedoTex);
  flags |= ResolveMaterialSlot(assets, m.normalTex, 2u, partMat.normalTex);
  flags |= ResolveMaterialSlot(assets, m.ormTex, 4u, partMat.ormTex);
  flags |= ResolveMaterialSlot(assets, m.heightTex, 8u, partMat.heightTex);
  flags |= ResolveMaterialSlot(assets, m.emissiveTex, 32u, partMat.emissiveTex);
  flags |= ResolveMaterialSlot(assets, m.specularTex, 64u, partMat.specularTex);
  flags |= ResolveMaterialSlot(assets, m.specularColorTex, 128u, partMat.specularColorTex);
  flags |= ResolveMaterialSlot(assets, m.clearcoatTex, 256u, partMat.clearcoatTex);
  flags |= ResolveMaterialSlot(assets, m.clearcoatRoughnessTex, 512u, partMat.clearcoatRoughnessTex);
  flags |= ResolveMaterialSlot(assets, m.clearcoatNormalTex, 1024u, partMat.clearcoatNormalTex);
  flags |= ResolveMaterialSlot(assets, m.sheenColorTex, 2048u, partMat.sheenColorTex);
  flags |= ResolveMaterialSlot(assets, m.sheenRoughnessTex, 4096u, partMat.sheenRoughnessTex);
  flags |= ResolveMaterialSlot(assets, m.anisotropyTex, 8192u, partMat.anisotropyTex);
  flags |= ResolveMaterialSlot(assets, m.iridescenceTex, 16384u, partMat.iridescenceTex);
  flags |= ResolveMaterialSlot(assets, m.iridescenceThicknessTex, 32768u, partMat.iridescenceThicknessTex);
  flags |= ResolveMaterialSlot(assets, m.transmissionTex, 131072u, partMat.transmissionTex);
  flags |= ResolveMaterialSlot(assets, m.thicknessTex, 262144u, partMat.thicknessTex);
  flags |= ResolveMaterialSlot(assets, m.detailAlbedoTex, 1048576u, partMat.detailAlbedoTex);
  if (!m.detailNormalTex.empty()) {
    assets.EnsureTextureKind(m.detailNormalTex, TexKind::Normal);
    flags |= ResolveMaterialSlot(assets, m.detailNormalTex, 2097152u, partMat.detailNormalTex);
  }
  if (m.layerWeight > 0.001f) {
    flags |= 4194304u;
  }
  if (m.layerHeightBlend) {
    flags |= 8388608u;
  }
  if (m.hasSpecular) {
    flags |= 65536u;
  }
  if (m.sssKind == 1) {
    flags |= 16777216u;
  } else if (m.sssKind == 2) {
    flags |= 33554432u;
  } else if (m.sssKind == 3) {
    flags |= 67108864u;
  }
  if (m.triplanar) {
    flags |= 536870912u;
  }
  partMat.vtTable = 0;
  partMat.vtTail = 0;
  partMat.vtResidentMip = 99.0f;
  partMat.vtGuid.clear();
  if (m.vtAlbedo && !nodeAlbedoTex.empty()) {
    VtBundle bundle;
    const TextureRecord* vtRec = assets.GetTexByGuid(nodeAlbedoTex);
    const TextureAssetData* vtData = (vtRec != nullptr && vtRec->loaded) ? &vtRec->data : nullptr;
    if (assets.GetVirtualBundle(nodeAlbedoTex, bundle) && bundle.valid) {
      flags |= 134217728u;
      partMat.albedoTex = bundle.physical;
      partMat.vtTable = bundle.table;
      partMat.vtTail = bundle.tail;
      partMat.vtResidentMip = bundle.residentMip;
      partMat.vtGuid = nodeAlbedoTex;
    } else if (vtData != nullptr && assets.CreateVirtualTexture(vtData, nodeAlbedoTex) >= 0 && assets.GetVirtualBundle(nodeAlbedoTex, bundle) && bundle.valid) {
      flags |= 134217728u;
      partMat.albedoTex = bundle.physical;
      partMat.vtTable = bundle.table;
      partMat.vtTail = bundle.tail;
      partMat.vtResidentMip = bundle.residentMip;
      partMat.vtGuid = nodeAlbedoTex;
    }
  }
  partMat.texFlags = flags;
}
void FillDeferredFromMesh(AssetDatabase& assets, const MeshMaterialData& mm, DeferredMaterial& partMat) {
  partMat.celShaded = false;
  partMat.celBands = 3.0f;
  partMat.celSpecular = 0.35f;
  partMat.celRim = 0.2f;
  partMat.celOutline = 0.7f;
  partMat.celOutlineWidth = 1.5f;
  partMat.celOutlineColor[0] = 0.02f;
  partMat.celOutlineColor[1] = 0.02f;
  partMat.celOutlineColor[2] = 0.03f;
  partMat.albedo[0] = mm.albedo[0];
  partMat.albedo[1] = mm.albedo[1];
  partMat.albedo[2] = mm.albedo[2];
  partMat.alpha = mm.alpha;
  partMat.roughness = mm.roughness;
  partMat.metallic = mm.metallic;
  partMat.emissive[0] = mm.emissive[0];
  partMat.emissive[1] = mm.emissive[1];
  partMat.emissive[2] = mm.emissive[2];
  partMat.emissiveStrength = mm.emissiveStrength;
  partMat.unlit = mm.unlit;
  partMat.doubleSided = mm.doubleSided;
  partMat.alphaMode = mm.alphaMode;
  partMat.alphaCutoff = mm.alphaCutoff;
  for (int s = 0; s < 5; ++s) {
    partMat.xforms[s].offsetU = mm.xforms[s].offsetU;
    partMat.xforms[s].offsetV = mm.xforms[s].offsetV;
    partMat.xforms[s].scaleU = mm.xforms[s].scaleU;
    partMat.xforms[s].scaleV = mm.xforms[s].scaleV;
    partMat.xforms[s].rotation = mm.xforms[s].rotation;
    partMat.xforms[s].uvSet = mm.xforms[s].uvSet;
  }
  partMat.ior = mm.ior;
  partMat.hasSpecular = mm.hasSpecular;
  partMat.specularColor[0] = mm.specularColor[0];
  partMat.specularColor[1] = mm.specularColor[1];
  partMat.specularColor[2] = mm.specularColor[2];
  partMat.specularStrength = mm.specularStrength;
  partMat.clearcoat = mm.clearcoat;
  partMat.clearcoatRoughness = mm.clearcoatRoughness;
  partMat.sheenColor[0] = mm.sheenColor[0];
  partMat.sheenColor[1] = mm.sheenColor[1];
  partMat.sheenColor[2] = mm.sheenColor[2];
  partMat.sheenRoughness = mm.sheenRoughness;
  partMat.anisotropy = mm.anisotropy;
  partMat.anisotropyRotation = mm.anisotropyRotation;
  partMat.iridescence = mm.iridescence;
  partMat.iridescenceIor = mm.iridescenceIor;
  partMat.iridescenceThicknessMin = mm.iridescenceThicknessMin;
  partMat.iridescenceThicknessMax = mm.iridescenceThicknessMax;
  partMat.transmission = mm.transmission;
  partMat.thickness = mm.thickness;
  partMat.attenuationColor[0] = mm.attenuationColor[0];
  partMat.attenuationColor[1] = mm.attenuationColor[1];
  partMat.attenuationColor[2] = mm.attenuationColor[2];
  partMat.attenuationDistance = mm.attenuationDistance;
  partMat.dispersion = mm.dispersion;
  uint32_t flags = 0;
  flags |= ResolveMaterialSlot(assets, mm.albedoTexGuid, 1u, partMat.albedoTex);
  flags |= ResolveMaterialSlot(assets, mm.normalTexGuid, 2u, partMat.normalTex);
  flags |= ResolveMaterialSlot(assets, mm.ormTexGuid, 4u, partMat.ormTex);
  flags |= ResolveMaterialSlot(assets, mm.heightTexGuid, 8u, partMat.heightTex);
  flags |= ResolveMaterialSlot(assets, mm.emissiveTexGuid, 32u, partMat.emissiveTex);
  flags |= ResolveMaterialSlot(assets, mm.specularTexGuid, 64u, partMat.specularTex);
  flags |= ResolveMaterialSlot(assets, mm.specularColorTexGuid, 128u, partMat.specularColorTex);
  flags |= ResolveMaterialSlot(assets, mm.clearcoatTexGuid, 256u, partMat.clearcoatTex);
  flags |= ResolveMaterialSlot(assets, mm.clearcoatRoughnessTexGuid, 512u, partMat.clearcoatRoughnessTex);
  flags |= ResolveMaterialSlot(assets, mm.clearcoatNormalTexGuid, 1024u, partMat.clearcoatNormalTex);
  flags |= ResolveMaterialSlot(assets, mm.sheenColorTexGuid, 2048u, partMat.sheenColorTex);
  flags |= ResolveMaterialSlot(assets, mm.sheenRoughnessTexGuid, 4096u, partMat.sheenRoughnessTex);
  flags |= ResolveMaterialSlot(assets, mm.anisotropyTexGuid, 8192u, partMat.anisotropyTex);
  flags |= ResolveMaterialSlot(assets, mm.iridescenceTexGuid, 16384u, partMat.iridescenceTex);
  flags |= ResolveMaterialSlot(assets, mm.iridescenceThicknessTexGuid, 32768u, partMat.iridescenceThicknessTex);
  flags |= ResolveMaterialSlot(assets, mm.transmissionTexGuid, 131072u, partMat.transmissionTex);
  flags |= ResolveMaterialSlot(assets, mm.thicknessTexGuid, 262144u, partMat.thicknessTex);
  if (mm.hasSpecular) {
    flags |= 65536u;
  }
  partMat.texFlags = flags;
}
static void ApplyEntityLightmap(AssetDatabase& assets, const Entity& entity, DeferredMaterial& out) {
  if (!entity.hasLightmap || entity.lightmap.texGuid.empty()) {
    return;
  }
  RHITexture lightmapTex = 0;
  if (ResolveMaterialSlot(assets, entity.lightmap.texGuid, 16u, lightmapTex) != 0) {
    out.lightmapTex = lightmapTex;
    out.lightmapIntensity = entity.lightmap.intensity;
    out.texFlags |= 16u;
  }
}
bool FillDeferredForMesh(AssetDatabase& assets, const Entity& entity, const MeshAssetData& mesh, DeferredMaterial& out) {
  out = DeferredMaterial();
  if (entity.hasMaterial) {
    FillDeferredFromEntity(assets, entity.material, out);
    ApplyEntityLightmap(assets, entity, out);
    return true;
  }
  if (!mesh.materials.empty()) {
    FillDeferredFromMesh(assets, mesh.materials[0], out);
    ApplyEntityLightmap(assets, entity, out);
    return true;
  }
  return false;
}
bool FillDeferredForPart(AssetDatabase& assets, const Entity& entity, const MeshAssetData& mesh, size_t partIndex, DeferredMaterial& out) {
  out = DeferredMaterial();
  if (partIndex >= mesh.parts.size()) {
    return false;
  }
  for (size_t k = 0; k < entity.partMaterials.size(); ++k) {
    if (entity.partMaterials[k].part == static_cast<uint32_t>(partIndex)) {
      FillDeferredFromEntity(assets, entity.partMaterials[k].material, out);
      ApplyEntityLightmap(assets, entity, out);
      return true;
    }
  }
  if (entity.hasMaterial) {
    FillDeferredFromEntity(assets, entity.material, out);
    ApplyEntityLightmap(assets, entity, out);
    return true;
  }
  const uint32_t materialIndex = ResolveMeshMaterialIndex(mesh, entity, partIndex);
  if (materialIndex == 0xFFFFFFFFu) {
    return false;
  }
  FillDeferredFromMesh(assets, mesh.materials[materialIndex], out);
  ApplyEntityLightmap(assets, entity, out);
  return true;
}
}
