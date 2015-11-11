// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#ifndef UI
#define UI

#include "../Renderer.hpp"
#include "../util/util.hpp"
#include "../../print_structs.hpp"

#include <shellapi.h>
#include <objbase.h>

namespace ui {

struct texture {
  detail::Uint32 *tex;
  int bounds[2];
  int frames;
  int frameHeight;
  bool validData;

  bool hasData() const { return validData; }

  texture(int w, int h) : tex(nullptr) {
    bounds[0] = w;
    bounds[1] = h;
    frames = 1;
    frameHeight = bounds[1];
    tex = new detail::Uint32[bounds[0] * bounds[1]];
    validData = true;
  }

  texture(const texture &other) : tex(nullptr) {
    bounds[0] = other.bounds[0];
    bounds[1] = other.bounds[1];
    frames = other.frames;
    frameHeight = other.frameHeight;
    tex = new detail::Uint32[bounds[0] * bounds[1]];
    memcpy(tex, other.tex, (bounds[0] * bounds[1]) * sizeof(detail::Uint32));
    validData = true;
  }

  texture(const std::string &name) : tex(nullptr) {
    // Load a texture resource from a file with the given name. First tries
    // searching in whichever folder is the current working directory, if that
    // fails it will try searching in the module folder (path to .exe file)
    validData = false;
    if (name.empty())
      return;

    // Open the file here.
    FILE *input(0);

    errno_t err(fopen_s(&input, name.c_str(), "rb"));
    if (err != 0) {
      std::cout << "Could not load texture resource " << name << std::endl;
      char file_name[MAX_PATH];
      memset(file_name, 0, MAX_PATH);
      if (GetModuleFileName(NULL, file_name, MAX_PATH) != 0) {
        std::cout << "Module path[" << file_name << "]" << std::endl;

        // Try to load the absolute path.
        std::string fp(file_name);
        std::string::size_type position(fp.find_last_of("\\"));
        if (position != std::string::npos) {
          // Append the file name to the absolute path.
          std::string path(fp.substr(0, position + 1));
          path.append(name);
          std::cout << "Trying to load from [" << path << "]" << std::endl;
          if (fopen_s(&input, path.c_str(), "rb") != 0) {
            std::cout << "Load failed." << std::endl;
            return; // could not open the file.
          }
        } else
          return; // could not find the slashes.
      } else
        return; // could not get the path to the exe.
    }

    fread(&bounds[0], 4, 1, input);
    fread(&bounds[1], 4, 1, input);

    std::cout << "Loaded texture resource [" << name << "] w[" << bounds[0]
              << "] h[" << bounds[1] << "]" << std::endl;
    int len = bounds[0] * bounds[1];
    tex = new detail::Uint32[len];
    // Now that we have width and height, we can start reading pixels.
    fread(tex, sizeof(detail::Uint32), len, input);

    fclose(input);
    validData = true;
  }

  void clear() {
    int len(bounds[0] * bounds[1]);
    memset(tex, 0, len * sizeof(detail::Uint32));
  }

  void copyFrame(detail::Uint32 *out, int w, int x, int y, int c, int max) {
    frames = max;
    frameHeight = bounds[1] / max;
    if (c > max || w < bounds[0])
      return;

    int frameHeight = bounds[1] / max;
    int rowOffset = frameHeight * (c * bounds[0]);

    for (int ys = 0; ys < frameHeight; ++ys) {
      detail::Uint32 *location{out + ((ys + y) * w) + x};
      detail::Uint32 *source{tex + rowOffset + (ys * bounds[0])};
      memcpy(location, source, bounds[0] * sizeof(detail::Uint32));
    }
  }

  ~texture() {
    delete[] tex;
    tex = nullptr;
    validData = false;
  }
};

class Background {
public:
  Background(const std::string &tex) : m_texture(tex), m_x(0), m_y(0) {}
  Background(const std::string &tex, int x, int y)
      : m_texture(tex), m_x(x), m_y(y) {}
  virtual ~Background() {}

