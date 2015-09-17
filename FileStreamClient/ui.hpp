// Copyright (c) - 2015, Shaheed Abdol.
// Use this code as you wish, but don't blame me.

#ifndef UI
#define UI

#include "Renderer.hpp"
#include "util.hpp"

namespace ui {

struct texture {
  detail::Uint32 *tex;
  int bounds[2];

  texture(int w, int h) : tex(nullptr) {
    bounds[0] = w;
    bounds[1] = h;
    tex = new detail::Uint32[bounds[0] * bounds[1]];
  }

  texture(const texture &other) {
    bounds[0] = other.bounds[0];
    bounds[1] = other.bounds[1];
    tex = new detail::Uint32[bounds[0] * bounds[1]];
    memcpy(tex, other.tex, (bounds[0] * bounds[1]) * sizeof(detail::Uint32));
  }

  texture(const std::string &name) {
    // Load a texture resource from a file with the given name. First tries
    // searching in whichever folder is the current working directory, if that
    // fails it will try searching in the module folder (path to .exe file)

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
  }

  void clear() {
    int len(bounds[0] * bounds[1]);
    memset(tex, 0, len * sizeof(detail::Uint32));
  }

  ~texture() {
    delete[] tex;
    tex = nullptr;
  }
};

class Panel {
public:
  Panel(detail::RendererSurface &screen) : m_screen(screen), m_exiting(false) {}
  virtual ~Panel() {}

  virtual void update() { m_screen.Flip(true); }

  bool exiting() const { return m_exiting; }

protected:
  detail::RendererSurface &m_screen;
  bool m_exiting;
};

class Background {
public:
  Background(const std::string &tex) : m_texture(tex) {}
  virtual ~Background() {}

  void draw(detail::RendererSurface &surface) {
    // A background must always have the same resolution as a panel.
    detail::Uint32 *buffer = surface.GetPixels();

    int len =
        (m_texture.bounds[0] * m_texture.bounds[1] * sizeof(detail::Uint32));
    memcpy(buffer, m_texture.tex, len);
  }

protected:
  texture m_texture;
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

class InputPanel : public Panel {
public:
  InputPanel(detail::RendererSurface &screen, const std::string &bg)
      : Panel(screen), m_bg(bg), m_cursor('_', 500) {}
  virtual ~InputPanel() {}

  void addText(const std::string &text) { m_text.push_back(text); }
  void addText(const std::vector<std::string> &text) {
    m_text.insert(m_text.end(), text.begin(), text.end());
  }
  void getText(std::vector<std::string> &text) { text = m_text; }
  void clearText() { m_text.clear(); }
  const std::string &getScratch() const { return m_scratch; }

  virtual void process() {}

  void update() override {
    m_bg.draw(m_screen);

    int numStrings{static_cast<int>(m_text.size())};
    int screenSize = m_screen.GetHeight() - 40;
    int yPos = 20;
    if (screenSize < (numStrings * 16))
      yPos = screenSize - (numStrings * 16);

    for (auto &i : m_text) {
      m_screen.RenderText(i, 20, yPos);
      yPos += 16;
    }

    std::vector<std::pair<WPARAM, bool>> keys;
    std::vector<std::pair<WPARAM, LPARAM>> chars;
    m_screen.GetInput(keys);
    m_screen.GetCharInput(chars);
    bool add{util::MutateString(keys, chars, 60, &m_scratch)};
    if (add && !m_scratch.empty()) {
      process();
      m_scratch = "";
    } else {
      std::string temp(m_scratch);
      temp.push_back(m_cursor.getCursor());
      m_screen.RenderText(temp, 20, yPos);
    }
  }

  void commit() { Panel::update(); }

protected:
  Background m_bg;
  std::vector<std::string> m_text;
  // The scratchpad text used to composition the final output.
  std::string m_scratch;
  cursor m_cursor;
};

class Button {
public:
  Button(const std::string &name) : m_textLimit(-1), m_texture(name) {}
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
  bool handleMouseInput(int x, int y, bool l, bool m, bool r) {
    // Buttons (for now) only deal with left mouse button input.
    if (!l)
      return false;

    // If left button is pressed, check if it's within bounds.
    if (x >= m_x && x <= m_x + m_texture.bounds[0]) {
      if (y >= m_y && y <= m_y + m_texture.bounds[1]) {
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
    for (int ys = 0; ys < m_texture.bounds[1]; ++ys) {
      detail::Uint32 *location{buffer + ((ys + y) * sw) + x};
      detail::Uint32 *source{m_texture.tex + (ys * m_texture.bounds[0])};
      memcpy(location, source, m_texture.bounds[0] * sizeof(detail::Uint32));
    }

    if (m_text.empty())
      return;

    int yPos = y + 20;
    for (auto &i : m_text) {
      surface.RenderText(i, x + 20, yPos);
      yPos += 20;
    }
  }

protected:
  int m_x;
  int m_y;
  int m_textLimit;
  texture m_texture;
  std::vector<std::string> m_text;
};

} // namespace ui

#endif // UI