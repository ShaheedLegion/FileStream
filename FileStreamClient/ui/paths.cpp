#include "paths.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace util {

Paths::Paths() {}

Paths::~Paths() {}

void Paths::getPaths(std::vector<std::string> &paths,
                     const std::string &fName) {
  paths.push_back(fName);

  char file_name[MAX_PATH];
  memset(file_name, 0, MAX_PATH);
  if (GetModuleFileName(NULL, file_name, MAX_PATH) != 0) {
    // Try to load the absolute path.
    std::string fp(file_name);
    std::string::size_type position(fp.find_last_of("\\"));
    if (position != std::string::npos) {
      // Append the file name to the absolute path.
      std::string path(fp.substr(0, position + 1));
      path.append(fName);
      paths.push_back(path);
    }
  }
}

} // namespace util