  void draw(detail::RendererSurface &surface) {
    if (!m_texture.hasData())
      return;

    if (m_x > surface.GetWidth() || m_y > surface.GetHeight())
      return;

    if (m_x == 0 && m_y == 0 && m_texture.bounds[0] == surface.GetWidth() &&
        m_texture.bounds[1] == surface.GetHeight()) {
      detail::Uint32 *buffer = surface.GetPixels();

      int len =
          (m_texture.bounds[0] * m_texture.bounds[1] * sizeof(detail::Uint32));
      memcpy(buffer, m_texture.tex, len);
    } else {
      // Complex scenario - render within the bounds of the surface.
      int stride = surface.GetWidth();
      int width = m_texture.bounds[0];
      int height = m_texture.bounds[1];

      if (width > stride)
        width = stride; // sanity check.
      if (m_x + width > stride)
        width -= ((m_x + width) - stride);
      if (height > surface.GetHeight())
        height = surface.GetHeight();
      if (m_y + height > surface.GetHeight())
        height -= ((m_y + height) - surface.GetHeight());

      for (int i = 0; i < height; ++i) {
        detail::Uint32 *buffer = surface.GetPixels();
        // Shift buffer to the correct position.
        detail::Uint32 *offsetBuffer = buffer + ((m_y + i) * stride) + m_x;
        memcpy(offsetBuffer, m_texture.tex + (i * m_texture.bounds[0]),
               width * sizeof(detail::Uint32));
      }
    }
  }

  int getWidth() {
    if (!m_texture.hasData())
      return 0;

    return m_texture.bounds[0];
  }

  int getHeight() {
    if (!m_texture.hasData())
      return 0;

    return m_texture.bounds[1];
  }

protected:
  texture m_texture;
  int m_x;
  int m_y;
};

struct cursor {
  std::string::value_type m_cursor;
  util::Timer m_timer;

  cursor(std::string::value_type cursor, long rate)
      : m_cursor(cursor), m_timer(rate) {}

  std::string::value_type getCursor() {
    if (m_timer.measure()) {
      return m_cursor;
    }
    return std::string::value_type(' ');
  }
};

class Panel {
public:
  Panel(detail::RendererSurface &screen, const std::string &bg)
      : m_bg(bg), m_screen(screen), m_exiting(false) {
    Init();
  }
  Panel(detail::RendererSurface &screen, const std::string &bg, int x, int y)
      : m_bg(bg, x, y), m_screen(screen), m_exiting(false) {
    Init();
  }
  virtual ~Panel() {}

  void openDir(const std::string &path) {
    ShellExecute(0, 0, path.c_str(), 0, 0, SW_SHOW);
  }

  virtual void update() { m_bg.draw(m_screen); }
  virtual void commit() { m_screen.Flip(true); }

  bool exiting() const { return m_exiting; }

protected:
  Background m_bg;
  detail::RendererSurface &m_screen;
  bool m_exiting;

  void Init() {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  }
};

class InputPanel : public Panel, public detail::ScreenObserver {

  void openFile(const print::PrintInfo &item) {
    ShellExecute(0, 0, item.bgInfo.c_str(), 0, 0, SW_SHOW);
  }

public:
  InputPanel(detail::RendererSurface &screen, const std::string &bg)
      : Panel(screen, bg), m_cursor('_', 500), m_panelX(0), m_panelY(0) {

    screen.RegisterObserver(this);

    m_x = 0;
    m_y = 0;
    m_l = false;
    m_m = false;
    m_r = false;
  }
  InputPanel(detail::RendererSurface &screen, const std::string &bg, int x,
             int y)
      : Panel(screen, bg, x, y), m_cursor('_', 500), m_panelX(x), m_panelY(y) {

    screen.RegisterObserver(this);

    m_x = 0;
    m_y = 0;
    m_l = false;
    m_m = false;
    m_r = false;
  }
  virtual ~InputPanel() {}

  void addText(const std::string &text) {
    m_text.push_back(print::PrintInfo(text, "", false));
  }
  void addText(const std::vector<std::string> &text) {
    for (int i = 0; i < text.size(); ++i) {
      m_text.push_back(print::PrintInfo(text[i], "", false));
    }
  }
  void addText(const print::printQueue &text) {
    m_text.insert(m_text.end(), text.begin(), text.end());
  }

