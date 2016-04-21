#ifndef VARCO_UICTRLBASE_HPP
#define VARCO_UICTRLBASE_HPP

#include <SkBitmap.h>

namespace varco {

  class MainWindow;

  // Every Varco UI control derives from this class which holds common data
  // and methods for painting and resizing purposes
  class UICtrlBase {
  protected:
    UICtrlBase(MainWindow& parentWindow); // Should not be instantiated directly

    MainWindow& parentWindow;
    SkRect rect; // Rect where to draw the control, relative to the client area of the parent
    SkBitmap bitmap; // The entire control will be rendered here
    bool dirty = true;

  public:

    virtual void resize(SkRect rect);
    virtual void paint() = 0;
    
    SkBitmap& getBitmap();
    SkRect getRect();
  };

}

#endif // VARCO_UICTRLBASE_HPP
