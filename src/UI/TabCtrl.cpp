#include <WindowHandling/MainWindow.hpp>
#include <UI/TabCtrl.hpp>
#include <Utils/Utils.hpp>
#include <SkTypeface.h>
#include <SkGradientShader.h>
#include <algorithm>

#include <sstream> // DEBUG

namespace varco {

  static const float movementSpeed = 0.5f;

  Tab::Tab(TabCtrl *parent, std::string title) :
    parent(parent),
    title(title)
  {
    resize();
  }

  SkBitmap& Tab::getBitmap() {
    return this->bitmap;
  }

  SkPath& Tab::getPath() {
    return this->path;
  }

  void Tab::setSelected(bool selected) {
    this->selected = selected;
    this->dirty = true;
    this->parent->dirty = true;
  }

  void Tab::setOffset(SkScalar offset) {
    this->parentOffset = offset;
  }

  SkScalar Tab::getOffset() {
    return this->parentOffset;
  }

  SkScalar Tab::getMovementOffset() {
    return this->movementOffset;
  }

  SkScalar Tab::getTrackingOffset() {
    return this->trackingOffset;
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
    SkPoint topToBottomPoints[2] = {
      SkPoint::Make(this->parent->tabsCurrentRect.width() / 2.0f, this->parent->tabsCurrentRect.top()),
      SkPoint::Make(this->parent->tabsCurrentRect.width() / 2.0f, this->parent->tabsCurrentRect.bottom())
    };
    if (this->selected == true) {      
      SkColor colors[2] = { SkColorSetARGB(255, 52, 53, 57), SkColorSetARGB(255, 39, 40, 34) };
      auto shader = SkGradientShader::MakeLinear(topToBottomPoints, colors, NULL, 2, SkShader::kClamp_TileMode, 0, NULL);
      tabBorderPaint.setShader(shader);
    } else {      
      SkColor colors[2] = { SkColorSetARGB(255, 78, 78, 76), SkColorSetARGB(255, 48, 48, 46) };
      auto shader = SkGradientShader::MakeLinear(topToBottomPoints, colors, NULL, 2, SkShader::kClamp_TileMode, 0, NULL);
      tabBorderPaint.setShader(shader);
    }
    SkRect tabRect = SkRect::MakeLTRB(0, 5 /* top padding */, this->parent->tabsCurrentRect.width(), this->parent->tabsCurrentRect.fBottom);
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
      tabRect.fLeft + tabBezierWidth, tabRect.fTop + 1.0f // P2
      );
    path.lineTo(tabRect.fRight - tabBezierWidth, tabRect.fTop + 1.0f); // P2
    path.cubicTo(
      tabRect.fRight - tabBezierWidth + ((1.0f - 0.64f) * tabBezierWidth), tabRect.fTop, // C2
      tabRect.fRight - tabBezierWidth + ((1.0f - 0.60f) * tabBezierWidth), tabRect.fBottom, // C1
      tabRect.fRight, tabRect.fBottom // P0
      );
    tabBorderPaint.setStyle(SkPaint::kFill_Style); // Fill with the background color
    canvas.drawPath(path, tabBorderPaint);

    tabBorderPaint.setShader(nullptr);
    tabBorderPaint.setStyle(SkPaint::kStroke_Style); // Remark the contour with a line color
    tabBorderPaint.setColor(SkColorSetARGB(255, 54, 55, 49));
    canvas.drawPath(path, tabBorderPaint);

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

  void Tab::resize() {
    bitmap.allocPixels(SkImageInfo::Make( (int)this->parent->tabsCurrentRect.width(),
                                          (int)this->parent->tabsCurrentRect.height(),
                                          kN32_SkColorType, kPremul_SkAlphaType) );
    this->dirty = true;
  }

  TabCtrl::TabCtrl(MainWindow& parentWindow) :
    parentWindow(parentWindow)
  {
    // DEBUG - add some tabs
    tabs.emplace_back(this, "ALOTOFTEXTALOTOFTEXT");
    tabs.emplace_back(this, "Second tab");
    tabs.emplace_back(this, "Third tab");
  }
  
  void TabCtrl::resize(SkRect rect) {
    if (this->rect != rect) {
      this->rect = rect;
      this->bitmap.allocPixels(SkImageInfo::Make((int)rect.width(), (int)rect.height(), kN32_SkColorType, kPremul_SkAlphaType));
      dirty = true;
      recalculateTabsRects();
    }
  }

  void TabCtrl::recalculateTabsRects() {
    if (tabs.size() > 0) {
      // Calculate how much should a tab be wide if we had to cover the entire control width with the number
      // of tabs we currently have (and keeping in mind the overlapping among them)
      auto tabWidth = (this->rect.width() + ((tabs.size() - 1) * tabOverlapSize)) / tabs.size();
      tabWidth = std::min(std::max(tabWidth, TAB_MIN_WIDTH), TAB_MAX_WIDTH); // Clamp to extremes
      if (this->tabsCurrentRect.fRight != tabWidth) {
        this->tabsCurrentRect.fRight = tabWidth;
        // Every tab has to be resized
        std::for_each(this->tabs.begin(), this->tabs.end(), [](auto& tab) {
          tab.resize(); // Also automatically sets the tab as dirty
        });        
      }
    }
  }