  void getText(print::printQueue &text) { text = m_text; }
  void clearText() { m_text.clear(); }
  const std::string &getScratch() const { return m_scratch; }

  virtual void process() {}

  virtual void HandleMouse(int x, int y, bool l, bool m, bool r) override {
    m_x = x;
    m_y = y;
    m_l = l;
    m_m = m;
    m_r = r;
  }

  virtual bool HandleDrag(int x, int y, bool) override { return false; }

  virtual void update() override {
    Panel::update();

    // One of the things we do is to check if the user is hovering or clicking
    // on some clickable text.
    int numStrings{static_cast<int>(m_text.size())};
    int screenSize = m_screen.GetHeight() - 40;
    int yPos = 20;
    if (screenSize < (numStrings * 16))
      yPos = screenSize - (numStrings * 16);

    for (auto &i : m_text) {
      if (m_screen.RenderText(i, m_panelX + 20, m_panelY + yPos, m_x, m_y, m_l,
                              m_m, m_r))
        openFile(i);
      yPos += 16;
    }

    std::vector<std::pair<WPARAM, bool>> keys;
    std::vector<std::pair<WPARAM, LPARAM>> chars;
    m_screen.GetInput(keys);
    m_screen.GetCharInput(chars);
    bool add{util::MutateString(keys, chars, 56, &m_scratch)};
    if (add && !m_scratch.empty()) {
      process();
      m_scratch = "";
    } else {
      std::string temp(m_scratch);
      temp.push_back(m_cursor.getCursor());
      m_screen.RenderText(print::PrintInfo(temp, "", false), m_panelX + 20,
                          m_panelY + yPos, m_x, m_y, m_l, m_m, m_r);
    }
  }

  void commit() { Panel::commit(); }

protected:
  print::printQueue m_text;
  // The scratchpad text used to composition the final output.
  std::string m_scratch;
  cursor m_cursor;

  // Panel location on screen.
  int m_panelX;
  int m_panelY;

  // Mouse input storage
  int m_x;
  int m_y;
  bool m_l;
  bool m_m;
  bool m_r;
};

class OutputPanel : public Panel, public detail::ScreenObserver {

  void openFile(const print::PrintInfo &item) {
    ShellExecute(0, 0, item.bgInfo.c_str(), 0, 0, SW_SHOW);
  }

public:
  OutputPanel(detail::RendererSurface &screen, const std::string &bg)
      : Panel(screen, bg), m_panelX(0), m_panelY(0) {

    screen.RegisterObserver(this);

    m_x = 0;
    m_y = 0;
    m_l = false;
    m_m = false;
    m_r = false;
  }
  OutputPanel(detail::RendererSurface &screen, const std::string &bg, int x,
              int y)
      : Panel(screen, bg, x, y), m_panelX(x), m_panelY(y) {

    screen.RegisterObserver(this);

    m_x = 0;
    m_y = 0;
    m_l = false;
    m_m = false;
    m_r = false;
  }
  virtual ~OutputPanel() {}

  void addText(const std::string &text) {
    m_text.push_back(print::PrintInfo(text, "", false));
  }
  void addText(const std::vector<std::string> &text) {
    for (int i = 0; i < text.size(); ++i) {
      m_text.push_back(print::PrintInfo(text[i], "", false));
    }
  }
  void addText(const print::printQueue &text) {
    m_text.insert(m_text.end(), text.begin(), text.end());
  }

  void getText(print::printQueue &text) { text = m_text; }
  void clearText() { m_text.clear(); }

  virtual void process() {}

  virtual void HandleMouse(int x, int y, bool l, bool m, bool r) override {
    m_x = x;
    m_y = y;
    m_l = l;
    m_m = m;
    m_r = r;
  }

  virtual bool HandleDrag(int x, int y, bool) override { return false; }

