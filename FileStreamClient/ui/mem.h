#pragma once
#ifndef UI_LIB_MEM
#define UI_LIB_MEM

#include "singleton.h"
#include <map>
#include <vector>

namespace mem {

class Memory : public detail::Singleton<Memory> {
  int poolBytes;
  unsigned char *pool;
  unsigned char *end;
  static const int megs = (1024 * 1024 * 10);

  Memory() : poolBytes(megs), pool(nullptr), end(nullptr) {
    pool = new unsigned char[poolBytes];
    end = pool;
    memset(pool, 0, poolBytes / sizeof(unsigned int));
  }

  ~Memory() { delete[] pool; }

  friend class detail::Singleton<Memory>;

public:
  // Allocate a chunk of this memory to whatever purpose.
  unsigned char *alloc(int bytes) {
    unsigned char *ret{0};
    if (bytes >= poolBytes) // Bogus allocation
      return ret;

    // We could find a chunk that fits somewhere at the end of the block.
    if ((pool + poolBytes) - (end + sizeof(int) + sizeof(bool)) >= bytes) {
      int *start{reinterpret_cast<int *>(end)};
      *start = bytes;
      ret = end + sizeof(int);
      *ret = true; // memory is allocated.
      ++ret;
      end += bytes + sizeof(int) + sizeof(bool);
      return ret;
    }

// TODO(shaheed.abdol) - Add alloc/dealloc scheme.
#if 0
    // We couldn't find a free chunk at the end of the block.
    // Walk through the allocated block to find a freed chunk.
    {
      unsigned char *alias = m_pool;
      int *block_size{reinterpret_cast<int *>(alias)};
      bool notfound = true;
      while (notfound) {
        int next_block{*block_size};
        if (next_block == 0)
          return ret; // something went mad wrong.

        alias += sizeof(int);
        bool allocated{*reinterpret_cast<bool *>(alias)};
        if (!allocated) {
          // Allocate the block of memory ...
        }
        // ...
      }
    }
#endif // 0
    return ret;
  }

  // Length is measured in sizeof unsigned int.
  // Manually unrolled the loop for a bit of speed.
  inline void memcpy(void *dst, void *src, int len) {
    unsigned int *ds_a{reinterpret_cast<unsigned int *>(dst)};
    unsigned int *sr_a{reinterpret_cast<unsigned int *>(src)};
    int half_len{len >> 1};
    while (half_len) {
      *ds_a = *sr_a;
      ++ds_a;
      ++sr_a;
      *ds_a = *sr_a;
      ++ds_a;
      ++sr_a;
      --half_len;
    }
  }

  // Length is measured in sizeof unsigned int.
  // Manually unrolled the loop for a bit of speed.
  inline void memset(void *dst, unsigned int v, int len) {
    unsigned int *ds_a{reinterpret_cast<unsigned int *>(dst)};
    int half_len{len >> 1};
    while (half_len) {
      *ds_a = v;
      ++ds_a;
      *ds_a = v;
      ++ds_a;
      --half_len;
    }
  }
};

} // namespace detail

#endif // UI_LIB_MEM