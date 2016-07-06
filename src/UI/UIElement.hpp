#ifndef VARCO_UIELEMENT_HPP
#define VARCO_UIELEMENT_HPP

#include <SkCanvas.h>
#include <SkBitmap.h>

namespace varco {

  // Tag dispatching
  struct ui_container_tag {}; // This UI element is tagged to contain other UI elements
  struct ui_control_tag {}; // This UI element is a UI control

  // Every Varco UI control derives from this class which holds common data
  // and methods depending on the UI control type
  template<typename... Tag>
  struct UIElement {};

  template<>
  class UIElement<ui_container_tag> {
  public:
    using tag_container = ui_container_tag;

    virtual ~UIElement() = default;

    virtual void repaint() = 0; // Might be requested by child controls (schedule or performs a repaint)
    virtual void startMouseCapture() {} // Might be requested by child controls
    virtual void stopMouseCapture() {} // Might be requested by child controls
  };

  template<>
  class UIElement<ui_control_tag> {
  protected:

    explicit UIElement(UIElement<ui_container_tag>& parentContainer) : // Should not be instantiated directly
      m_parentContainer(parentContainer) {}

    virtual ~UIElement() = default;

    UIElement<ui_container_tag>& m_parentContainer;
    SkRect m_rect; // Rect where to draw the control, relative to the client area of the parent
    SkBitmap m_bitmap; // The entire control will be rendered here
    bool m_dirty = true;

  public:

    using tag_control = ui_container_tag;

    virtual void resize(SkRect rect /* relative to parent */) {
//      if (m_rect != rect && rect.fTop < rect.fBottom && rect.fLeft < rect.fRight) {
//        m_rect = rect;

//        // This adjustment is necessary since the control needs not to know anything about its
//        // relative position on its parent. It always starts drawing its bitmap at top-left 0;0
//        m_bitmap.allocPixels(SkImageInfo::Make((int)(m_rect.fRight - m_rect.fLeft),
//          (int)(m_rect.fBottom - m_rect.fTop), kN32_SkColorType, kPremul_SkAlphaType));
//        m_dirty = true;
//      }
      m_rect = rect;
    }

    virtual void paint(SkCanvas& canvas) = 0;

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

  template<typename... Tag>
  struct UIElement <ui_container_tag, Tag...> : public UIElement<ui_container_tag>, public UIElement<Tag...> {

    template<typename... Args>
    explicit UIElement(Args&& ...args) : UIElement<Tag...>(std::forward<Args>(args)...) {}
    
  };

  template<typename... Tag>
  struct UIElement <ui_control_tag, Tag...> : public UIElement<ui_control_tag>, public UIElement<Tag...> {

    template<typename... Args>
    explicit UIElement(UIElement<ui_container_tag>& arg, Args&& ...args) :
      UIElement<ui_control_tag>(arg), UIElement<Tag...>(std::forward<Args>(args)...) {}

  };
}

#endif // VARCO_UIELEMENT_HPP
