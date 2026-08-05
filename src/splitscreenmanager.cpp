/**
 * POC: Dynamic Split-Screen Manager — implementation
 *
 * Per-frame animation model
 * ─────────────────────────
 * Each pane holds a `current` rect and a `target` rect.
 * Every call to update() applies an exponential-ease-out lerp:
 *
 *   current += (target − current) × animSpeed
 *
 * At 40 fps with the default animSpeed of 0.12 the pane reaches 99 % of
 * its target in ≈ 300 ms.  Increasing animSpeed makes transitions snappier;
 * setting it to 1.0 gives an instant snap.
 *
 * The position is pushed to the compositor via
 *   CompositorController::scaleToFit(client, x, y, w, h)
 * which calls setPosition() + setScale() on the underlying RdkCompositor.
 **/

#include "splitscreenmanager.h"
#include "compositorcontroller.h"
#include "logger.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace RdkWindowManager
{

// ── Singleton ────────────────────────────────────────────────────────────────

SplitScreenManager& SplitScreenManager::instance()
{
    static SplitScreenManager inst;
    return inst;
}

// ── Lifecycle ────────────────────────────────────────────────────────────────

bool SplitScreenManager::activate(const std::vector<std::string>& clients,
                                   Layout   layout,
                                   uint32_t screenW,
                                   uint32_t screenH)
{
    if (clients.empty())
    {
        Logger::log(Error, "[SplitScreen] activate: no clients provided\n");
        return false;
    }

    mScreenW      = screenW;
    mScreenH      = screenH;
    mLayout       = layout;
    mActive       = true;
    mDeactivating = false;
    mFocusedPane  = 0;

    mPanes.clear();
    for (const auto& c : clients)
    {
        Pane p;
        p.client  = c;
        // All panes start at full-screen; computeTargetRects() sets the
        // destinations so the split-in animation plays automatically.
        p.current = { 0.0f, 0.0f, (float)screenW, (float)screenH };
        p.target  = p.current;
        mPanes.push_back(p);
    }

    computeTargetRects();

    // Focus the first pane by default.
    CompositorController::setFocus(mPanes[0].client);

    Logger::log(Information,
                "[SplitScreen] activated: %zu pane(s), layout=%d, screen=%ux%u\n",
                mPanes.size(), static_cast<int>(mLayout), mScreenW, mScreenH);
    return true;
}

bool SplitScreenManager::deactivate()
{
    if (!mActive)
        return false;

    mDeactivating = true;

    // Animate each pane back to full-screen; update() will clear mActive once
    // all panes have settled.
    const PaneRect full{ 0.0f, 0.0f, (float)mScreenW, (float)mScreenH };
    for (auto& p : mPanes)
        p.target = full;

    Logger::log(Information,
                "[SplitScreen] deactivating — animating %zu pane(s) to full-screen\n",
                mPanes.size());
    return true;
}

// ── Dynamic controls ─────────────────────────────────────────────────────────

bool SplitScreenManager::setLayout(Layout layout)
{
    if (!mActive || mDeactivating)
        return false;

    mLayout = layout;
    computeTargetRects();

    Logger::log(Information, "[SplitScreen] layout → %d\n",
                static_cast<int>(mLayout));
    return true;
}

bool SplitScreenManager::setSplitRatio(float ratio)
{
    if (!mActive || mDeactivating)
        return false;

    mSplitRatio = std::max(0.05f, std::min(0.95f, ratio));
    computeTargetRects();

    Logger::log(Information, "[SplitScreen] split ratio → %.2f\n", mSplitRatio);
    return true;
}

bool SplitScreenManager::focusPane(int index)
{
    if (!mActive || mDeactivating)
        return false;
    if (index < 0 || index >= static_cast<int>(mPanes.size()))
        return false;

    mFocusedPane = index;
    CompositorController::setFocus(mPanes[index].client);

    Logger::log(Information, "[SplitScreen] focus → pane %d (%s)\n",
                index, mPanes[index].client.c_str());
    return true;
}

bool SplitScreenManager::swapPanes(int a, int b)
{
    if (!mActive || mDeactivating)
        return false;
    if (a < 0 || a >= static_cast<int>(mPanes.size()))
        return false;
    if (b < 0 || b >= static_cast<int>(mPanes.size()))
        return false;
    if (a == b)
        return true;

    // Swap the client names; the rects stay where they are so the apps
    // appear to slide across to each other's position.
    std::swap(mPanes[a].client, mPanes[b].client);
    computeTargetRects();

    Logger::log(Information, "[SplitScreen] swapped panes %d ↔ %d\n", a, b);
    return true;
}

// ── Per-frame driver ─────────────────────────────────────────────────────────

void SplitScreenManager::update()
{
    if (!mActive)
        return;

    bool allSettled = true;

    for (auto& pane : mPanes)
    {
        PaneRect&       cur = pane.current;
        const PaneRect& tgt = pane.target;

        // Check whether each dimension has settled (sub-half-pixel threshold).
        auto near = [](float a, float b) { return std::fabs(a - b) < 0.5f; };

        bool settled = near(cur.x, tgt.x) &&
                       near(cur.y, tgt.y) &&
                       near(cur.w, tgt.w) &&
                       near(cur.h, tgt.h);

        if (settled)
        {
            cur = tgt; // snap to avoid drift accumulation
        }
        else
        {
            // Exponential ease-out: approach target by mAnimSpeed each frame.
            cur.x += (tgt.x - cur.x) * mAnimSpeed;
            cur.y += (tgt.y - cur.y) * mAnimSpeed;
            cur.w += (tgt.w - cur.w) * mAnimSpeed;
            cur.h += (tgt.h - cur.h) * mAnimSpeed;
            allSettled = false;
        }

        applyRect(pane);
    }

    // Complete deactivation once animation finishes.
    if (allSettled && mDeactivating)
    {
        mActive       = false;
        mDeactivating = false;
        Logger::log(Information, "[SplitScreen] deactivation complete\n");
    }
}

// ── Accessors ────────────────────────────────────────────────────────────────

bool        SplitScreenManager::isActive()      const { return mActive; }
SplitScreenManager::Layout
            SplitScreenManager::currentLayout() const { return mLayout; }
float       SplitScreenManager::splitRatio()    const { return mSplitRatio; }
int         SplitScreenManager::focusedPane()   const { return mFocusedPane; }
std::size_t SplitScreenManager::paneCount()     const { return mPanes.size(); }

std::vector<std::string> SplitScreenManager::getClients() const
{
    if (!mActive)
        return {};
    std::vector<std::string> clients;
    clients.reserve(mPanes.size());
    for (const auto& pane : mPanes)
        clients.push_back(pane.client);
    return clients;
}

void SplitScreenManager::setAnimationSpeed(float speed)
{
    mAnimSpeed = std::max(0.01f, std::min(1.0f, speed));
}

void SplitScreenManager::setGapPixels(uint32_t gap) { mGap = gap; }

// ── Private helpers ───────────────────────────────────────────────────────────

/**
 * Recompute the target rect for every pane based on the current layout,
 * split ratio, screen dimensions, and gap width.
 *
 * Layout illustrations (g = gap, r = mSplitRatio):
 *
 *  SideBySide          TopBottom
 *  ┌───────┬───────┐   ┌───────────────┐
 *  │       │       │   │     pane 0    │
 *  │pane 0 │pane 1 │   ├───────────────┤
 *  │       │       │   │     pane 1    │
 *  └───────┴───────┘   └───────────────┘
 *
 *  Quad                PictureInPicture
 *  ┌───────┬───────┐   ┌───────────────┐
 *  │pane 0 │pane 1 │   │               │
 *  ├───────┼───────┤   │    pane 0     │  ┌──────┐
 *  │pane 2 │pane 3 │   │               │  │pane 1│
 *  └───────┴───────┘   └───────────────┘  └──────┘
 */
void SplitScreenManager::computeTargetRects()
{
    const float W = static_cast<float>(mScreenW);
    const float H = static_cast<float>(mScreenH);
    const float g = static_cast<float>(mGap);
    const float r = mSplitRatio;
    const std::size_t n = mPanes.size();

    switch (mLayout)
    {
    // ── SideBySide ───────────────────────────────────────────────────────────
    case Layout::SideBySide:
    {
        const float leftW  = W * r - g * 0.5f;
        const float rightX = W * r + g * 0.5f;
        const float rightW = W - rightX;

        if (n > 0) mPanes[0].target = { 0.0f,  0.0f, leftW,  H };
        if (n > 1) mPanes[1].target = { rightX, 0.0f, rightW, H };
        // Extra panes share the right slot (rare; caller should use Quad).
        for (std::size_t i = 2; i < n; ++i)
            mPanes[i].target = (n > 1) ? mPanes[1].target
                                        : PaneRect{0.0f, 0.0f, W, H};
        break;
    }

    // ── TopBottom ────────────────────────────────────────────────────────────
    case Layout::TopBottom:
    {
        const float topH = H * r - g * 0.5f;
        const float botY = H * r + g * 0.5f;
        const float botH = H - botY;

        if (n > 0) mPanes[0].target = { 0.0f, 0.0f,  W, topH };
        if (n > 1) mPanes[1].target = { 0.0f, botY,  W, botH };
        for (std::size_t i = 2; i < n; ++i)
            mPanes[i].target = (n > 1) ? mPanes[1].target
                                        : PaneRect{0.0f, 0.0f, W, H};
        break;
    }

    // ── Quad ─────────────────────────────────────────────────────────────────
    case Layout::Quad:
    {
        const float hw = (W - g) * 0.5f; // half-width
        const float hh = (H - g) * 0.5f; // half-height
        const float rx = hw + g;          // right column x
        const float by = hh + g;          // bottom row y

        if (n > 0) mPanes[0].target = { 0.0f, 0.0f, hw, hh };
        if (n > 1) mPanes[1].target = { rx,   0.0f, hw, hh };
        if (n > 2) mPanes[2].target = { 0.0f, by,   hw, hh };
        if (n > 3) mPanes[3].target = { rx,   by,   hw, hh };
        break;
    }

    // ── PictureInPicture ─────────────────────────────────────────────────────
    case Layout::PictureInPicture:
    {
        // Main pane occupies the full screen.
        // PIP pane is 30 % wide × 30 % tall, placed at the bottom-right with
        // a 16-px margin.
        const float pipW   = W * 0.30f;
        const float pipH   = H * 0.30f;
        const float margin = 16.0f;

        if (n > 0) mPanes[0].target = { 0.0f,
                                         0.0f,
                                         W,
                                         H };
        if (n > 1) mPanes[1].target = { W - pipW - margin,
                                         H - pipH - margin,
                                         pipW,
                                         pipH };
        for (std::size_t i = 2; i < n; ++i)
            mPanes[i].target = (n > 1) ? mPanes[1].target
                                        : PaneRect{0.0f, 0.0f, W, H};
        break;
    }
    }
}

/**
 * Push the pane's current (interpolated) rect to the compositor.
 * Uses CompositorController::scaleToFit which calls
 *   setPosition(x, y) + setScale(w/naturalW, h/naturalH)
 * on the underlying RdkCompositor.
 */
void SplitScreenManager::applyRect(const Pane& pane) const
{
    const PaneRect& r = pane.current;
    if (r.w < 1.0f || r.h < 1.0f)
        return;

    CompositorController::scaleToFit(pane.client,
                                      static_cast<int32_t>(r.x),
                                      static_cast<int32_t>(r.y),
                                      static_cast<uint32_t>(r.w),
                                      static_cast<uint32_t>(r.h));
}

} // namespace RdkWindowManager