  virtual void update() override {
    Panel::update();

    // One of the things we do is to check if the user is hovering or clicking
    // on some clickable text.
    int numStrings{static_cast<int>(m_text.size())};
    int screenSize = m_bg.getHeight() - 40;
    int yPos = 20;
    if (screenSize < (numStrings * 16))
      yPos = screenSize - (numStrings * 16);

    for (auto &i : m_text) {
      if (m_screen.RenderText(i, m_panelX + 20, m_panelY + yPos, m_x, m_y, m_l,
                              m_m, m_r))
        openFile(i);
      yPos += 16;
    }
  }

  void commit() { Panel::commit(); }

protected:
  print::printQueue m_text;

  // Panel location on screen.
  int m_panelX;
  int m_panelY;

  // Mouse input storage
  int m_x;
  int m_y;
  bool m_l;
  bool m_m;
  bool m_r;
};
/*
The UI representation is a bit chaotic at the moment, but that will change very
soon - I plan to refactor the entire codebase and move things into header and
cpp files as needed.

The buttons will only take 1 image as their input, but the button will contain
states which determine which part of the image to show - each input image will
contain 3 frames.
*/
class Button : public detail::ScreenObserver {
  enum State { NORMAL = 0, HOVER, CLICK };
  State m_state;

public:
  Button(const std::string &name, bool handlesDrag = false)
      : m_textLimit(-1), m_texture(name), m_state(NORMAL),
        m_handlesDrag(handlesDrag) {}
  virtual ~Button() {}

  void setTextLimit(int chars) { m_textLimit = chars; }
  void addText(const std::string &text) { m_text.push_back(text); }
  void addText(const std::vector<std::string> &text) {
    m_text.insert(m_text.end(), text.begin(), text.end());
  }
  void getText(std::vector<std::string> &text) { text = m_text; }
  void clearText() { m_text.clear(); }

  bool handleInput(std::vector<std::pair<WPARAM, bool>> &keys) {
    if (keys.empty())
      return false;

    bool shouldReturn = false;
    for (auto &i : keys) {
      if (i.first == VK_RETURN)
        shouldReturn = true;
      else
        util::MutateString(m_text, i, m_textLimit);
    }

    return (shouldReturn && !m_text.empty());
  }

  // Returns true if this button has been clicked.
  bool wasClicked() { return (m_state == CLICK); }

  virtual void HandleMouse(int x, int y, bool l, bool m, bool r) override {
    // If left button is pressed, check if it's within bounds.
    if (x >= m_x && x <= m_x + m_texture.bounds[0]) {
      if (y >= m_y && y <= m_y + m_texture.frameHeight) {
        if (!l) {
          m_state = HOVER;
          return;
        }

        m_state = CLICK;
        return;
      }
    }

    m_state = NORMAL;
  }

  virtual bool HandleDrag(int x, int y, bool l) override {
    if (!m_handlesDrag)
      return false;

    if (x >= m_x && x <= m_x + m_texture.bounds[0]) {
      if (y >= m_y && y <= m_y + m_texture.frameHeight) {
        m_state = (l ? CLICK : HOVER);
        return true;
      }
    }

    return false;
  }

  void draw(detail::RendererSurface &surface, int x, int y) {
    m_x = x;
    m_y = y;
    // Button is always rendered at original size.
    detail::Uint32 *buffer = surface.GetPixels();
    int sw = surface.GetWidth();

    m_texture.copyFrame(buffer, sw, m_x, m_y, static_cast<int>(m_state), 3);

    if (m_text.empty())
      return;

    int yPos = y + 20;
    for (auto &i : m_text) {
      surface.RenderText(print::PrintInfo(i, "", false), x + 20, yPos, -1, -1,
                         false, false, false);
      yPos += 20;
    }
  }

  int getX() const { return m_x; }
  int getY() const { return m_y; }
  int getW() const { return m_texture.bounds[0]; }
  int getH() const { return m_texture.frameHeight; }

protected:
  int m_x;
  int m_y;
  int m_textLimit;
  texture m_texture;
  std::vector<std::string> m_text;
  bool m_handlesDrag;
};

} // namespace ui

#endif // UI