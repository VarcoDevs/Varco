#include <UI/UICtrlBase.hpp>
#include <SkRect.h>

namespace varco {

  UICtrlBase::UICtrlBase(UIContainer& parentContainer) :
    m_parentContainer(parentContainer)
  {}

  void UICtrlBase::resize(SkRect rect) {
    if (m_rect != rect && rect.fTop < rect.fBottom && rect.fLeft < rect.fRight) {
      m_rect = rect;

      // This adjustment is necessary since the control needs not to know anything about its
      // relative position on its parent. It always starts drawing its bitmap at top-left 0;0
      m_bitmap.allocPixels(SkImageInfo::Make((int)(m_rect.fRight - m_rect.fLeft), 
                           (int)(m_rect.fBottom - m_rect.fTop), kN32_SkColorType, kPremul_SkAlphaType));
      m_dirty = true;
    }
  }

  SkBitmap& UICtrlBase::getBitmap() {
    return m_bitmap;
  }

  SkRect UICtrlBase::getRect(RectType type) {
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

}