#ifndef VARCO_UTILS_HPP
#define VARCO_UTILS_HPP

#include <SkSurface.h>

namespace varco {
  
  template<typename T>
  auto isPointInsideRect(T x, T y, SkRect rect) {
    SkScalar x_ = static_cast<SkScalar>(x);
    SkScalar y_ = static_cast<SkScalar>(y);
    if (rect.fLeft <= x_ && rect.fTop <= y_ && rect.fRight >= x_ && rect.fBottom >= y_)
      return true;
    else
      return false;
  };

  // Create a surface from a bitmap
  inline SkSurface* createSurfaceFromBitmap(SkBitmap& bitmap) {
    const SkSurfaceProps fSurfaceProps = SkSurfaceProps::kLegacyFontHost_InitType;
    return SkSurface::NewRasterDirect(bitmap.info(), bitmap.getPixels(), bitmap.rowBytes(), &fSurfaceProps);
  }

}

#endif // VARCO_UTILS_HPP
