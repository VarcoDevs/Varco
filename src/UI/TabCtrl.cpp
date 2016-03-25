#include <UI/TabCtrl.hpp>
#include <Utils/Utils.hpp>
#include <SkTypeface.h>
#include <SkGradientShader.h>

namespace varco {

  static SkRect tabDefaultRect = SkRect::MakeLTRB(0, 0, 140.0, 33.0); // The rect that encloses the tab (coords relative to the tab element)

  Tab::Tab(TabCtrl& parent, std::string title) :
    parent(parent),
    title(title)
  {
    bitmap.allocPixels(SkImageInfo::Make((int)tabDefaultRect.width(), (int)tabDefaultRect.height(), kN32_SkColorType, kPremul_SkAlphaType));
  }

  SkBitmap& Tab::getBitmap() {
    return this->bitmap;
  }

  SkPath& Tab::getPath() {
    return this->path;
  }

  void Tab::setSelected(bool selected) {
    this->selected = selected;
    dirty = true;
    parent.dirty = true;
  }

  void Tab::setOffset(SkScalar offset) {
    this->parentOffset = offset;
  }

  SkScalar Tab::getOffset() {
    return this->parentOffset;
  }

  void Tab::paint() {

    if (!dirty)
      return;

    SkCanvas canvas(this->bitmap);

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the tab (alpha only)
    //////////////////////////////////////////////////////////////////////
    canvas.clear(0x00000000); // ARGB

    //////////////////////////////////////////////////////////////////////
    // Draw the tab
    //////////////////////////////////////////////////////////////////////

    // ---> x
    // |y
    // v
    //             tabBezierWidth
    //           <->
    //    ---------
    //   /         \
    //  /           \
    //  <--tabWidth-->
    SkScalar tabBezierWidth = 18;

    SkPaint tabBorderPaint;
    tabBorderPaint.setStyle(SkPaint::kFill_Style);
    if (this->selected == false)
      tabBorderPaint.setColor(SkColorSetARGB(255, 40, 40, 40));
    else
      tabBorderPaint.setColor(SkColorSetARGB(255, 255, 60, 60));
    SkRect tabRect = SkRect::MakeLTRB(0, 5 /* top padding */, tabDefaultRect.width(), tabDefaultRect.fBottom);
    SkScalar yDistanceBetweenBezierPoints = (tabRect.fBottom - tabRect.fTop) / 4.0f;

    path.reset();
    //    _ P2
    // C2/
    //   \_
    //     \ C1
    //     /
    //    - P0
    path.moveTo(tabRect.fLeft, tabRect.fBottom); // P0      
    path.cubicTo(
      tabRect.fLeft + (0.60f * tabBezierWidth), tabRect.fBottom, // C1
      tabRect.fLeft + (0.64f * tabBezierWidth), tabRect.fTop, // C2
      tabRect.fLeft + tabBezierWidth, tabRect.fTop // P2
      );
    path.lineTo(tabRect.fRight - tabBezierWidth, tabRect.fTop); // P2
    path.cubicTo(
      tabRect.fRight - tabBezierWidth + ((1.0f - 0.64f) * tabBezierWidth), tabRect.fTop, // C2
      tabRect.fRight - tabBezierWidth + ((1.0f - 0.60f) * tabBezierWidth), tabRect.fBottom, // C1
      tabRect.fRight, tabRect.fBottom // P0
      );
    canvas.drawPath(path, tabBorderPaint); // Fill

    tabBorderPaint.setStyle(SkPaint::kStroke_Style);
    tabBorderPaint.setStrokeWidth(1);
    tabBorderPaint.setAntiAlias(true);
    tabBorderPaint.setStrokeCap(SkPaint::kRound_Cap);
    tabBorderPaint.setColor(SkColorSetARGB(255, 70, 70, 70));
    canvas.drawPath(path, tabBorderPaint); // Stroke

    SkTypeface *arialTypeface = SkTypeface::CreateFromName("Arial", SkTypeface::kNormal); // Or closest match
    SkPaint tabTextPaint;
    tabTextPaint.setColor(SK_ColorWHITE);
    tabTextPaint.setAlpha(255);
    tabTextPaint.setTextSize(SkIntToScalar(11));
    tabTextPaint.setAntiAlias(true);
    tabTextPaint.setLCDRenderText(true);
    tabTextPaint.setTypeface(arialTypeface);

    SkRect textRect = SkRect::MakeLTRB(tabRect.fLeft + 20, tabRect.fTop + 5, tabRect.fRight - 15, tabRect.fBottom - 5);
    canvas.save();
    canvas.clipRect(textRect);
    SkPoint points[2] = {
      SkPoint::Make(textRect.right() - 15, textRect.top()),
      SkPoint::Make(textRect.right(), textRect.top())
    };
    SkColor colors[2] = { 0xFFFFFFFF, 0x0}; // Opaque white to transparent
    auto shader = SkGradientShader::MakeLinear(points, colors, NULL, 2, SkShader::kClamp_TileMode, 0, NULL);
    tabTextPaint.setShader(shader);
    canvas.drawText(title.data(), title.size(), tabRect.fLeft + 20, tabRect.fBottom - 10, tabTextPaint);

    canvas.flush();
    dirty = false;

    canvas.restore();
  }

