#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include <UI/UICtrlBase.hpp>
#include <SkPath.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <set>

namespace varco {

  class TabCtrl;  

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
    SkScalar getMovementOffset();
    SkScalar getTrackingOffset();

  private:
    TabCtrl *parent;
    std::string title;
    int uniqueId;
    SkBitmap bitmap; // The tab will be rendered here
    bool dirty = true;
    SkPath path; // The path where clicks and inputs are accepted
    SkScalar parentOffset; // The offset from the start of the parent tab control
    SkScalar movementOffset = 0.0; // A movement offset that decreases over time to reach
                                   // the parentOffset stationary value
    std::chrono::time_point<std::chrono::system_clock> firstMovementTime;
    SkScalar trackingOffset = 0.0f; // The additional offset due to tracking
    bool selected = false; // Is this a selected tab?
  };

  class TabCtrl : public UICtrlBase { // The main tab control
    friend class Tab;
    // Initialization values for all the tabs in this control
    const SkScalar TAB_MAX_WIDTH = 150.0;
    const SkScalar TAB_MIN_WIDTH = 50.0;
    const SkScalar tabOverlapSize = 20.0f; // The amount of overlap between two adjacent tabs
    SkRect tabsCurrentRect = SkRect::MakeLTRB(0, 0, TAB_MAX_WIDTH, 33.0); // The rect that encloses the tab (coords relative to the tab control)
  public:
    TabCtrl(MainWindow& parentWindow);
    
    void resize(SkRect rect) override; // Set the new rect where the control will redraw itself    
    void paint() override; // Paint the control in the bitmap
    void onLeftMouseDown(SkScalar x, SkScalar y);
    void onLeftMouseMove(SkScalar x, SkScalar y);
    void onLeftMouseUp(SkScalar x, SkScalar y);
    // Adds a new tab and returns a unique identifier
    int addNewTab(std::string title, bool makeSelected = true);

    bool isTrackingActive();
    void stopTracking();

  private:
    std::vector<Tab> tabs;
    int selectedTabIndex = -1; // The position index of the selected tab

    // Nb. there are two different kind of indices:
    //  - Tab id -> this is unique for every tab and can never change
    //  - Tab index -> this is the position of the tab in the control vector and might be change (swap)
    // Users only deal with tab ids
    std::map<int, int> tabId2tabIndexMap; // The tab_id->control_position_index map for the tabs
    std::set<int> tabIdHoles; // The non-contiguous tab ids (left by deleted tabs)

    // Tracking section
    bool m_tracking = false; // Whether a tab is being tracked (dragged)
    SkScalar m_startXTrackingPosition;
    void swapTabs(int tab1, int tab2);

    void recalculateTabsRects(); // Recalculates all the tabs rects (e.g. shrinks them in case the window got smaller)
    // Return true if the control needs redrawing
    bool getAndDecreaseMovementOffsetForTab(int tab, SkScalar& movement);
  };
}

#endif // VARCO_TABCTRL_HPP
