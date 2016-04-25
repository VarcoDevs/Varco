#ifndef VARCO_TABCTRL_HPP
#define VARCO_TABCTRL_HPP

#include <UI/UIElement.hpp>
#include <SkPath.h>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include <set>

namespace varco {

  class TabBar;  

  class Tab { // Represents a tab inside the tab control
  private:

    // Why are we using delegate constructors and declaring a private_access inner structure?
    //
    // We want to have Tab's private constructor accessible only to TabBar but we also want
    // to be able to construct objects in-place from TabBar with
    //   tabs_vector.emplace_back("I'm a new tab");
    // The problem with the approach above is: emplace_back delegates construction of the
    // Tab object to std::allocator (construct()) and this is not a friend of Tab.
    //
    // To workaround this issue we can either supply a custom TabAllocator which, being an inner class,
    // as of [class.access.nest]/p1
    //
    //   "A nested class is a member and as such has the same access rights as any other member."
    //
    // would allow TabsBar to emplace_back Tabs by using the custom member allocator provided.
    //
    // Unfortunately this doesn't work on MSVC and thus we need a plan B: a public constructor
    //  Tab(TabBar *parent, std::string title, const private_access&);
    // that, since requires a private_access structure (privately declared), can only be called by
    // a friend class. Since the constructor (called by the allocator) then delegates the actual object
    // construction to another constructor (C++11 and above only), this will ensure there will be no
    // access problems.

    struct private_access {};

  public:
    Tab(TabBar *parent, std::string title, const private_access&) : // Only accessible to friend classes
      Tab(parent, title)
    {}
  private:

    friend class TabBar;
  
    Tab(TabBar *parent, std::string title);

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

    TabBar *parent;
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

  class TabBar : public UIElement<ui_control_tag> { // The main tab control
    friend class Tab;
    // Initialization values for all the tabs in this control
    const SkScalar TAB_MAX_WIDTH = 150.0;
    const SkScalar TAB_MIN_WIDTH = 50.0;
    const SkScalar tabOverlapSize = 20.0f; // The amount of overlap between two adjacent tabs
    SkRect tabsCurrentRect = SkRect::MakeLTRB(0, 0, TAB_MAX_WIDTH, 33.0); // The rect that encloses the tab (coords relative to the tab control)
  public:
    TabBar(UIElement<ui_container_tag>& parentWindow);
    
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
