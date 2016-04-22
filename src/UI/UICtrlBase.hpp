#ifndef VARCO_UICTRLBASE_HPP
#define VARCO_UICTRLBASE_HPP

#include <SkBitmap.h>

namespace varco {

  // Every container of controls should implement redraw() to be called by one of its subcontrols
  class UIContainer {
  public:
    virtual ~UIContainer() = default;
    virtual void redraw() {}
    virtual void startMouseCapture() {}
  };



  // Every Varco UI control derives from this class which holds common data
  // and methods for painting and resizing purposes
  class UICtrlBase {
  protected:
    UICtrlBase(UIContainer& parentContainer); // Should not be instantiated directly
    virtual ~UICtrlBase() = default;

    UIContainer& m_parentContainer;
    SkRect m_rect; // Rect where to draw the control, relative to the client area of the parent
    SkBitmap m_bitmap; // The entire control will be rendered here
    bool m_dirty = true;

  public:

    virtual void resize(SkRect rect);
    virtual void paint() = 0;
    
    SkBitmap& getBitmap();

    enum RectType {relativeToParentRect, absoluteRect};
    SkRect getRect(RectType type = relativeToParentRect);
  };

}

#endif // VARCO_UICTRLBASE_HPP
