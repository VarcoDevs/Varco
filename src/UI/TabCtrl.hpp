#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include <SkCanvas.h>
#include <SkBitmap.h>
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

  private:
    std::string title;
    SkBitmap bitmap; // The tab will be rendered here
    SkRect rect;
  };

  class TabCtrl {
  public:
    TabCtrl();

    // Set the new rect where the control will redraw itself
    void setRect(SkRect rect);
    // Paint the control on the given canvas
    void paint(SkCanvas *canvas);

  private:
    SkRect rect;
    std::vector<Tab> tabs;
  };
}

#endif // VARCO_TABCTRL_HPP
