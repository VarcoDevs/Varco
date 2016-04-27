#ifndef VARCO_UTILS_HPP
#define VARCO_UTILS_HPP

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

  template<class T>
  constexpr const T& clamp(const T& val, const T& lo, const T& hi) {    
    return (val < lo) ? lo : (hi < val) ? hi : val;
  }

}

#endif // VARCO_UTILS_HPP
