#ifndef _PRINT_STRUCTS_HPP
#define _PRINT_STRUCTS_HPP
#pragma once
#include <string>
#include <vector>

namespace print {
struct PrintInfo {
  std::string text;
  std::string bgInfo;

  bool clickable;

  PrintInfo(const std::string &text, const std::string &data, bool clickable)
      : text(text), bgInfo(data), clickable(clickable) {}
};

typedef std::vector<PrintInfo> printQueue;

} // namespace print

#endif // _PRINT_STRUCTS_HPP