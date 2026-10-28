#pragma once
namespace Kizuri {
struct WindowPlacement {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};
inline float DpiScaleFromValue(unsigned int dpi) {
  if (dpi < 96u) {
    return 1.0f;
  }
  return static_cast<float>(dpi) / 96.0f;
}
inline WindowPlacement FitWindowToWorkArea(int workX, int workY, int workWidth, int workHeight, int desiredWidth, int desiredHeight) {
  int width = desiredWidth > 0 ? desiredWidth : 1;
  int height = desiredHeight > 0 ? desiredHeight : 1;
  if (workWidth <= 0 || workHeight <= 0) {
    WindowPlacement fallback;
    fallback.x = workX;
    fallback.y = workY;
    fallback.width = width;
    fallback.height = height;
    return fallback;
  }
  const int maxWidth = workWidth * 92 / 100;
  const int maxHeight = workHeight * 92 / 100;
  if (width > maxWidth) {
    width = maxWidth;
  }
  if (height > maxHeight) {
    height = maxHeight;
  }
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  WindowPlacement placement;
  placement.x = workX + (workWidth - width) / 2;
  placement.y = workY + (workHeight - height) / 2;
  placement.width = width;
  placement.height = height;
  return placement;
}
}
