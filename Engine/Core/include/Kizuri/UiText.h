#pragma once
#include <cstddef>
#include <string>
namespace Kizuri {
inline void ClearTextBuffer(char* buffer, std::size_t size) {
  if (buffer != nullptr && size > 0) {
    buffer[0] = '\0';
  }
}
inline void AppendComboItem(std::string& items, const std::string& label) {
  items.push_back('\0');
  items += label;
}
inline void FinishComboItems(std::string& items) {
  items.push_back('\0');
}
}
