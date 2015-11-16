#pragma once
#ifndef UI_LIB_TEX_LOADER
#define UI_LIB_TEX_LOADER

#include "detail.h"
#include "mem.h"
#include "paths.h"
#include "singleton.h"
#include <map>
#include <vector>

namespace detail {

class TextureLoader : public Singleton<TextureLoader> {
  std::map<std::string, Texture *> textures;

  TextureLoader() {}
  ~TextureLoader() {}

  friend class detail::Singleton<TextureLoader>;

public:
  Texture &getTexture(const std::string &name) {
    if (textures.find(name) != textures.end())
      return *textures[name];

    if (!name.empty()) {
      // Actually load the texture - this is more involved.
      std::vector<std::string> paths;
      util::Paths::getInstance()->getPaths(paths, name);

      // Open the file here.
      FILE *input(0);

      for (auto path : paths) {
        input = fopen(path.c_str(), "rb");
        if (input == nullptr)
          continue;

        int w, h;
        fread(&w, 4, 1, input);
        fread(&h, 4, 1, input);

        int len = w * h;
        detail::Uint32 *buffer = reinterpret_cast<detail::Uint32 *>(
            mem::Memory::getInstance()->alloc(len * sizeof(detail::Uint32)));

        if (buffer) {
          // Now that we have width and height, we can start reading pixels.
          fread(buffer, sizeof(detail::Uint32), len, input);
          fclose(input);

          textures[name] = new detail::Texture(w, h, buffer);
          break;
        }
      }
    }

    // We need to get the file path for this thing.
    if (textures.find(name) == textures.end() && name.empty()) {
      textures[name] = new detail::Texture(0, 0, nullptr);
    }

    return *textures[name];
  }
};

} // namespace detail

#endif // UI_LIB_TEX_LOADER