#pragma once
#ifndef UI_LIB_PATHS
#define UI_LIB_PATHS

#include "singleton.h"
#include <vector>

namespace util {

class Paths : public detail::Singleton<Paths> {

  Paths();

  ~Paths();

  friend class detail::Singleton<Paths>;

public:
  void getPaths(std::vector<std::string> &paths, const std::string &fName);
};

} // namespace util

#endif // UI_LIB_PATHS