// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#ifndef DETAIL_THREAD
#define DETAIL_THREAD

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace detail {

class RendererThread {
public:
  RendererThread(LPTHREAD_START_ROUTINE callback)
      : m_running(false), hThread(INVALID_HANDLE_VALUE), m_callback(callback) {}

  ~RendererThread() {}

  void Start(LPVOID lParam) {
    hThread = CreateThread(NULL, 0, m_callback, lParam, 0, NULL);
    m_running = true;
  }
  void Join() {
    if (m_running)
      m_running = false;

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    hThread = INVALID_HANDLE_VALUE;
  }

  void Delay(DWORD millis) const {
    // Try to delay for |millis| time duration.
    // This is called from within the threading function (callback)
    // So it's safe to sleep in the calling thread.
    Sleep(millis);
  }

  bool isRunning() const { return m_running; }

protected:
  bool m_running;
  HANDLE hThread;
  LPTHREAD_START_ROUTINE m_callback;
};

} // namespace detail

#endif // DETAIL_THREAD