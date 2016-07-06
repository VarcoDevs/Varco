#include <WindowHandling/MainWindow.hpp>
#include <UI/TabBar/TabBar.hpp>
#include <Utils/Utils.hpp>
#include <SkCanvas.h>
#include <SkTypeface.h>
#include <SkGradientShader.h>
#include <algorithm>

#include <sstream> // DEBUG

namespace varco {

  static const float movementMilliseconds = 200.f; // Every tab movement to home position takes
                                                   // this amount of milliseconds

  Tab::Tab(TabBar *parent, std::string title) :
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
    this->parent->m_dirty = true;
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
    // SkScalar yDistanceBetweenBezierPoints = (tabRect.fBottom - tabRect.fTop) / 4.0f;

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

    sk_sp<SkTypeface> arialTypeface = SkTypeface::MakeFromName("Arial",
                                                               SkFontStyle{SkFontStyle::Weight::kNormal_Weight,
                                                               SkFontStyle::Width::kNormal_Width,
                                                               SkFontStyle::Slant::kUpright_Slant}); // Or closest match
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

  TabBar::TabBar(UIElement<ui_container_tag>& parentWindow) :
    UIElement(parentWindow)
  {
    // DEBUG - add some tabs
    //tabs.emplace_back(this, "ALOTOFTEXTALOTOFTEXT");
    //tabs.emplace_back(this, "Second tab");
    //tabs.emplace_back(this, "Third tab");
  }
  
  void TabBar::resize(SkRect rect) {
    UIElement::resize(rect); // Call base class first

    if (this->m_dirty)
      recalculateTabsRects();
  }

  void TabBar::recalculateTabsRects() {
    if (tabs.size() > 0) {
      // Calculate how much should a tab be wide if we had to cover the entire control width with the number
      // of tabs we currently have (and keeping in mind the overlapping among them)
      auto tabWidth = (getRect().width() + ((tabs.size() - 1) * tabOverlapSize)) / tabs.size();
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

  bool TabBar::getAndDecreaseMovementOffsetForTab(int tab, SkScalar& movement) {
    movement = tabs[tab].getMovementOffset();
    if (movement != 0.f) {
      // Decrease movement offset over time
      auto now = std::chrono::system_clock::now();
      auto timeFromStart = now - tabs[tab].firstMovementTime;
      // Force the amount of movement to complete in 'movementMilliseconds' ms
      auto timeFromStartInMs = std::chrono::duration_cast<std::chrono::milliseconds>(timeFromStart).count();      
      if (timeFromStartInMs >= movementMilliseconds) {
        tabs[tab].movementOffset = 0;
        movement = 0;
      } else {
        auto amount = timeFromStartInMs / static_cast<float>(movementMilliseconds);
        movement -= movement * amount;
      }
      return true; // Also needs a redraw
    }
    return false;
  }

  void TabBar::paint(SkCanvas& canvas) {

    SkRect rect = getRect(relativeToParentRect); // Drawing is performed directly on the canvas - relative rect

    //////////////////////////////////////////////////////////////////////
    // Draw the background of the control
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint background;
      background.setColor(SkColorSetARGB(255, 20, 20, 20));
      canvas.drawRect(rect, background);
    }

    //////////////////////////////////////////////////////////////////////
    // Render all tabs, but only draw unselected ones
    //////////////////////////////////////////////////////////////////////

    recalculateTabsRects();

#define TAB_SAFETY_OFFSET 20.5f // A safety offset from the swap point to prevent swap left-right fighting

    // Before rendering, check if a tracking is in progress and if a swap is needed
    SkScalar selectedTabOffset = (selectedTabIndex != -1) ? rect.fLeft + tabs[selectedTabIndex].getOffset() + tabs[selectedTabIndex].getTrackingOffset() : 0;
    if (m_tracking == true && selectedTabIndex != -1) {

      bool swapped = false; // Only one swap (left OR right) is allowed per redraw cycle

      if (selectedTabIndex > 0) { // If there are tabs on the left and..
        SkScalar leftTabEnd = rect.fLeft + tabs[selectedTabIndex - 1].getOffset() + tabs[selectedTabIndex - 1].getTrackingOffset() + tabsCurrentRect.width();
        if (selectedTabOffset + (tabsCurrentRect.width() / 2.0f) < leftTabEnd - TAB_SAFETY_OFFSET) { // ..if the center of the selected tab is inside another one
          swapTabs(selectedTabIndex, selectedTabIndex - 1);
          swapped = true;
        }
      }

      if (swapped == false && tabs.size() > selectedTabIndex + 1) { // If there are tabs on the right and..
        SkScalar rightTabBegin = rect.fLeft + tabs[selectedTabIndex + 1].getOffset() + tabs[selectedTabIndex + 1].getTrackingOffset();
        if (selectedTabOffset + (tabsCurrentRect.width() / 2.0f) > rightTabBegin + TAB_SAFETY_OFFSET) { // ..if the center of the selected tab is inside another one
          swapTabs(selectedTabIndex, selectedTabIndex + 1);
        }
      }

    }

    SkScalar tabOffset = 0.0f;
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];
      tab.setOffset(tabOffset);
      auto tabOffsetWithMovement = tabOffset;
      if (i != selectedTabIndex) { // Selected one is special and is tracked
        SkScalar movementOffset = 0.f;
        auto needRedraw = getAndDecreaseMovementOffsetForTab(i, movementOffset);
        if (movementOffset != 0)
          tabOffsetWithMovement += movementOffset;
        if (needRedraw)
          m_dirty = true;
      }
      tab.paint(); // Render the tab into its own buffer
      if (i != selectedTabIndex) // The selected one is drawn AFTER all the others
        canvas.drawBitmap(tab.getBitmap(), rect.fLeft + tabOffsetWithMovement, rect.fTop);
      tabOffset += this->tabsCurrentRect.width() - tabOverlapSize;
    }