  bool TabCtrl::getAndDecreaseMovementOffsetForTab(size_t tab, SkScalar& movement) {
    movement = tabs[tab].getMovementOffset();
    if (movement != 0.f) {
      // Decrease movement offset over time
      auto now = std::chrono::system_clock::now();
      auto elapsedSeconds = now - tabs[tab].lastMovementTime;
      auto subtrahend = std::chrono::duration_cast<std::chrono::milliseconds>(elapsedSeconds).count() * movementSpeed;
      if (movement >= 0.f) {
        movement = std::max(
          movement - subtrahend,
          0.0f
          );
      }
      else {
        movement = std::min(
          movement + subtrahend,
          0.0f
          );
      }

      tabs[tab].movementOffset = movement;
      tabs[tab].lastMovementTime = now;
      return true; // Also needs a redraw
    }
    return false;
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

    dirty = false; // It will be false at the end of this function, unless overridden

    SkCanvas canvas(this->bitmap);

    canvas.save();

    // The following is no longer necessary due to drawing into different bitmap buffers
    //canvas->clipRect(this->rect); // Clip to control rect, this helps preventing overflowing
                                    // drawings when using antialiasing and other hard-to-control
                                    // drawing techniques

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint background;
      background.setColor(SkColorSetARGB(255, 20, 20, 20));
      canvas.drawRect(this->rect, background);
    }

    //////////////////////////////////////////////////////////////////////
    // Render all tabs, but only draw unselected ones
    //////////////////////////////////////////////////////////////////////

    recalculateTabsRects();

#define TAB_SAFETY_OFFSET 20.5f // A safety offset from the swap point to prevent swap left-right fighting

    // Before rendering, check if a tracking is in progress and if a swap is needed
    SkScalar selectedTabOffset = (selectedTab != -1) ? this->rect.fLeft + tabs[selectedTab].getOffset() + tabs[selectedTab].getTrackingOffset() : 0;
    if (m_tracking == true && selectedTab != -1) {

      bool swapped = false; // Only one swap (left OR right) is allowed per redraw cycle

      if (selectedTab > 0) { // If there are tabs on the left and..
        SkScalar leftTabEnd = this->rect.fLeft + tabs[selectedTab - 1].getOffset() + tabs[selectedTab - 1].getTrackingOffset() + tabsCurrentRect.width();
        if (selectedTabOffset + (tabsCurrentRect.width() / 2.0f) < leftTabEnd - TAB_SAFETY_OFFSET) { // ..if the center of the selected tab is inside another one
          swapTabs(selectedTab, selectedTab - 1);
          swapped = true;
        }
      }

      if (swapped == false && tabs.size() > selectedTab + 1) { // If there are tabs on the right and..
        SkScalar rightTabBegin = this->rect.fLeft + tabs[selectedTab + 1].getOffset() + tabs[selectedTab + 1].getTrackingOffset();
        if (selectedTabOffset + (tabsCurrentRect.width() / 2.0f) > rightTabBegin + TAB_SAFETY_OFFSET) { // ..if the center of the selected tab is inside another one
          swapTabs(selectedTab, selectedTab + 1);
        }
      }

    }

