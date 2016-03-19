#include <UI/TabCtrl.hpp>
#include <SkPath.h>
#include <SkTypeface.h>

namespace varco {

  Tab::Tab(std::string title) :
    title(title)
  {
    rect = SkRect::MakeLTRB(0, 0, 140.0, 33.0); // Default tab rect
    bitmap.allocPixels(SkImageInfo::Make((int)rect.width(), (int)rect.height(), kN32_SkColorType, kPremul_SkAlphaType));
  }

  SkBitmap& Tab::getBitmap() {
    return this->bitmap;
  }

  void Tab::paint() {

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
    tabBorderPaint.setColor(SkColorSetARGB(255, 40, 40, 40));
    SkRect tabRect = SkRect::MakeLTRB(0, 5 /* top padding */, this->rect.width(), this->rect.fBottom);
    SkScalar yDistanceBetweenBezierPoints = (tabRect.fBottom - tabRect.fTop) / 4.0f;

    SkPath path;
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

    SkTypeface *monospaceTypeface = SkTypeface::CreateFromName("Consolas", SkTypeface::kNormal); // Or closest match
    SkPaint tabTextPaint;
    tabTextPaint.setColor(SK_ColorWHITE);
    tabTextPaint.setTextSize(SkIntToScalar(12));
    tabTextPaint.setAntiAlias(true);
    tabTextPaint.setTypeface(monospaceTypeface);
    canvas.drawText(title.data(), title.size(), tabRect.fLeft + 10, tabRect.fBottom - 10, tabTextPaint);

    canvas.flush();
  }

  TabCtrl::TabCtrl() {
    // DEBUG - add some tabs
    tabs.emplace_back("Tab0");
  }
  
  void TabCtrl::setRect(SkRect rect) {
    this->rect = rect;
  }

  void TabCtrl::paint(SkCanvas *canvas) {

    // TODO: redraw only if necessary

    canvas->save();
    //canvas->clipRect(this->rect); // Clip to control rect, this helps preventing overflowing
                                  // drawings when using antialiasing and other hard-to-control
                                  // drawing techniques

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    SkPaint background;
    background.setColor(SkColorSetARGB(255, 20, 20, 20));
    canvas->drawRect(this->rect, background);

  
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];
      tab.paint(); // Render tab
      canvas->drawBitmap(tab.getBitmap(), this->rect.fLeft, this->rect.fTop);
    }

    canvas->flush();

    canvas->restore();
  }  

}
