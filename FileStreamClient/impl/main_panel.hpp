#pragma once
#ifndef _MAIN_PANEL_HPP
#define _MAIN_PANEL_HPP

#include "../ui/button.h"
#include "../ui/panel.h"
#include "../ui/text.h"
#include "../ui/texture_loader.h"
#include "rendition_panel.hpp"
#include "chat_input_panel.hpp"
#include "command_panel.hpp"
#include "../xml/src/pugixml.hpp"
#include <map>
#include <functional>

namespace impl {

class MainPanel : public ui::Control {
  // Lets create a callback type

  typedef std::map<std::string, ui::Control *> ControlsType;
  ControlsType panels;

  typedef std::map<std::string, std::function<Control *()>> ControlFactory;
  ControlFactory factory;

  static ui::Panel *createPanel() { return new ui::Panel(); }
  static ui::Text *createText() { return new ui::Text(); }

  static void RegisterControls(ControlFactory &factory) {
    // We can immediately add the panels to the map here.
    // We do this by name instead of by type so that we can identify and
    // easily find specific controls.
    // panels["user_name"] = new ui::Input();
    // panels["chat_text"] = new ui::Input();
    factory["panel"] = std::bind(createPanel);
    factory["input"] = std::bind(createText);
  }

  static ui::Control *CreateControl(const std::string &name,
                                    ControlFactory &factory) {
    auto it = factory.find(name);
    if (it == factory.end())
      return new ui::Control();

    return ((*it).second)();
  }

  static void CreateDom(Control *parent, pugi::xml_node &node,
                        ControlsType &panels, ControlFactory &factory) {
    std::string nodeName(node.attribute("name").value());

    if (panels.find(nodeName) == panels.end())
      panels[nodeName] = CreateControl(node.name(), factory);

    for (pugi::xml_attribute attr = node.first_attribute(); attr;
         attr = attr.next_attribute())
      panels[nodeName]->SetAttribute(attr.name(), attr.value());

    parent->AddChild(panels[nodeName]);

    for (pugi::xml_node child = node.first_child(); child;
         child = child.next_sibling())
      CreateDom(panels[nodeName], child, panels, factory);
  }

  static void PositionControls(Control *parent, int x, int y, int w, int h) {
    // Here we apply the layout rules. We assume the parent has been positioned.
    int startX = x;
    int startY = y;
    int pwidth = w;
    int pheight = h;

    switch (parent->getOrientation()) {
    case ui::HORIZONTAL:
    case ui::VERTICAL: {
      // Start laying out from top to bottom.
      for (auto i : parent->getChildren()) {

        ui::DimensionInfo &width(i->getWidth());
        ui::DimensionInfo &height(i->getHeight());
        int controlWidth = width.getValue(pwidth);
        int controlHeight = height.getValue(pheight);

        // Now place the control there.
        int controlX = startX;
        int controlY = startY;

        if (i->getHAlign() == ui::LEFT) {
          // Do nothing because it's already left aligned.
        } else if (i->getHAlign() == ui::CENTER) {
          // Align the control in the center of the remaining space.
          controlX = startX + (((pwidth - startX) / 2) - (controlWidth / 2));
        } else if (i->getHAlign() == ui::RIGHT) {
          // Align the control to the right in the remaining space.
          controlX = pwidth - (controlWidth);
        }

        if (i->getVAlign() == ui::TOP) {
          // Do nothing since it's already top aligned.
        } else if (i->getVAlign() == ui::MIDDLE) {
          // Align the control in the middle of the remaining space.
          controlY = startY + (((pheight) / 2) - (controlHeight / 2));
        } else if (i->getVAlign() == ui::BOTTOM) {
          // Align the control to the bottom of the remaining space.
          controlY = pheight - (controlHeight);
        }

        i->setX(controlX);
        i->setY(controlY);
        i->setW(controlWidth);
        i->setH(controlHeight);

        startX = controlX + controlWidth;
        startY = controlY + controlHeight;
        pwidth -= controlWidth;
        pheight -= controlHeight;
      }
    } break;
    case ui::STACK: {
      // Do something special here.
    } break;
    }

    for (auto i : parent->getChildren()) {
      PositionControls(i, i->getX(), i->getY(), i->getW(), i->getH());
    }
  }

public:
  static MainPanel &ConstructUI(pugi::xml_document &uiTree) {
    // Parse the document tree to get at the panel
    // We know the structure of this thing, so we can juse use it.
    static MainPanel *impl{nullptr};
    if (impl == nullptr)
      impl = new MainPanel(uiTree.first_child());

    return *impl;
  }

  virtual ~MainPanel() {}

  void update(detail::Uint32 *bits, int w, int h) {
    PositionControls(this, getX(), getY(), w, h);

    detail::RenderBuffer buffer(w, h, bits);
    render(buffer);
  }

private:
  MainPanel(pugi::xml_node &node) : ui::Control() {
    RegisterControls(factory);
    for (pugi::xml_attribute attr = node.first_attribute(); attr;
         attr = attr.next_attribute())
      SetAttribute(attr.name(), attr.value());

    // So at this point we need something that will iterate through the tree and
    // add the child controls .... this should be interesting.
    for (pugi::xml_node child = node.first_child(); child;
         child = child.next_sibling())
      CreateDom(this, child, panels, factory);

    setX(0);
    setY(0);
  }
};

} // namespace impl

#endif // _MAIN_PANEL_HPP