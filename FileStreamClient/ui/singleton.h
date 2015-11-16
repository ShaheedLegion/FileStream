#pragma once
#ifndef UI_LIB_SINGLETON
#define UI_LIB_SINGLETON

namespace detail {

template <class T> class Singleton {

public:
  static T *getInstance() {
    static T *instance{nullptr};
    if (instance == nullptr)
      instance = new T();

    return instance;
  }
};

} // namespace detail

#endif // UI_LIB_SINGLETON