    SkScalar tabOffset = 0.0f;
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];
      tab.setOffset(tabOffset);
      auto tabOffsetWithMovement = tabOffset;
      if (i != selectedTab) { // Selected one is special and is tracked
        SkScalar movementOffset = 0.f;
        auto needRedraw = getAndDecreaseMovementOffsetForTab(i, movementOffset);
        if (movementOffset != 0.0f)
          tabOffsetWithMovement += movementOffset;
        if (needRedraw)
          dirty = true;
      }
      tab.paint(); // Render the tab into its own buffer
      if (i != selectedTab) // The selected one is drawn AFTER all the others
        canvas.drawBitmap(tab.getBitmap(), this->rect.fLeft + tabOffsetWithMovement, this->rect.fTop);
      tabOffset += this->tabsCurrentRect.width() - tabOverlapSize;
    }

    //////////////////////////////////////////////////////////////////////
    // Draw an horizontal line to separate code area from tabs
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint line;
      line.setColor(SkColorSetARGB(255, 54, 55, 49));
      line.setStrokeWidth(2.0);
      canvas.drawLine(this->rect.left(), this->rect.bottom() - 1.5f, this->rect.right(), this->rect.bottom() - 1.5f, line);
    }

    //////////////////////////////////////////////////////////////////////
    // Now if there's a selected one, draw it last
    //////////////////////////////////////////////////////////////////////
    if (selectedTab != -1) {
      // Recalculate selected tab offset (there might have been a swap)
      auto movementOrTrackingOffset = 0.0f;
      if (m_tracking == true)
        movementOrTrackingOffset = tabs[selectedTab].getTrackingOffset();
      else {
        auto needRedraw = getAndDecreaseMovementOffsetForTab(selectedTab, movementOrTrackingOffset);
        if (needRedraw)
          dirty = true;
      }

      selectedTabOffset = this->rect.fLeft + tabs[selectedTab].getOffset() + movementOrTrackingOffset;
      canvas.drawBitmap(tabs[selectedTab].getBitmap(), selectedTabOffset, this->rect.fTop);
    }

    canvas.flush();
    canvas.restore();

    if (dirty == true)
      parentWindow.redraw(); // Schedule a redraw
  }

  void TabCtrl::swapTabs(size_t tab1, size_t tab2) {

    // Adjust selected tab index and tracking offsets / movement offsets
    auto slideOffset = tabsCurrentRect.width() - tabOverlapSize;
    size_t unselectedTab = (selectedTab == tab1 ? tab2 : tab1);
    // Swapped with a right unselected
    if (selectedTab == tab1 && tab1 < tab2 || selectedTab == tab2 && tab2 < tab1) {

      m_startXTrackingPosition += slideOffset;
      tabs[selectedTab].trackingOffset -= slideOffset;

      // Transfer the previous position for the unselected tab in movement offset (accumulate on it)
      tabs[unselectedTab].movementOffset += tabs[unselectedTab].getOffset() - tabs[selectedTab].getOffset();
      tabs[unselectedTab].lastMovementTime = std::chrono::system_clock::now();

    } else { // swapped with a left unselected

      m_startXTrackingPosition -= slideOffset;
      tabs[selectedTab].trackingOffset += slideOffset;

      // Transfer the previous position for the unselected tab in movement offset (accumulate on it)
      tabs[unselectedTab].movementOffset += tabs[unselectedTab].getOffset() - tabs[selectedTab].getOffset();
      tabs[unselectedTab].lastMovementTime = std::chrono::system_clock::now();

    }

    std::swap(tabs[tab1], tabs[tab2]);
    
    if (selectedTab == tab1)
      selectedTab = tab2;      
    else if (selectedTab == tab2)
      selectedTab = tab1;
  }

  void TabCtrl::onLeftMouseDown(SkScalar x, SkScalar y) {
    
    SkPoint relativeToTabCtrl = SkPoint::Make(x - this->rect.fLeft, y - this->rect.fTop);
    
    // Detect if the click was on a tab
    bool redrawNeeded = false;
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];

      SkPoint relativeToTab = SkPoint::Make(relativeToTabCtrl.x() - tab.getOffset(), relativeToTabCtrl.y());

      if (tab.getPath().contains(relativeToTab.x(), relativeToTab.y()) == true) {

        if (selectedTab != i) { // Left click on the already selected tab won't trigger a "selected check"          
          redrawNeeded = true;

          if (selectedTab != -1) // Deselect old selected tab
            tabs[selectedTab].setSelected(false);

          selectedTab = i;
          tab.setSelected(true);
        }
        
        m_tracking = true;
        m_startXTrackingPosition = x;
        parentWindow.startMouseCapture();
      }
    }

    if (redrawNeeded) {
      this->dirty = true;
      parentWindow.redraw();
    }
  }

  void TabCtrl::onLeftMouseMove(SkScalar x, SkScalar y) {
    if (!m_tracking)
      return;

    tabs[selectedTab].trackingOffset = x - m_startXTrackingPosition;
    tabs[selectedTab].dirty = true;
    this->dirty = true;
    parentWindow.redraw();
  }

  void TabCtrl::onLeftMouseUp(SkScalar x, SkScalar y) {
    stopTracking();
  }

  void TabCtrl::addNewTab(std::string title, bool makeSelected) {    
    if (selectedTab != -1) // Deselect old selected tab
      tabs[selectedTab].setSelected(false);
    tabs.emplace_back(this, title);
    tabs.back().setSelected(true);
    selectedTab = tabs.size() - 1;
    this->parentWindow.redraw();
  }

  bool TabCtrl::isTrackingActive() {
    return m_tracking;
  }

  void TabCtrl::stopTracking() {
    OutputDebugString("STOP TRACKING - back to home position");
    m_tracking = false;

    // Transfer the current tracking offset in movement offset (accumulate on it)
    tabs[selectedTab].movementOffset += tabs[selectedTab].trackingOffset;
    tabs[selectedTab].lastMovementTime = std::chrono::system_clock::now();

    tabs[selectedTab].trackingOffset = 0.0f;

    tabs[selectedTab].dirty = true;
    this->dirty = true;
    parentWindow.redraw();
  }

}
