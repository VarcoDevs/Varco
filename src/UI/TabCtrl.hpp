#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkPath.h>
#include <string>
#include <vector>
#include <memory>

namespace varco {

  class TabCtrl;
  class MainWindow;

  class Tab { // Represents a tab inside the tab control
    friend class TabCtrl;
  public:
    Tab(TabCtrl *parent, std::string title);

    // Paint the tab into its own bitmap
    void paint();
    void resize();
    SkBitmap& getBitmap();
    SkPath& getPath();
    void setSelected(bool);
    void setOffset(SkScalar);
    SkScalar getOffset();
    SkScalar getTrackingOffset();

  private:
    TabCtrl *parent;
    std::string title;
    SkBitmap bitmap; // The tab will be rendered here
    bool dirty = true;
    SkPath path; // The path where clicks and inputs are accepted
    SkScalar parentOffset; // The offset from the start of the parent tab control
    SkScalar trackingOffset = 0.0f; // The additional offset due to tracking
    bool selected = false; // Is this a selected tab?
  };

  class TabCtrl { // The main tab control
    friend class Tab;
    // Initialization values for all the tabs in this control
    const SkScalar TAB_MAX_WIDTH = 150.0;
    const SkScalar TAB_MIN_WIDTH = 50.0;
    const SkScalar tabOverlapSize = 20.0f; // The amount of overlap between two adjacent tabs
    SkRect tabsCurrentRect = SkRect::MakeLTRB(0, 0, TAB_MAX_WIDTH, 33.0); // The rect that encloses the tab (coords relative to the tab control)
  public:
    TabCtrl(MainWindow& parentWindow);
    
    void resize(SkRect rect); // Set the new rect where the control will redraw itself
    SkBitmap& getBitmap();
    SkRect getRect();
    void paint(); // Paint the control in the bitmap
    void onLeftMouseDown(SkScalar x, SkScalar y);
    void onLeftMouseMove(SkScalar x, SkScalar y);
    void onLeftMouseUp(SkScalar x, SkScalar y);
    void addNewTab(std::string title, bool makeSelected = true); // TODO: return unique id!

    bool isTrackingActive();
    void stopTracking();

  private:
    MainWindow& parentWindow;
    SkRect rect; // Rect where to draw the control, relative to the client area of the parent
    SkBitmap bitmap; // The entire control will be rendered here
    bool dirty = true;
    std::vector<Tab> tabs;
    size_t selectedTab = -1; // The index of the selected tab

    // Tracking section
    bool m_tracking = false; // Whether a tab is being tracked (dragged)
    SkScalar m_startXTrackingPosition;
    void swapTabs(size_t tab1, size_t tab2);

    void recalculateTabsRects(); // Recalculates all the tabs rects (e.g. shrinks them in case the window got smaller)
  };
}

#endif // VARCO_TABCTRL_HPP
