#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkPath.h>
#include <string>
#include <vector>

namespace varco {
  class Tab {
  public:
    Tab(std::string title);

    // TODO: resize

    // Paint the tab into its own bitmap
    void paint();
    SkBitmap& getBitmap();
    SkRect getRect();
    SkPath getPath();
    void setSelected(bool);
    void setOffset(SkScalar);
    SkScalar getOffset();

  private:
    std::string title;
    SkBitmap bitmap; // The tab will be rendered here
    SkRect rect; // The rect that encloses the tab (coords relative to the tab element)
    SkPath path; // The path where clicks and inputs are accepted
    SkScalar parentOffset; // The offset from the start of the parent tab control
    bool selected = false;
  };

  class TabCtrl {
  public:
    TabCtrl();

    // Set the new rect where the control will redraw itself
    void setRect(SkRect rect);
    SkRect getRect();
    // Paint the control on the given canvas
    void paint(SkCanvas *canvas);
    bool onMouseClick(SkScalar x, SkScalar y); // Returns true if a redraw is needed

  private:
    SkRect rect; // Rect enclosing the control, relative to the canvas passed to paint()
    SkCanvas *lastCanvas = nullptr; // Last canvas we were drawn to
    std::vector<Tab> tabs;
    size_t selectedTab = -1; // The index of the selected tab
  };
}

#endif // VARCO_TABCTRL_HPP
