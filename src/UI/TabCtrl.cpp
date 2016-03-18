#include <UI/TabCtrl.hpp>
#include <SkPath.h>

namespace varco {

  Tab::Tab(std::string title) :
    title(title)
  {}

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
    canvas->clipRect(this->rect); // Clip to control rect, this helps preventing overflowing
                                  // drawings when using antialiasing and other hard-to-control
                                  // drawing techniques

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    SkPaint background;
    background.setColor(SkColorSetRGB(39, 40, 34));
    canvas->drawRect(rect, background);

    //////////////////////////////////////////////////////////////////////
    // Draw the tabs
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
    SkScalar tabWidth = 120; // TODO: calculate this and clamp this
    SkScalar tabBezierWidth = 18;

    SkPaint tabBorderPaint;
    tabBorderPaint.setStyle(SkPaint::kStroke_Style);
    tabBorderPaint.setStrokeWidth(1);
    tabBorderPaint.setColor(0xff1f78b4);
    tabBorderPaint.setAntiAlias(true);
    tabBorderPaint.setStrokeCap(SkPaint::kRound_Cap);
    SkRect tabRect = SkRect::MakeLTRB(0, 5 /* top padding */, tabWidth, this->rect.fBottom);
    SkScalar yDistanceBetweenBezierPoints = (tabRect.fBottom - tabRect.fTop) / 4.0f;

    for (auto i = 0; i < tabs.size(); ++i) {
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
      canvas->drawPath(path, tabBorderPaint);
    }
    /*SkPaint line;
    line.setColor(SkColorSetRGB(255, 0, 0));
    canvas->drawLine(rect.left(), rect.bottom() - 2, rect.right(), rect.top(), line);*/

    canvas->restore();
  }  

}
