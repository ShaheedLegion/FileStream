#pragma once
#ifndef UI_LIB_DETAIL
#define UI_LIB_DETAIL

namespace detail {
typedef unsigned int Uint32;

#pragma pack(1)
struct RGBA {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

struct RenderBuffer {
  int width;
  int height;
  Uint32 *bits;

  RenderBuffer(int w, int h, Uint32 *bits) : width(w), height(h), bits(bits) {}
  RenderBuffer(const RenderBuffer &other)
      : width(other.width), height(other.height), bits(other.bits) {}

  void render(RenderBuffer &target, int x, int y, int hFrames, int vFrames,
              int cFrame) {
    if (target.bits == nullptr || bits == nullptr)
      return;

    if (x == -1 || y == -1)
      return;

    if (width == 0 || height == 0 || target.width == 0 || target.height == 0)
      return;

    if (x > target.width || y > target.height)
      return;

    bool staticImg = (hFrames == 1 && vFrames == 1);

    if (staticImg && x == 0 && y == 0 && width == target.width &&
        height == target.height) {

      int len = (width * height);
      RGBA *alias = reinterpret_cast<RGBA *>(bits);
      detail::Uint32 *aliasBits = target.bits;

      while (len > 0) {
        // Perform the blending - we don't support true alpha yet.
        if (alias->a != 0)
          *aliasBits = *(reinterpret_cast<detail::Uint32 *>(alias));

        ++aliasBits;
        ++alias;
        --len;
      }
    } else {
      // Complex scenario - render within the bounds of the surface.
      int stride = target.width;
      int nwidth = width / hFrames;
      int nheight = height / vFrames;

      if (nwidth > stride)
        nwidth = stride; // sanity check.
      if (x + nwidth > stride)
        nwidth -= ((x + width) - stride);
      if (nheight > target.height)
        nheight = target.height;
      if (y + nheight > target.height)
        nheight -= ((y + nheight) - target.height);

      int offsetFrame = nheight * cFrame;

      for (int i = 0; i < nheight; ++i) {
        // Shift buffer to the correct position.
        detail::Uint32 *offsetBuffer = target.bits + ((y + i) * stride) + x;

		detail::RGBA *thisBuffer = reinterpret_cast<detail::RGBA*>(bits + ((i) * width));
		for (int tx = 0; tx < nwidth; ++tx) {
			if (thisBuffer->a != 0) {
				*offsetBuffer = *(reinterpret_cast<detail::Uint32 *>(thisBuffer));
			}
			++offsetBuffer;
			++thisBuffer;
		}

        //memcpy(offsetBuffer, bits + ((i + offsetFrame) * width),
        //       nwidth * sizeof(detail::Uint32));
      }
    }
  }
};

// Every renderable has a position/dimensions.
class IRenderable {
  int x;
  int y;
  int w;
  int h;
  bool visible;
  int hFrames;
  int vFrames;
  int cFrame;

public:
  IRenderable()
      : x(-1), y(-1), w(0), h(0), visible(false), hFrames(1), vFrames(1),
        cFrame(0) {}
  virtual ~IRenderable() {}

  bool isVisible() const { return visible; }
  virtual int getW() const { return w; }
  virtual int getH() const { return h; }
  int getX() const { return x; }
  int getY() const { return y; }
  int getHFrames() const { return hFrames; }
  int getVFrames() const { return vFrames; }
  int getCurrentFrame() const { return cFrame; }

  void setVisible(bool visible) { this->visible = visible; }
  virtual void setW(int w) { this->w = w; }
  virtual void setH(int h) { this->h = h; }
  virtual void setX(int x) { this->x = x; }
  virtual void setY(int y) { this->y = y; }
  void setHFrames(int frames) { hFrames = frames; }
  void setVFrames(int frames) { vFrames = frames; }
  void setCurrentFrame(int frame) { cFrame = frame; }

  // No default implementation since we don't have data here.
  virtual void render(RenderBuffer &buffer) = 0;
}; // IRenderable

class Interactive {
public:
  virtual ~Interactive() {}

  virtual void HandleKey(int keyCode, bool pressed) = 0;

  // We can accomplish all other events with this one - drag, move, capture.
  virtual void HandleMouse(int mx, int my, bool l, bool m, bool r) = 0;
}; // Interactive

class Texture : public IRenderable {
  RenderBuffer data;

public:
  const RenderBuffer &getData() const { return data; }

  Texture(int w, int h, Uint32 *bits) : data(w, h, bits) {
    setW(w);
    setH(h);
  }

  Texture(int color, int w, int h, Uint32 *bits) : data(w, h, bits) {
    setW(w);
    setH(h);

    if (data.bits == nullptr) {
      // Ok, it's null, so we push the single color pixel into it.
      data.bits = new Uint32[w * h];
      memset(data.bits, color, w * h * sizeof(Uint32));
    }
  }

  Texture(const Texture &other) : data(other.getData()) {
    setW(data.width);
    setH(data.height);
  }

  Uint32 *getBuffer() const { return data.bits; }

  virtual void render(RenderBuffer &target) override {
    data.render(target, getX(), getY(), getHFrames(), getVFrames(),
                getCurrentFrame());
  }
};

} // namespace detail

#endif // UI_LIB_DETAIL