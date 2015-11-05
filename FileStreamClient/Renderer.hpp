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
#include "../print_structs.hpp"

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

class ScreenObserver {
public:
  virtual ~ScreenObserver() {}

  virtual void HandleMouse(int x, int y, bool l, bool m, bool r) = 0;
  virtual bool HandleDrag(int x, int y, bool l) = 0;
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

  bool RenderText(const print::PrintInfo &text, int x, int y, int mx, int my,
                  bool l, bool m, bool r) {
    bool hover = (my >= y && my <= y + 16);
    bool clicked = l && hover;

    m_textInfo.push_back(textInfo(text.text, x, y, hover && text.clickable));

    return clicked;
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
        if (i.hover) {
          int oldmode = GetBkMode(m_dc);
          SetBkMode(m_dc, OPAQUE);
          SetBkColor(m_dc, RGB(64, 64, 255));
          TextOut(m_dc, i.x, i.y, i.t.c_str(), i.t.length());
          SetBkMode(m_dc, oldmode);
        } else {
          TextOut(m_dc, i.x, i.y, i.t.c_str(), i.t.length());
        }
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
    // Unfortunately our window is soft scaled, so we will have to soft scale
    // the mouse coordinates to appear to match the original window dimensions.
    int scaledX, scaledY;
    getScaledWindowCoords(x, y, scaledX, scaledY);

    for (auto i : m_observers)
      i->HandleMouse(scaledX, scaledY, l, m, r);
  }

  bool HandleDrag(int x, int y, bool l) {
    int scaledX, scaledY;
    getScaledWindowCoords(x, y, scaledX, scaledY);

    bool dragged = false;
    for (auto i : m_observers)
      dragged |= i->HandleDrag(scaledX, scaledY, l);

    return dragged;
  }

  void RegisterObserver(ScreenObserver *obs) { m_observers.push_back(obs); }

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
    bool hover;

    textInfo(const std::string &y, int u, int v, bool h)
        : t(y), x(u), y(v), hover(h) {}
  };

  void getScaledWindowCoords(int x, int y, int &sx, int &sy) {
    float scaleX = static_cast<float>(_WIDTH) / static_cast<float>(_WW);
    float scaleY = static_cast<float>(_HEIGHT) / static_cast<float>(_WH);

    sx = static_cast<int>(static_cast<float>(x) * scaleX);
    sy = static_cast<int>(static_cast<float>(y) * scaleY);
  }

  std::vector<ScreenObserver *> m_observers;
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
           detail::IBitmapRenderer *renderer, bool showConsole);

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

void HandleMouse(WPARAM wp, LPARAM lp, bool l, bool r, bool m) {
  if (!g_renderer)
    return;

  int x = GET_X_LPARAM(lp);
  int y = GET_Y_LPARAM(lp);
  // These are virtual keycodes - useful for keyboard mouse emulation.
  // bool left = (wp & MK_LBUTTON);
  // bool middle = (wp & MK_MBUTTON);
  // bool right = (wp & MK_RBUTTON);

  g_renderer->screen.HandleMouse(x, y, l, m, r);
}

bool HandleDrag(WPARAM wp, LPARAM lp, int x, int y, bool l) {
  if (!g_renderer)
    return false;

  return g_renderer->screen.HandleDrag(x, y, l);
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
  case WM_LBUTTONDOWN:
    HandleMouse(wp, lp, true, false, false);
    return 0L;
  case WM_MBUTTONDOWN:
    HandleMouse(wp, lp, false, true, false);
    return 0L;
  case WM_RBUTTONDOWN:
    HandleMouse(wp, lp, false, false, true);
    return 0L;
  case WM_MOUSEMOVE:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    HandleMouse(wp, lp, false, false, false);
    return 0L;
  case WM_NCHITTEST: {
    LRESULT hit = DefWindowProc(window, msg, wp, lp);
    if (hit == HTCLIENT) {
      POINT point;
      GetCursorPos(&point);
      if (ScreenToClient(window, &point) &&
          HandleDrag(wp, lp, point.x, point.y, (GetKeyState(VK_LBUTTON) < 0)))
        hit = HTCAPTION;
      return hit;
    }
  } break;
  default:
    return DefWindowProc(window, msg, wp, lp);
  }

  return 0;
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
                   detail::IBitmapRenderer *renderer, bool showConsole)
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

      if (!showConsole)
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
