#include <UI/UICtrlBase.hpp>

namespace varco {

  UICtrlBase::UICtrlBase(MainWindow& parentWindow) :
    parentWindow(parentWindow)
  {}

  void UICtrlBase::resize(SkRect rect) {
    if (this->rect != rect) {
      this->rect = rect;

      // This adjustment is necessary since the control needs not to know anything about its
      // relative position on its parent. It always starts drawing its bitmap at top-left 0;0
      this->rect.fRight -= this->rect.fLeft;
      this->rect.fBottom -= this->rect.fTop;
      this->rect.fLeft = 0;
      this->rect.fTop = 0;

      this->bitmap.allocPixels(SkImageInfo::Make((int)rect.width(), (int)rect.height(), 
                                                 kN32_SkColorType, kPremul_SkAlphaType));
      this->dirty = true;
    }
  }

  SkBitmap& UICtrlBase::getBitmap() {
    return this->bitmap;
  }

  SkRect UICtrlBase::getRect() {
    return this->rect;
  }






}