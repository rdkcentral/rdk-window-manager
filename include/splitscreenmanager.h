/**
 * POC: Dynamic Split-Screen Manager
 *
 * Manages up to 4 compositor panes in configurable split-screen layouts
 * with smooth animated transitions and a dynamic resizable divider.
 *
 * Layouts
 *   SideBySide        - [ Left  | Right ]
 *   TopBottom         - [ Top   / Bottom ]
 *   Quad              - [ TL | TR / BL | BR ]
 *   PictureInPicture  - [ Main  +  small PIP overlay ]
 *
 * Usage
 *   auto& ss = SplitScreenManager::instance();
 *   ss.activate({"appA", "appB"}, Layout::SideBySide, 1920, 1080);
 *
 *   // dynamically drag the divider:
 *   ss.setSplitRatio(0.35f);
 *
 *   // switch layout with a smooth animation:
 *   ss.setLayout(Layout::TopBottom);
 *
 *   // call every frame (already wired into RdkWindowManager::update()):
 *   ss.update();
 **/

#ifndef RDK_WINDOW_MANAGER_SPLIT_SCREEN_MANAGER_H
#define RDK_WINDOW_MANAGER_SPLIT_SCREEN_MANAGER_H

#include <string>
#include <vector>
#include <cstdint>

namespace RdkWindowManager
{

class SplitScreenManager
{
public:
    // -----------------------------------------------------------------
    // Layout modes
    // -----------------------------------------------------------------
    enum class Layout
    {
        SideBySide,        ///< Two panes split left/right
        TopBottom,         ///< Two panes split top/bottom
        Quad,              ///< Four equal quadrants
        PictureInPicture   ///< Main full-screen + small overlay (bottom-right)
    };

    // -----------------------------------------------------------------
    // Singleton accessor
    // -----------------------------------------------------------------
    static SplitScreenManager& instance();

    // -----------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------

    /**
     * Activate split-screen mode.
     * @param clients   Ordered list of compositor client names (max 4).
     * @param layout    Initial layout.
     * @param screenW   Display width in pixels.
     * @param screenH   Display height in pixels.
     * @return true on success.
     */
    bool activate(const std::vector<std::string>& clients,
                  Layout   layout,
                  uint32_t screenW,
                  uint32_t screenH);

    /**
     * Animate all panes back to full-screen then deactivate.
     */
    bool deactivate();

    // -----------------------------------------------------------------
    // Dynamic controls (each triggers an animated transition)
    // -----------------------------------------------------------------

    /** Switch to a different layout with a smooth animation. */
    bool setLayout(Layout layout);

    /**
     * Move the divider.  Range [0.05, 0.95].
     * Applies to SideBySide (horizontal divider) and TopBottom (vertical).
     */
    bool setSplitRatio(float ratio);

    /** Move focus to a pane by index (0-based). */
    bool focusPane(int index);

    /** Swap the clients assigned to two pane slots and re-animate. */
    bool swapPanes(int a, int b);

    // -----------------------------------------------------------------
    // Per-frame driver — must be called once per frame
    // -----------------------------------------------------------------
    void update();

    // -----------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------
    bool        isActive()      const;
    Layout      currentLayout() const;
    float       splitRatio()    const;
    int         focusedPane()   const;
    std::size_t paneCount()     const;

    /**
     * Return the client names of the active panes in their current display
     * order (which may differ from the original activate() list after swapPanes()).
     * Returns an empty vector when split-screen is not active.
     */
    std::vector<std::string> getClients() const;

    /**
     * Lerp factor applied each frame.  Higher = faster animation.
     * Default 0.12 gives ~300 ms settle time at 40 fps.
     */
    void setAnimationSpeed(float speed);

    /** Pixel gap inserted between panes.  Default 4 px. */
    void setGapPixels(uint32_t gap);

private:
    SplitScreenManager() = default;

    // Screen-space rectangle (float to support sub-pixel lerp)
    struct PaneRect
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 1920.0f;
        float h = 1080.0f;
    };

    struct Pane
    {
        std::string client;
        PaneRect    current; ///< Interpolated position this frame
        PaneRect    target;  ///< Destination for the ongoing animation
    };

    // Recompute target rects for all panes using the current layout/ratio.
    void computeTargetRects();

    // Push the current (interpolated) rect to the compositor via scaleToFit.
    void applyRect(const Pane& pane) const;

    // -----------------------------------------------------------------
    // State
    // -----------------------------------------------------------------
    bool     mActive       = false;
    bool     mDeactivating = false;
    Layout   mLayout       = Layout::SideBySide;
    float    mSplitRatio   = 0.5f;
    float    mAnimSpeed    = 0.12f;
    uint32_t mGap          = 4;
    uint32_t mScreenW      = 1920;
    uint32_t mScreenH      = 1080;
    int      mFocusedPane  = 0;

    std::vector<Pane> mPanes;
};

} // namespace RdkWindowManager

#endif // RDK_WINDOW_MANAGER_SPLIT_SCREEN_MANAGER_H
