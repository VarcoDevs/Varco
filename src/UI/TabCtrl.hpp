#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkPath.h>
#include <string>
#include <vector>
#include <memory>

namespace varco {

  class TabCtrl;

  class Tab { // Represents a tab inside the tab control
  public:
    Tab(TabCtrl& parent, std::string title);

    // Paint the tab into its own bitmap
    void paint();
    SkBitmap& getBitmap();
    SkPath& getPath();
    void setSelected(bool);
    void setOffset(SkScalar);
    SkScalar getOffset();

  private:
    TabCtrl& parent;
    std::string title;
    SkBitmap bitmap; // The tab will be rendered here
    bool dirty = true;
    SkPath path; // The path where clicks and inputs are accepted
    SkScalar parentOffset; // The offset from the start of the parent tab control
    bool selected = false; // Is this the selected tab?
  };

  class TabCtrl { // The main tab control
    friend class Tab;
  public:
    TabCtrl();

    // Set the new rect where the control will redraw itself
    void resize(SkRect rect);
    SkBitmap& getBitmap();
    SkRect getRect();
    void paint(); // Paint the control in the bitmap
    bool onMouseClick(SkScalar x, SkScalar y); // Returns true if a redraw is needed

  private:
    SkRect rect; // Rect where to draw the control, relative to the client area of the parent
    SkBitmap bitmap; // The entire control will be rendered here
    bool dirty = true;
    std::vector<Tab> tabs;
    size_t selectedTab = -1; // The index of the selected tab
  };
}

#endif // VARCO_TABCTRL_HPP
