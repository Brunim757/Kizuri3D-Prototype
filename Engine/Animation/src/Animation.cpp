#include "Kizuri/Animation.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace Kizuri {
namespace {
const float kEpsilon = 1.0e-6f;
bool Finite(float v) {
  return std::isfinite(v);
}
bool Finite3(const float v[3]) {
  return Finite(v[0]) && Finite(v[1]) && Finite(v[2]);
}
bool Finite4(const float v[4]) {
  return Finite(v[0]) && Finite(v[1]) && Finite(v[2]) && Finite(v[3]);
}
float Dot4(const float a[4], const float b[4]) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}
float Clamp01(float v) {
  if (v < 0.0f) {
    return 0.0f;
  }
  if (v > 1.0f) {
    return 1.0f;
  }
  return v;
}
void Lerp3(const float a[3], const float b[3], float t, float out[3]) {
  out[0] = a[0] + (b[0] - a[0]) * t;
  out[1] = a[1] + (b[1] - a[1]) * t;
  out[2] = a[2] + (b[2] - a[2]) * t;
}
void Copy3(const float a[3], float out[3]) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
}
void Copy4(const float a[4], float out[4]) {
  out[0] = a[0];
  out[1] = a[1];
  out[2] = a[2];
  out[3] = a[3];
}
bool NearlyEqual(float a, float b, float epsilon) {
  return std::fabs(a - b) <= epsilon;
}
bool MatrixNearIdentity(const float m[16], float epsilon) {
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      float expected = r == c ? 1.0f : 0.0f;
      if (!NearlyEqual(m[r * 4 + c], expected, epsilon)) {
        return false;
      }
    }
  }
  return true;
}
}
namespace Anim {
int Skeleton::FindJoint(const std::string& name) const {
  for (size_t i = 0; i < joints.size(); ++i) {
    if (joints[i].name == name) {
      return static_cast<int>(i);
    }
  }
  return -1;
}
bool Skeleton::IsValid() const {
  if (joints.empty()) {
    return false;
  }
  const int count = static_cast<int>(joints.size());
  for (int i = 0; i < count; ++i) {
    const Joint& joint = joints[static_cast<size_t>(i)];
    if (joint.parent < -1 || joint.parent >= count || joint.parent == i) {
      return false;
    }
    if (!Finite3(joint.bindTranslation) || !Finite4(joint.bindRotation) || !Finite3(joint.bindScale)) {
      return false;
    }
    float rotationLength = Dot4(joint.bindRotation, joint.bindRotation);
    if (!Finite(rotationLength) || rotationLength < kEpsilon * kEpsilon) {
      return false;
    }
    for (int k = 0; k < 16; ++k) {
      if (!Finite(joint.inverseBind[k])) {
        return false;
      }
    }
  }
  std::vector<unsigned char> state(static_cast<size_t>(count), 0);
  for (int i = 0; i < count; ++i) {
    int current = i;
    while (current >= 0 && state[static_cast<size_t>(current)] == 0) {
      state[static_cast<size_t>(current)] = 1;
      current = joints[static_cast<size_t>(current)].parent;
    }
    if (current >= 0 && state[static_cast<size_t>(current)] == 1) {
      return false;
    }
    current = i;
    while (current >= 0 && state[static_cast<size_t>(current)] == 1) {
      state[static_cast<size_t>(current)] = 2;
      current = joints[static_cast<size_t>(current)].parent;
    }
  }
  return true;
}
float Clip::Duration() const {
  float duration = 0.0f;
  for (size_t c = 0; c < channels.size(); ++c) {
    const Channel& channel = channels[c];
    if (channel.keys.empty()) {
      continue;
    }
    float last = channel.keys[channel.keys.size() - 1].time;
    if (last > duration) {
      duration = last;
    }
  }
  return duration;
}
bool Clip::IsValid(const Skeleton& skeleton) const {
  const int jointCount = static_cast<int>(skeleton.joints.size());
  for (size_t c = 0; c < channels.size(); ++c) {
    const Channel& channel = channels[c];
    if (channel.joint < 0 || channel.joint >= jointCount || channel.keys.empty()) {
      return false;
    }
    if (channel.interpolation < 0 || channel.interpolation > 1) {
      return false;
    }
    float previous = -1.0f;
    for (size_t k = 0; k < channel.keys.size(); ++k) {
      const Key& key = channel.keys[k];
      if (!Finite(key.time) || key.time < 0.0f || key.time <= previous) {
        return false;
      }
      previous = key.time;
      if (channel.path == Path::Rotation) {
        if (!Finite4(key.value)) {
          return false;
        }
        float length = Dot4(key.value, key.value);
        if (!Finite(length) || length < kEpsilon * kEpsilon) {
          return false;
        }
      } else if (!Finite3(key.value)) {
        return false;
      }
    }
  }
  return true;
}
void IdentityMatrix(float out[16]) {
  for (int i = 0; i < 16; ++i) {
    out[i] = 0.0f;
  }
  out[0] = 1.0f;
  out[5] = 1.0f;
  out[10] = 1.0f;
  out[15] = 1.0f;
}
bool InvertMatrix(const float in[16], float out[16]) {
  float inv[16];
  inv[0] = in[5] * in[10] * in[15] - in[5] * in[11] * in[14] - in[9] * in[6] * in[15] + in[9] * in[7] * in[14] + in[13] * in[6] * in[11] - in[13] * in[7] * in[10];
  inv[4] = -in[4] * in[10] * in[15] + in[4] * in[11] * in[14] + in[8] * in[6] * in[15] - in[8] * in[7] * in[14] - in[12] * in[6] * in[11] + in[12] * in[7] * in[10];
  inv[8] = in[4] * in[9] * in[15] - in[4] * in[11] * in[13] - in[8] * in[5] * in[15] + in[8] * in[7] * in[13] + in[12] * in[5] * in[11] - in[12] * in[7] * in[9];
  inv[12] = -in[4] * in[9] * in[14] + in[4] * in[10] * in[13] + in[8] * in[5] * in[14] - in[8] * in[6] * in[13] - in[12] * in[5] * in[10] + in[12] * in[6] * in[9];
  inv[1] = -in[1] * in[10] * in[15] + in[1] * in[11] * in[14] + in[9] * in[2] * in[15] - in[9] * in[3] * in[14] - in[13] * in[2] * in[11] + in[13] * in[3] * in[10];
  inv[5] = in[0] * in[10] * in[15] - in[0] * in[11] * in[14] - in[8] * in[2] * in[15] + in[8] * in[3] * in[14] + in[12] * in[2] * in[11] - in[12] * in[3] * in[10];
  inv[9] = -in[0] * in[9] * in[15] + in[0] * in[11] * in[13] + in[8] * in[1] * in[15] - in[8] * in[3] * in[13] - in[12] * in[1] * in[11] + in[12] * in[3] * in[9];
  inv[13] = in[0] * in[9] * in[14] - in[0] * in[10] * in[13] - in[8] * in[1] * in[14] + in[8] * in[2] * in[13] + in[12] * in[1] * in[10] - in[12] * in[2] * in[9];
  inv[2] = in[1] * in[6] * in[15] - in[1] * in[7] * in[14] - in[5] * in[2] * in[15] + in[5] * in[3] * in[14] + in[13] * in[2] * in[7] - in[13] * in[3] * in[6];
  inv[6] = -in[0] * in[6] * in[15] + in[0] * in[7] * in[14] + in[4] * in[2] * in[15] - in[4] * in[3] * in[14] - in[12] * in[2] * in[7] + in[12] * in[3] * in[6];
  inv[10] = in[0] * in[5] * in[15] - in[0] * in[7] * in[13] - in[4] * in[1] * in[15] + in[4] * in[3] * in[13] + in[12] * in[1] * in[7] - in[12] * in[3] * in[5];
  inv[14] = -in[0] * in[5] * in[14] + in[0] * in[6] * in[13] + in[4] * in[1] * in[14] - in[4] * in[2] * in[13] - in[12] * in[1] * in[6] + in[12] * in[2] * in[5];
  inv[3] = -in[1] * in[6] * in[11] + in[1] * in[7] * in[10] + in[5] * in[2] * in[11] - in[5] * in[3] * in[10] - in[9] * in[2] * in[7] + in[9] * in[3] * in[6];
  inv[7] = in[0] * in[6] * in[11] - in[0] * in[7] * in[10] - in[4] * in[2] * in[11] + in[4] * in[3] * in[10] + in[8] * in[2] * in[7] - in[8] * in[3] * in[6];
  inv[11] = -in[0] * in[5] * in[11] + in[0] * in[7] * in[9] + in[4] * in[1] * in[11] - in[4] * in[3] * in[9] - in[8] * in[1] * in[7] + in[8] * in[3] * in[5];
  inv[15] = in[0] * in[5] * in[10] - in[0] * in[6] * in[9] - in[4] * in[1] * in[10] + in[4] * in[2] * in[9] + in[8] * in[1] * in[6] - in[8] * in[2] * in[5];
  float det = in[0] * inv[0] + in[1] * inv[4] + in[2] * inv[8] + in[3] * inv[12];
  if (!Finite(det) || std::fabs(det) < 1.0e-12f) {
    return false;
  }
  float invDet = 1.0f / det;
  for (int i = 0; i < 16; ++i) {
    out[i] = inv[i] * invDet;
  }
  return true;
}
void MultiplyMatrix(const float a[16], const float b[16], float out[16]) {
  float result[16];
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      result[r * 4 + c] = a[r * 4 + 0] * b[0 * 4 + c] + a[r * 4 + 1] * b[1 * 4 + c] + a[r * 4 + 2] * b[2 * 4 + c] + a[r * 4 + 3] * b[3 * 4 + c];
    }
  }
  std::memcpy(out, result, sizeof(result));
}
void ComposeTRS(const float translation[3], const float rotation[4], const float scale[3], float out[16]) {
  float q[4];
  Copy4(rotation, q);
  if (!Finite4(q) || Dot4(q, q) < kEpsilon * kEpsilon) {
    q[0] = 0.0f;
    q[1] = 0.0f;
    q[2] = 0.0f;
    q[3] = 1.0f;
  }
  NormalizeQuat(q);
  float x = q[0];
  float y = q[1];
  float z = q[2];
  float w = q[3];
  float xx = x * x;
  float yy = y * y;
  float zz = z * z;
  float xy = x * y;
  float xz = x * z;
  float yz = y * z;
  float wx = w * x;
  float wy = w * y;
  float wz = w * z;
  float sx = Finite(scale[0]) ? scale[0] : 1.0f;
  float sy = Finite(scale[1]) ? scale[1] : 1.0f;
  float sz = Finite(scale[2]) ? scale[2] : 1.0f;
  out[0] = (1.0f - 2.0f * (yy + zz)) * sx;
  out[1] = (2.0f * (xy + wz)) * sx;
  out[2] = (2.0f * (xz - wy)) * sx;
  out[3] = 0.0f;
  out[4] = (2.0f * (xy - wz)) * sy;
  out[5] = (1.0f - 2.0f * (xx + zz)) * sy;
  out[6] = (2.0f * (yz + wx)) * sy;
  out[7] = 0.0f;
  out[8] = (2.0f * (xz + wy)) * sz;
  out[9] = (2.0f * (yz - wx)) * sz;
  out[10] = (1.0f - 2.0f * (xx + yy)) * sz;
  out[11] = 0.0f;
  out[12] = Finite(translation[0]) ? translation[0] : 0.0f;
  out[13] = Finite(translation[1]) ? translation[1] : 0.0f;
  out[14] = Finite(translation[2]) ? translation[2] : 0.0f;
  out[15] = 1.0f;
}
void TransformPoint(const float m[16], const float p[3], float out[3]) {
  float x = p[0] * m[0] + p[1] * m[4] + p[2] * m[8] + m[12];
  float y = p[0] * m[1] + p[1] * m[5] + p[2] * m[9] + m[13];
  float z = p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14];
  out[0] = x;
  out[1] = y;
  out[2] = z;
}
void NormalizeQuat(float q[4]) {
  float length = std::sqrt(Dot4(q, q));
  if (!Finite(length) || length < 1.0e-12f) {
    q[0] = 0.0f;
    q[1] = 0.0f;
    q[2] = 0.0f;
    q[3] = 1.0f;
    return;
  }
  float inv = 1.0f / length;
  q[0] *= inv;
  q[1] *= inv;
  q[2] *= inv;
  q[3] *= inv;
}
void SlerpQuat(const float a[4], const float b[4], float t, float out[4]) {
  float qa[4];
  float qb[4];
  Copy4(a, qa);
  Copy4(b, qb);
  NormalizeQuat(qa);
  NormalizeQuat(qb);
  t = Clamp01(t);
  float dot = Dot4(qa, qb);
  if (dot < 0.0f) {
    qb[0] = -qb[0];
    qb[1] = -qb[1];
    qb[2] = -qb[2];
    qb[3] = -qb[3];
    dot = -dot;
  }
  dot = std::min(dot, 1.0f);
  if (dot > 0.9995f) {
    out[0] = qa[0] + (qb[0] - qa[0]) * t;
    out[1] = qa[1] + (qb[1] - qa[1]) * t;
    out[2] = qa[2] + (qb[2] - qa[2]) * t;
    out[3] = qa[3] + (qb[3] - qa[3]) * t;
    NormalizeQuat(out);
    return;
  }
  float omega = std::acos(dot);
  float sinOmega = std::sin(omega);
  if (!Finite(sinOmega) || std::fabs(sinOmega) < 1.0e-6f) {
    Copy4(qa, out);
    return;
  }
  float scaleA = std::sin((1.0f - t) * omega) / sinOmega;
  float scaleB = std::sin(t * omega) / sinOmega;
  out[0] = qa[0] * scaleA + qb[0] * scaleB;
  out[1] = qa[1] * scaleA + qb[1] * scaleB;
  out[2] = qa[2] * scaleA + qb[2] * scaleB;
  out[3] = qa[3] * scaleA + qb[3] * scaleB;
  NormalizeQuat(out);
}
bool ResetPose(const Skeleton& skeleton, Pose& out) {
  if (skeleton.joints.empty()) {
    return false;
  }
  out.joints.resize(skeleton.joints.size());
  for (size_t i = 0; i < skeleton.joints.size(); ++i) {
    const Joint& joint = skeleton.joints[i];
    JointPose& pose = out.joints[i];
    Copy3(joint.bindTranslation, pose.translation);
    Copy4(joint.bindRotation, pose.rotation);
    Copy3(joint.bindScale, pose.scale);
    NormalizeQuat(pose.rotation);
  }
  return true;
}
static bool SampleValidatedClip(const Skeleton& skeleton, const Clip& clip, float time, Pose& out) {
  if (!Finite(time) || !ResetPose(skeleton, out)) {
    return false;
  }
  const int jointCount = static_cast<int>(skeleton.joints.size());
  for (size_t c = 0; c < clip.channels.size(); ++c) {
    const Channel& channel = clip.channels[c];
    if (channel.joint < 0 || channel.joint >= jointCount || channel.keys.empty()) {
      return false;
    }
    const std::vector<Key>& keys = channel.keys;
    const Key* previous = &keys[0];
    const Key* next = &keys[0];
    float blend = 0.0f;
    if (time >= keys[keys.size() - 1].time) {
      previous = &keys[keys.size() - 1];
      next = previous;
    } else if (time > keys[0].time) {
      size_t low = 0;
      size_t high = keys.size();
      while (low < high) {
        size_t mid = (low + high) / 2;
        if (keys[mid].time <= time) {
          low = mid + 1;
        } else {
          high = mid;
        }
      }
      size_t nextIndex = low;
      size_t previousIndex = nextIndex - 1;
      previous = &keys[previousIndex];
      next = &keys[nextIndex];
      float span = next->time - previous->time;
      blend = span > 0.0f ? (time - previous->time) / span : 0.0f;
    }
    JointPose& joint = out.joints[static_cast<size_t>(channel.joint)];
    if (channel.interpolation == 1) {
      if (channel.path == Path::Rotation) {
        Copy4(previous->value, joint.rotation);
        NormalizeQuat(joint.rotation);
      } else if (channel.path == Path::Translation) {
        joint.translation[0] = previous->value[0];
        joint.translation[1] = previous->value[1];
        joint.translation[2] = previous->value[2];
      } else {
        joint.scale[0] = previous->value[0];
        joint.scale[1] = previous->value[1];
        joint.scale[2] = previous->value[2];
      }
      continue;
    }
    if (channel.path == Path::Rotation) {
      if (previous == next) {
        Copy4(previous->value, joint.rotation);
      } else {
        SlerpQuat(previous->value, next->value, blend, joint.rotation);
      }
      NormalizeQuat(joint.rotation);
    } else if (channel.path == Path::Translation) {
      if (previous == next) {
        joint.translation[0] = previous->value[0];
        joint.translation[1] = previous->value[1];
        joint.translation[2] = previous->value[2];
      } else {
        Lerp3(previous->value, next->value, blend, joint.translation);
      }
    } else {
      if (previous == next) {
        joint.scale[0] = previous->value[0];
        joint.scale[1] = previous->value[1];
        joint.scale[2] = previous->value[2];
      } else {
        Lerp3(previous->value, next->value, blend, joint.scale);
      }
    }
  }
  return true;
}
bool SampleClip(const Skeleton& skeleton, const Clip& clip, float time, Pose& out) {
  if (!skeleton.IsValid() || !clip.IsValid(skeleton) || !Finite(time)) {
    return false;
  }
  return SampleValidatedClip(skeleton, clip, time, out);
}
bool SampleClipUnchecked(const Skeleton& skeleton, const Clip& clip, float time, Pose& out) {
  if (skeleton.joints.empty()) {
    return false;
  }
  return SampleValidatedClip(skeleton, clip, time, out);
}
bool BlendPoses(const Pose& a, const Pose& b, float t, Pose& out) {
  if (a.joints.size() != b.joints.size()) {
    return false;
  }
  if (!Finite(t)) {
    return false;
  }
  t = Clamp01(t);
  out.joints.resize(a.joints.size());
  for (size_t i = 0; i < a.joints.size(); ++i) {
    const JointPose& ja = a.joints[i];
    const JointPose& jb = b.joints[i];
    JointPose& jo = out.joints[i];
    Lerp3(ja.translation, jb.translation, t, jo.translation);
    Lerp3(ja.scale, jb.scale, t, jo.scale);
    SlerpQuat(ja.rotation, jb.rotation, t, jo.rotation);
  }
  return true;
}
bool ComputeModelMatrices(const Skeleton& skeleton, const Pose& pose, std::vector<float>& out) {
  ModelScratch scratch;
  return ComputeModelMatrices(skeleton, pose, out, scratch);
}
bool ComputeModelMatrices(const Skeleton& skeleton, const Pose& pose, std::vector<float>& out, ModelScratch& scratch) {
  const size_t count = skeleton.joints.size();
  if (count == 0 || pose.joints.size() != count) {
    return false;
  }
  out.assign(count * 16, 0.0f);
  scratch.state.assign(count, 0);
  scratch.chain.clear();
  unsigned char* state = scratch.state.data();
  std::vector<int>& chain = scratch.chain;
  float* destination = out.data();
  for (size_t i = 0; i < count; ++i) {
    if (state[i] == 2) {
      continue;
    }
    chain.clear();
    int current = static_cast<int>(i);
    while (current >= 0) {
      if (current >= static_cast<int>(count)) {
        return false;
      }
      if (state[static_cast<size_t>(current)] == 1) {
        return false;
      }
      if (state[static_cast<size_t>(current)] == 2) {
        break;
      }
      state[static_cast<size_t>(current)] = 1;
      chain.push_back(current);
      current = skeleton.joints[static_cast<size_t>(current)].parent;
    }
    for (size_t c = chain.size(); c > 0; --c) {
      int index = chain[c - 1];
      const Joint& joint = skeleton.joints[static_cast<size_t>(index)];
      const JointPose& jointPose = pose.joints[static_cast<size_t>(index)];
      float local[16];
      ComposeTRS(jointPose.translation, jointPose.rotation, jointPose.scale, local);
      if (joint.parent >= 0) {
        MultiplyMatrix(local, destination + static_cast<size_t>(joint.parent) * 16, destination + static_cast<size_t>(index) * 16);
      } else {
        std::memcpy(destination + static_cast<size_t>(index) * 16, local, sizeof(local));
      }
      state[static_cast<size_t>(index)] = 2;
    }
  }
  return true;
}
bool SkinPosition(const Skeleton& skeleton, const std::vector<float>& modelMatrices, const SkinVertex& skin, const float bindPosition[3], float out[3]) {
  if (skeleton.joints.empty() || modelMatrices.size() != skeleton.joints.size() * 16) {
    return false;
  }
  float weightSum = 0.0f;
  for (int i = 0; i < 4; ++i) {
    float weight = skin.weights[i];
    if (!Finite(weight) || weight < 0.0f) {
      return false;
    }
    weightSum += weight;
  }
  if (weightSum < kEpsilon) {
    return false;
  }
  float result[3] = { 0.0f, 0.0f, 0.0f };
  for (int i = 0; i < 4; ++i) {
    float weight = skin.weights[i];
    if (weight <= 0.0f) {
      continue;
    }
    uint32_t jointIndex = skin.joints[i];
    if (jointIndex >= skeleton.joints.size()) {
      return false;
    }
    float bindSpace[3];
    TransformPoint(skeleton.joints[jointIndex].inverseBind, bindPosition, bindSpace);
    float skinned[3];
    TransformPoint(modelMatrices.data() + static_cast<size_t>(jointIndex) * 16, bindSpace, skinned);
    result[0] += skinned[0] * weight;
    result[1] += skinned[1] * weight;
    result[2] += skinned[2] * weight;
  }
  out[0] = result[0] / weightSum;
  out[1] = result[1] / weightSum;
  out[2] = result[2] / weightSum;
  return true;
}
}
const char* Animation_Version() {
  return "0.1.0-fase10";
}
bool Animation_SelfTest() {
  using namespace Anim;
  float identity[16];
  IdentityMatrix(identity);
  if (!MatrixNearIdentity(identity, 1.0e-6f)) {
    return false;
  }
  float rotation[4] = { 0.0f, 0.0f, 0.70710678f, 0.70710678f };
  float translation[3] = { 1.0f, 2.0f, 3.0f };
  float scale[3] = { 2.0f, 2.0f, 2.0f };
  float transform[16];
  ComposeTRS(translation, rotation, scale, transform);
  float inverse[16];
  if (!InvertMatrix(transform, inverse)) {
    return false;
  }
  float product[16];
  MultiplyMatrix(transform, inverse, product);
  if (!MatrixNearIdentity(product, 1.0e-4f)) {
    return false;
  }
  float slerped[4];
  float identityRotation[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
  SlerpQuat(identityRotation, rotation, 0.5f, slerped);
  if (!NearlyEqual(slerped[2], 0.38268343f, 1.0e-4f) || !NearlyEqual(slerped[3], 0.92387953f, 1.0e-4f)) {
    return false;
  }
  Skeleton skeleton;
  Joint root;
  root.name = "root";
  Joint child;
  child.name = "child";
  child.parent = 0;
  child.bindTranslation[0] = 1.0f;
  child.inverseBind[12] = -1.0f;
  skeleton.joints.push_back(root);
  skeleton.joints.push_back(child);
  if (!skeleton.IsValid() || skeleton.FindJoint("child") != 1 || skeleton.FindJoint("missing") != -1) {
    return false;
  }
  Clip clip;
  clip.name = "rotate";
  Channel rotationChannel;
  rotationChannel.joint = 0;
  rotationChannel.path = Path::Rotation;
  Key key0;
  key0.time = 0.0f;
  key0.value[3] = 1.0f;
  Key key1;
  key1.time = 1.0f;
  key1.value[2] = 0.70710678f;
  key1.value[3] = 0.70710678f;
  rotationChannel.keys.push_back(key0);
  rotationChannel.keys.push_back(key1);
  clip.channels.push_back(rotationChannel);
  Channel translationChannel;
  translationChannel.joint = 1;
  translationChannel.path = Path::Translation;
  Key move0;
  move0.time = 0.0f;
  move0.value[0] = 1.0f;
  Key move1;
  move1.time = 1.0f;
  move1.value[0] = 3.0f;
  translationChannel.keys.push_back(move0);
  translationChannel.keys.push_back(move1);
  clip.channels.push_back(translationChannel);
  if (!clip.IsValid(skeleton) || !NearlyEqual(clip.Duration(), 1.0f, 1.0e-6f)) {
    return false;
  }
  Pose pose;
  if (!SampleClip(skeleton, clip, 0.5f, pose)) {
    return false;
  }
  if (!NearlyEqual(pose.joints[0].rotation[2], 0.38268343f, 1.0e-4f) || !NearlyEqual(pose.joints[1].translation[0], 2.0f, 1.0e-4f)) {
    return false;
  }
  Clip stepClip;
  Channel stepChannel;
  stepChannel.joint = 1;
  stepChannel.path = Path::Translation;
  stepChannel.interpolation = 1;
  Key step0;
  step0.time = 0.0f;
  step0.value[0] = 1.0f;
  Key step1;
  step1.time = 1.0f;
  step1.value[0] = 3.0f;
  stepChannel.keys.push_back(step0);
  stepChannel.keys.push_back(step1);
  stepClip.channels.push_back(stepChannel);
  Pose stepPose;
  if (!SampleClip(skeleton, stepClip, 0.5f, stepPose) || !NearlyEqual(stepPose.joints[1].translation[0], 1.0f, 1.0e-6f)) {
    return false;
  }
  if (!SampleClip(skeleton, stepClip, 1.0f, stepPose) || !NearlyEqual(stepPose.joints[1].translation[0], 3.0f, 1.0e-6f)) {
    return false;
  }
  std::vector<float> matrices;
  if (!ComputeModelMatrices(skeleton, pose, matrices)) {
    return false;
  }
  if (!NearlyEqual(matrices[28], 1.41421356f, 1.0e-3f) || !NearlyEqual(matrices[29], 1.41421356f, 1.0e-3f)) {
    return false;
  }
  SkinVertex skinVertex;
  skinVertex.joints[0] = 1;
  skinVertex.weights[0] = 1.0f;
  float bindPosition[3] = { 1.0f, 0.0f, 0.0f };
  float skinned[3];
  if (!SkinPosition(skeleton, matrices, skinVertex, bindPosition, skinned)) {
    return false;
  }
  if (!NearlyEqual(skinned[0], 1.41421356f, 1.0e-3f) || !NearlyEqual(skinned[1], 1.41421356f, 1.0e-3f)) {
    return false;
  }
  SkinVertex invalidSkin;
  invalidSkin.joints[0] = 1;
  invalidSkin.weights[0] = 0.0f;
  if (SkinPosition(skeleton, matrices, invalidSkin, bindPosition, skinned)) {
    return false;
  }
  Pose poseA;
  Pose poseB;
  if (!ResetPose(skeleton, poseA) || !ResetPose(skeleton, poseB)) {
    return false;
  }
  poseA.joints[1].translation[0] = 1.0f;
  poseB.joints[1].translation[0] = 3.0f;
  Pose blended;
  if (!BlendPoses(poseA, poseB, 0.25f, blended) || !NearlyEqual(blended.joints[1].translation[0], 1.5f, 1.0e-4f)) {
    return false;
  }
  Clip invalidClip;
  Channel invalidChannel;
  invalidChannel.joint = 0;
  invalidChannel.path = Path::Translation;
  Key invalid0;
  invalid0.time = 1.0f;
  Key invalid1;
  invalid1.time = 1.0f;
  invalidChannel.keys.push_back(invalid0);
  invalidChannel.keys.push_back(invalid1);
  invalidClip.channels.push_back(invalidChannel);
  if (invalidClip.IsValid(skeleton)) {
    return false;
  }
  Skeleton cyclic;
  Joint cycleA;
  cycleA.name = "a";
  cycleA.parent = 1;
  Joint cycleB;
  cycleB.name = "b";
  cycleB.parent = 0;
  cyclic.joints.push_back(cycleA);
  cyclic.joints.push_back(cycleB);
  Pose cyclicPose;
  cyclicPose.joints.resize(2);
  std::vector<float> cyclicMatrices;
  if (cyclic.IsValid() || ComputeModelMatrices(cyclic, cyclicPose, cyclicMatrices)) {
    return false;
  }
  return true;
}
}
