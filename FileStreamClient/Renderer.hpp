// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#pragma once
#ifndef RENDERER_HPP_INCLUDED
#define RENDERER_HPP_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>
#include <string>
#include <vector>
#include <iostream>
#include "thread.hpp"

#define _WIDTH 800
#define _HEIGHT 600
#define _WW (1024)
#define _WH (768)
#define _BPP 32

namespace detail {

class IBitmapRenderer {
public:
  virtual ~IBitmapRenderer() {}
  virtual void RenderToBitmap(HDC screenDC, int w, int h) = 0;
  virtual void HandleOutput(VOID *output) = 0;
  virtual void HandleDirection(int direction) = 0;
};

typedef unsigned int Uint32;

class RendererSurface {
public:
  RendererSurface(int w, int h, int bpp, IBitmapRenderer *renderer)
      : m_w(w), m_h(h), m_bpp(bpp), m_bitmapRenderer(renderer),
        m_shouldMinimize(false), m_shouldFlash(false) {
    m_pixels = new unsigned char[m_w * m_h * (m_bpp / 8)];
    memset(m_pixels, 0, (m_w * m_h * (m_bpp / 8)));
    m_backBuffer = new unsigned char[m_w * m_h * (m_bpp / 8)];
    memset(m_backBuffer, 0, (m_w * m_h * (m_bpp / 8)));
  }

  ~RendererSurface() {}

  void Cleanup() {
    delete[] m_pixels;
    delete[] m_backBuffer;
    std::cout << "Destroying surface";
    m_pixels = nullptr;
    m_backBuffer = nullptr;
    m_bitmapRenderer = nullptr;
  }

  void SetScreen(unsigned char *buffer, HDC screenDC, HDC memDC) {
    m_screen = buffer;
    m_screenDC = screenDC;
    m_dc = memDC;

    // Set a nice font here.
    long height = -MulDiv(12, GetDeviceCaps(m_dc, LOGPIXELSY), 72);
    HFONT font = CreateFont(height, 0, 0, 0, FW_THIN, 0, 0, 0, 0, 200, 0,
                            PROOF_QUALITY, 0, "Courier New");
    SelectObject(m_dc, font);
  }

  void SetDirection(int direction) {
    if (m_bitmapRenderer)
      m_bitmapRenderer->HandleDirection(direction);
  }

  void RenderText(const std::string &text, int x, int y) {
    m_textInfo.push_back(textInfo(text, x, y));
  }

  void Flip(bool clear = false) {
    if (!m_pixels || !m_backBuffer)
      return;
    // We need a mechanism to actually present the buffer to the drawing system.
    unsigned char *temp = m_pixels;
    m_pixels = m_backBuffer;
    m_backBuffer = temp;

    memcpy(m_screen, m_pixels, (m_w * m_h * (m_bpp / 8)));
    if (m_bitmapRenderer)
      m_bitmapRenderer->RenderToBitmap(m_dc, m_w, m_h);

    if (!m_textInfo.empty()) {
      for (auto &i : m_textInfo) {
        TextOut(m_dc, i.x, i.y, i.t.c_str(), i.t.length());
      }
      m_textInfo.clear();
    }

    // BitBlt(m_screenDC, 0, 0, m_w, m_h, m_dc, 0, 0, SRCCOPY);
    StretchBlt(m_screenDC, 0, 0, _WW, _WH, m_dc, 0, 0, m_w, m_h, SRCCOPY);

    if (clear)
      memset(m_backBuffer, 0, (m_w * m_h * (m_bpp / 8)));
  }

  Uint32 *GetPixels() { return reinterpret_cast<Uint32 *>(m_backBuffer); }

  int GetBPP() const { return m_bpp; }

  int GetWidth() const { return m_w; }

  int GetHeight() const { return m_h; }

  IBitmapRenderer *GetRenderer() const { return m_bitmapRenderer; }

  void HandleInput(WPARAM wp, bool pressed) {
    m_keyInfo.push_back(std::pair<WPARAM, bool>(wp, pressed));
  }

  void HandleText(WPARAM wp, LPARAM lp) {
    m_charInfo.push_back(std::pair<WPARAM, LPARAM>(wp, lp));
  }

  void GetInput(std::vector<std::pair<WPARAM, bool>> &keys) {
    keys = m_keyInfo;
    m_keyInfo.clear();
  }

  void GetCharInput(std::vector<std::pair<WPARAM, LPARAM>> &chars) {
    chars = m_charInfo;
    m_charInfo.clear();
  }