    //////////////////////////////////////////////////////////////////////
    // Draw an horizontal line to separate code area from tabs
    //////////////////////////////////////////////////////////////////////
    {
      SkPaint line;
      line.setColor(SkColorSetARGB(255, 54, 55, 49));
      line.setStrokeWidth(2.0);
      canvas.drawLine(rect.left(), rect.bottom() - 1.5f, rect.right(), rect.bottom() - 1.5f, line);
    }

    //////////////////////////////////////////////////////////////////////
    // Now if there's a selected one, draw it last
    //////////////////////////////////////////////////////////////////////
    if (selectedTabIndex != -1) {
      // Recalculate selected tab offset (there might have been a swap)
      auto movementOrTrackingOffset = 0.0f;
      if (m_tracking == true)
        movementOrTrackingOffset = tabs[selectedTabIndex].getTrackingOffset();
      else {
        auto needRedraw = getAndDecreaseMovementOffsetForTab(selectedTabIndex, movementOrTrackingOffset);
        if (needRedraw)
          m_dirty = true;
      }

      selectedTabOffset = rect.fLeft + tabs[selectedTabIndex].getOffset() + movementOrTrackingOffset;
      canvas.drawBitmap(tabs[selectedTabIndex].getBitmap(), selectedTabOffset, rect.fTop);
    }
  }

  void TabBar::swapTabs(int tab1, int tab2) {

    // Adjust selected tab index and tracking offsets / movement offsets
    auto slideOffset = tabsCurrentRect.width() - tabOverlapSize;
    size_t unselectedTab = (selectedTabIndex == tab1 ? tab2 : tab1);
    // Swapped with a right unselected
    if ((selectedTabIndex == tab1 && tab1 < tab2) || (selectedTabIndex == tab2 && tab2 < tab1)) {

      m_startXTrackingPosition += slideOffset;
      tabs[selectedTabIndex].trackingOffset -= slideOffset;

      // Transfer the previous position for the unselected tab in movement offset (accumulate on it)
      tabs[unselectedTab].movementOffset += tabs[unselectedTab].getOffset() - tabs[selectedTabIndex].getOffset();
      tabs[unselectedTab].firstMovementTime = std::chrono::system_clock::now();

    } else { // swapped with a left unselected

      m_startXTrackingPosition -= slideOffset;
      tabs[selectedTabIndex].trackingOffset += slideOffset;

      // Transfer the previous position for the unselected tab in movement offset (accumulate on it)
      tabs[unselectedTab].movementOffset += tabs[unselectedTab].getOffset() - tabs[selectedTabIndex].getOffset();
      tabs[unselectedTab].firstMovementTime = std::chrono::system_clock::now();

    }

    tabId2tabIndexMap[tabs[tab1].uniqueId] = tab2;
    tabId2tabIndexMap[tabs[tab2].uniqueId] = tab1;

    std::swap(tabs[tab1], tabs[tab2]);
    
    if (selectedTabIndex == tab1)
      selectedTabIndex = tab2;      
    else if (selectedTabIndex == tab2)
      selectedTabIndex = tab1;
  }

  void TabBar::onLeftMouseDown(SkScalar x, SkScalar y) {
    
    SkPoint relativeToTabCtrl = SkPoint::Make(x - getRect(relativeToParentRect).fLeft, y - getRect(relativeToParentRect).fTop);
    
    // Detect if the click was on a tab
    bool redrawNeeded = false;
    for (auto i = 0; i < tabs.size(); ++i) {
      Tab& tab = tabs[i];

      SkPoint relativeToTab = SkPoint::Make(relativeToTabCtrl.x() - tab.getOffset(), relativeToTabCtrl.y());

      if (tab.getPath().contains(relativeToTab.x(), relativeToTab.y()) == true) {

        if (tab.getMovementOffset() != 0)
          return; // No dragging when returning to home position

        if (selectedTabIndex != i) { // Left click on the already selected tab won't trigger a "selected check"

          // Signal that there has been a change of selection in the tabs bar
          if (!signalDocumentChange || signalDocumentChange(tab.uniqueId) == true) {
            redrawNeeded = true;

            if (selectedTabIndex != -1) // Deselect old selected tab
              tabs[selectedTabIndex].setSelected(false);

            selectedTabIndex = i; // Do NOT confuse this (tabs bar index) with the uniqueId of the tab/document
            tab.setSelected(true);
          }          
        }
        
        m_tracking = true;
        m_startXTrackingPosition = x;
        m_parentContainer.startMouseCapture();
      }
    }

//    if (redrawNeeded) {
//      this->m_dirty = true;
//      m_parentContainer.repaint();
//    }
  }

  void TabBar::onMouseMove(SkScalar x, SkScalar y) {
    if (!m_tracking)
      return;

    tabs[selectedTabIndex].trackingOffset = x - m_startXTrackingPosition;
    tabs[selectedTabIndex].dirty = true;
    // m_dirty = true;
    // m_parentContainer.repaint();
  }

  void TabBar::onLeftMouseUp(SkScalar x, SkScalar y) {
    stopTracking();
  }

  int TabBar::addNewTab(std::string title, bool makeSelected) {    
    if (makeSelected && selectedTabIndex != -1) // Deselect old selected tab
      tabs[selectedTabIndex].setSelected(false);

    tabs.emplace_back(this, title, Tab::private_access{});

    // Assign a free id to the tab (this is not the tab index in the control)
    auto getFreeIdFromPool = [&](int tabIndex) {
      if (tabIdHoles.begin() != tabIdHoles.end()) { // If there's a hole in the pool set, grab it and fill it
        int id = *tabIdHoles.begin();
        tabIdHoles.erase(tabIdHoles.begin());
        tabId2tabIndexMap[id] = tabIndex;
        return id;
      } else { // else simply grab the next free id
        int id = static_cast<int>(tabId2tabIndexMap.size());
        tabId2tabIndexMap[id] = tabIndex;
        return id;
      }
    };

    auto newTabId = getFreeIdFromPool(static_cast<int>(tabs.size() - 1));
    tabs.back().uniqueId = newTabId;

    if (makeSelected) {
      tabs.back().setSelected(true);
      selectedTabIndex = static_cast<int>(tabs.size() - 1);
    }

    // this->m_parentContainer.repaint();

    return newTabId;
  }

  bool TabBar::isTrackingActive() {
    return m_tracking;
  }

  void TabBar::stopTracking() {
    //OutputDebugString("STOP TRACKING - back to home position");
    m_tracking = false;

    // Transfer the current tracking offset in movement offset (accumulate on it)
    tabs[selectedTabIndex].movementOffset += tabs[selectedTabIndex].trackingOffset;
    tabs[selectedTabIndex].firstMovementTime = std::chrono::system_clock::now();

    tabs[selectedTabIndex].trackingOffset = 0.0f;

    tabs[selectedTabIndex].dirty = true;
//    m_dirty = true;
//    m_parentContainer.stopMouseCapture();
//    m_parentContainer.repaint();
  }

}
