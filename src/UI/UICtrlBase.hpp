#ifndef VARCO_UICTRLBASE_HPP
#define VARCO_UICTRLBASE_HPP

#include <SkBitmap.h>

namespace varco {


  // Every container of others UI controls should implement redraw() to be called by one of its subcontrols.
  // Other events are optionally overridden.
  class UIContainer {
  public:
    virtual ~UIContainer() = default;
    virtual void redraw() = 0;
    virtual void startMouseCapture() {}
  };


  // Tag dispatching
  struct ui_container_tag {}; // This UI element is tagged to contain other UI elements
  struct ui_control_tag {}; // This UI element is a UI control

  // Every Varco UI control derives from this class which holds common data
  // and methods depending on the UI control type
  template<typename... Tag>
  struct UIElement {};

  template<typename... Tag>
  class UIElement <ui_container_tag, Tag...> : public UIElement<Tag...> {
  public:
    using tag_container = ui_container_tag;

    virtual ~UIElement() = default;

    virtual void repaint() {} // Might be requested by child controls (schedule or performs a repaint)
    virtual void startMouseCapture() {} // Might be requested by child controls
  };

  template<typename... Tag>
  class UIElement <ui_control_tag, Tag...> : public UIElement<Tag...> {
  protected:

    UIElement(UIElement<ui_container_tag>& parentContainer) : // Should not be instantiated directly
      m_parentContainer(parentContainer) {}

    virtual ~UIElement() = default;

    UIElement<ui_container_tag>& m_parentContainer;
    SkRect m_rect; // Rect where to draw the control, relative to the client area of the parent
    SkBitmap m_bitmap; // The entire control will be rendered here
    bool m_dirty = true;

  public:

    virtual void resize(SkRect rect) {
      if (m_rect != rect && rect.fTop < rect.fBottom && rect.fLeft < rect.fRight) {
        m_rect = rect;

        // This adjustment is necessary since the control needs not to know anything about its
        // relative position on its parent. It always starts drawing its bitmap at top-left 0;0
        m_bitmap.allocPixels(SkImageInfo::Make((int)(m_rect.fRight - m_rect.fLeft),
          (int)(m_rect.fBottom - m_rect.fTop), kN32_SkColorType, kPremul_SkAlphaType));
        m_dirty = true;
      }
    }

    virtual void paint() = 0;    

    SkBitmap& getBitmap() {
      return m_bitmap;
    }

    enum RectType { relativeToParentRect, absoluteRect };
    SkRect getRect(RectType type = relativeToParentRect) {
      switch (type) {
      case relativeToParentRect: {
        return m_rect;
      } break;
      case absoluteRect: {
        return SkRect::MakeLTRB(0, 0, (SkScalar)m_bitmap.width(), (SkScalar)m_bitmap.height());
      } break;
      }
      return m_rect;
    }
  };

  

}

#endif // VARCO_UICTRLBASE_HPP