  void Minimize(bool min) { m_shouldMinimize = min; }
  bool GetMinimized() const { return m_shouldMinimize; }
  void FlashWindow(bool flash) { m_shouldFlash = flash; }
  bool GetFlashWindow() const { return m_shouldFlash; }

  // Get/Set the state of the app mouse tracking/info
  void HandleMouse(int x, int y, bool l, bool m, bool r) {
    m_mouse.set(x, y, l, m, r);
  }

  void QueryMouse(int &x, int &y, bool &l, bool &m, bool &r) {
    m_mouse.get(x, y, l, m, r);
  }

protected:
  unsigned char *m_pixels;
  unsigned char *m_backBuffer;
  unsigned char *m_screen;
  int m_w;
  int m_h;
  int m_bpp;
  HDC m_screenDC;
  HDC m_dc;
  IBitmapRenderer *m_bitmapRenderer;

  // Window state modifiers. This is the only tie we have to win32, so these
  // have to run through the screen. It's not ideal, but it's a valid shortcut.
  bool m_shouldMinimize;
  bool m_shouldFlash;

  struct textInfo {
    std::string t;
    int x;
    int y;

    textInfo(const std::string &y, int u, int v) : t(y), x(u), y(v) {}
  };
  struct mouseInfo {
  private:
    CRITICAL_SECTION sec;
    int m_x;
    int m_y;
    bool m_l;
    bool m_m;
    bool m_r;

  public:
    mouseInfo() : m_x(0), m_y(0), m_l(false), m_m(false), m_r(false) {
      InitializeCriticalSection(&sec);
    }

    ~mouseInfo() { DeleteCriticalSection(&sec); }

    void set(int x, int y, bool l, bool m, bool r) {
      EnterCriticalSection(&sec);
      m_x = x;
      m_y = y;
      m_l = l;
      m_m = m;
      m_r = r;
      LeaveCriticalSection(&sec);
    }

    void get(int &x, int &y, bool &l, bool &m, bool &r) {
      EnterCriticalSection(&sec);
      x = m_x;
      y = m_y;
      l = m_l;
      m = m_m;
      r = m_r;
      LeaveCriticalSection(&sec);
    }
  };

  mouseInfo m_mouse;
  std::vector<textInfo> m_textInfo;
  std::vector<std::pair<WPARAM, bool>> m_keyInfo;
  std::vector<std::pair<WPARAM, LPARAM>> m_charInfo;
};

} // namespace detail

// Declaration and partial implementation.
class Renderer {
public:
  detail::RendererThread updateThread;
  detail::RendererSurface screen;

  bool bRunning;

  void SetBuffer(unsigned char *buffer, HDC scrDC, HDC memDC) {
    screen.SetScreen(buffer, scrDC, memDC);
    updateThread.Start(static_cast<LPVOID>(this));
    SetRunning(true);
  }
  void SetDirection(int direction) { screen.SetDirection(direction); }

public:
  Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
           detail::IBitmapRenderer *renderer);

  ~Renderer() {
    updateThread.Join();
    screen.Cleanup();
  }

  bool IsRunning() { return bRunning; }

  void SetRunning(bool bRun) { bRunning = bRun; }
};

// Here we declare the functions and variables used by the renderer instance
namespace forward {
Renderer *g_renderer;

void HandleKey(WPARAM wp, bool pressed) {

  if (wp == VK_ESCAPE)
    PostQuitMessage(0);

  if (!g_renderer)
    return;

  g_renderer->screen.HandleInput(wp, pressed);
}

void HandleText(WPARAM wp, LPARAM lp) {

  if (!g_renderer)
    return;

  g_renderer->screen.HandleText(wp, lp);
}

void HandleMouse(WPARAM wp, LPARAM lp) {
  if (!g_renderer)
    return;

  int x = GET_X_LPARAM(lp);
  int y = GET_Y_LPARAM(lp);
  bool left = (wp & MK_LBUTTON);
  bool middle = (wp & MK_MBUTTON);
  bool right = (wp & MK_RBUTTON);

  g_renderer->screen.HandleMouse(x, y, left, middle, right);
}

long __stdcall WindowProcedure(HWND window, unsigned int msg, WPARAM wp,
                               LPARAM lp) {
  switch (msg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0L;
  case WM_CHAR:
    HandleText(wp, lp);
    return 0L;
  case WM_KEYDOWN:
    HandleKey(wp, true);
    return 0L;
  case WM_KEYUP:
    HandleKey(wp, false);
    return 0L;
  case WM_MOUSEMOVE:
    HandleMouse(wp, lp);
    return 0L;
  default:
    return DefWindowProc(window, msg, wp, lp);
  }
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData) {
  if (!dwData || !lprcMonitor)
    return TRUE;

  std::vector<RECT> *monitors = reinterpret_cast<std::vector<RECT> *>(dwData);
  RECT rct{lprcMonitor->left, lprcMonitor->top, lprcMonitor->right,
           lprcMonitor->bottom};
  monitors->push_back(rct);

  return TRUE;
}
} // namespace forward