  TabCtrl::TabCtrl() {
    // DEBUG - add some tabs
    tabs.emplace_back(*this, "ALOTOFTEXTALOTOFTEXT");
    tabs.emplace_back(*this, "Second tab");
    tabs.emplace_back(*this, "Third tab");
  }
  
  void TabCtrl::resize(SkRect rect) {
    if (this->rect != rect) {
      this->rect = rect;
      this->bitmap.allocPixels(SkImageInfo::Make((int)rect.width(), (int)rect.height(), kN32_SkColorType, kPremul_SkAlphaType));
      dirty = true;
    }
  }

  SkBitmap& TabCtrl::getBitmap() {
    return this->bitmap;
  }

  SkRect TabCtrl::getRect() {
    return this->rect;
  }

  void TabCtrl::paint() {

    if (!dirty)
      return;

    SkCanvas canvas(this->bitmap);

    canvas.save();

    // The following is no longer necessary due to drawing into different bitmap buffers
    //canvas->clipRect(this->rect); // Clip to control rect, this helps preventing overflowing
                                    // drawings when using antialiasing and other hard-to-control
                                    // drawing techniques

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    SkPaint background;
    background.setColor(SkColorSetARGB(255, 20, 20, 20));
    canvas.drawRect(this->rect, background);

    SkScalar tabOffset = 0.0f;
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];
      tab.setOffset(tabOffset);
      tab.paint(); // Render tab
      canvas.drawBitmap(tab.getBitmap(), this->rect.fLeft + tabOffset, this->rect.fTop);
      tabOffset += tabDefaultRect.width() - 20.0f;      
    }

    canvas.flush();
    dirty = false;

    canvas.restore();
  }

  bool TabCtrl::onMouseClick(SkScalar x, SkScalar y) {
    
    SkPoint relativeToTabCtrl = SkPoint::Make(x - this->rect.fLeft, y - this->rect.fTop);
    
    // Detect if the click was on a tab
    bool redrawNeeded = false;
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];

      SkPoint relativeToTab = SkPoint::Make(relativeToTabCtrl.x() - tab.getOffset(), relativeToTabCtrl.y());

      if (tab.getPath().contains(relativeToTab.x(), relativeToTab.y()) == true) {
        redrawNeeded = true;

        if (selectedTab != -1) // Deselect old selected tab
          tabs[selectedTab].setSelected(false);

        selectedTab = i;
        tab.setSelected(true);
      }
    }

    if (redrawNeeded)
      return true;
    else
      return false;
  }

}
