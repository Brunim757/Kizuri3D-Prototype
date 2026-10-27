#pragma once
#include <stdint.h>
#include <string>
#include <vector>
namespace Kizuri {
const char* Animation_Version();
bool Animation_SelfTest();
namespace Anim {
struct Joint {
  std::string name;
  int parent = -1;
  float bindTranslation[3] = { 0.0f, 0.0f, 0.0f };
  float bindRotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
  float bindScale[3] = { 1.0f, 1.0f, 1.0f };
  float inverseBind[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
};
struct Skeleton {
  std::vector<Joint> joints;
  int FindJoint(const std::string& name) const;
  bool IsValid() const;
};
struct SkinVertex {
  uint32_t joints[4] = { 0, 0, 0, 0 };
  float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
enum class Path {
  Translation,
  Rotation,
  Scale
};
struct Key {
  float time = 0.0f;
  float value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
struct Channel {
  int joint = -1;
  Path path = Path::Translation;
  std::vector<Key> keys;
};
struct Clip {
  std::string name;
  std::vector<Channel> channels;
  float Duration() const;
  bool IsValid(const Skeleton& skeleton) const;
};
struct JointPose {
  float translation[3] = { 0.0f, 0.0f, 0.0f };
  float rotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
  float scale[3] = { 1.0f, 1.0f, 1.0f };
};
struct Pose {
  std::vector<JointPose> joints;
};
void IdentityMatrix(float out[16]);
bool InvertMatrix(const float in[16], float out[16]);
void MultiplyMatrix(const float a[16], const float b[16], float out[16]);
void ComposeTRS(const float translation[3], const float rotation[4], const float scale[3], float out[16]);
void TransformPoint(const float m[16], const float p[3], float out[3]);
void NormalizeQuat(float q[4]);
void SlerpQuat(const float a[4], const float b[4], float t, float out[4]);
bool ResetPose(const Skeleton& skeleton, Pose& out);
bool SampleClip(const Skeleton& skeleton, const Clip& clip, float time, Pose& out);
bool BlendPoses(const Pose& a, const Pose& b, float t, Pose& out);
bool ComputeModelMatrices(const Skeleton& skeleton, const Pose& pose, std::vector<float>& out);
bool SkinPosition(const Skeleton& skeleton, const std::vector<float>& modelMatrices, const SkinVertex& skin, const float bindPosition[3], float out[3]);
}
}