// Implementation of the renderer functions.
Renderer::Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
                   detail::IBitmapRenderer *renderer)
    : screen(_WIDTH, _HEIGHT, _BPP, renderer), updateThread(callback) {
  forward::g_renderer = this;

  HDC windowDC;

  WNDCLASSEX wndclass = {sizeof(WNDCLASSEX), CS_DBLCLKS,
                         forward::WindowProcedure, 0, 0, GetModuleHandle(0),
                         LoadIcon(0, IDI_APPLICATION), LoadCursor(0, IDC_ARROW),
                         HBRUSH(COLOR_WINDOW + 1), 0, className,
                         LoadIcon(0, IDI_APPLICATION)};
  if (RegisterClassEx(&wndclass)) {
    HWND window = 0;
    {
      RECT displayRC = {0, 0, _WIDTH, _HEIGHT};
      // Get info on which monitor we want to use.

      std::vector<RECT> monitors;
      EnumDisplayMonitors(NULL, NULL, forward::MonitorEnumProc,
                          reinterpret_cast<DWORD>(&monitors));

      std::vector<RECT>::iterator i = monitors.begin();
      if (i != monitors.end())
        displayRC = *i;

      // Now we want to center the window in the display rect.
      int x = displayRC.left +
              (((displayRC.right - displayRC.left) / 2) - (_WW / 2));
      int y = displayRC.top +
              (((displayRC.bottom - displayRC.top) / 2) - (_WH / 2));

      displayRC.left = x;
      displayRC.top = y;
      displayRC.right = displayRC.left + _WW;
      displayRC.bottom = displayRC.top + _WH;

      window = CreateWindowEx(0, className, className, WS_POPUPWINDOW,
                              displayRC.left, displayRC.top, _WW, _WH, 0, 0,
                              GetModuleHandle(0), 0);
    }
    if (window) {

      windowDC = GetWindowDC(window);
      HDC hImgDC = CreateCompatibleDC(windowDC);
      if (hImgDC == NULL) {
        MessageBox(NULL, "Dc is NULL", "ERROR!", MB_OK);
        return;
      }
      SetBkMode(hImgDC, TRANSPARENT);
      SetTextColor(hImgDC, RGB(255, 255, 255));
      SetStretchBltMode(hImgDC, COLORONCOLOR);

      BITMAPINFO bf;
      ZeroMemory(&bf, sizeof(BITMAPINFO));

      bf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bf.bmiHeader.biWidth = _WIDTH;
      bf.bmiHeader.biHeight = -_HEIGHT;
      bf.bmiHeader.biPlanes = 1;
      bf.bmiHeader.biBitCount = _BPP;
      bf.bmiHeader.biCompression = BI_RGB;
      bf.bmiHeader.biSizeImage = (_WIDTH * _HEIGHT * (_BPP / 8));
      bf.bmiHeader.biXPelsPerMeter = -1;
      bf.bmiHeader.biYPelsPerMeter = -1;

      unsigned char *bits;

      HBITMAP hImg = CreateDIBSection(hImgDC, &bf, DIB_RGB_COLORS,
                                      (void **)&bits, NULL, 0);
      if (hImg == NULL) {
        MessageBox(NULL, "Image is NULL", "ERROR!", MB_OK);
        return;
      } else if (hImg == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, "Image is invalid", "Error!", MB_OK);
        return;
      }

      SelectObject(hImgDC, hImg);

      SetBuffer(bits, windowDC, hImgDC);
      ShowWindow(GetConsoleWindow(), SW_HIDE);
      ShowWindow(window, SW_SHOWDEFAULT);
      MSG msg;
      while (GetMessage(&msg, 0, 0, 0) && IsRunning()) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (screen.GetMinimized()) {
          // Minimize the window immediately, then reset the screen state.
          ShowWindow(window, SW_MINIMIZE);
          screen.Minimize(false);
        }
        if (screen.GetFlashWindow()) {
          // Flash the window immediately, then reset the screen state.
          FlashWindow(window, TRUE);
          screen.FlashWindow(false);
        }
      }
    }
  }
  SetRunning(false);
  forward::g_renderer = nullptr;
}

#endif // RENDERER_HPP_INCLUDED
