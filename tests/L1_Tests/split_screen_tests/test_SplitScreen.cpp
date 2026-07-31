/**
 * L1 Unit Tests — Dynamic Split-Screen Manager POC
 *
 * Strategy
 * ────────
 * SplitScreenManager calls only two CompositorController APIs:
 *   • setFocus(client)
 *   • scaleToFit(client, x, y, w, h)
 *
 * Both are intercepted via the existing MockCompositorControllerImpl /
 * CompositorController::setImpl() injection point, so no hardware
 * (Wayland, EGL, Westeros) is required.
 *
 * Animation speed is forced to 1.0 in every test so that a single
 * update() call snaps all panes to their target rects, making expected
 * argument values deterministic.
 *
 * Expected rect values (gap = 4 px, screen = 1920 × 1080, ratio = 0.5)
 * ─────────────────────────────────────────────────────────────────────
 *  SideBySide  : left  {0,   0, 958, 1080}  right {962, 0, 958, 1080}
 *  TopBottom   : top   {0,   0, 1920,  538}  bot  {0, 542, 1920,  538}
 *  Quad        : TL {0,0,958,538} TR {962,0,958,538}
 *                BL {0,542,958,538} BR {962,542,958,538}
 *  PIP         : main {0,0,1920,1080}  pip {1328,740,576,324}
 **/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "splitscreenmanager.h"
#include "compositorcontrollerMock.h"

using namespace RdkWindowManager;
using namespace testing;

// ── Convenience ──────────────────────────────────────────────────────────────

static constexpr uint32_t W = 1920;
static constexpr uint32_t H = 1080;

// ── Test fixture ─────────────────────────────────────────────────────────────

class SplitScreenTest : public ::testing::Test
{
protected:
    NiceMock<MockCompositorControllerImpl> mMock;

    void SetUp() override
    {
        CompositorController::setImpl(&mMock);
        auto& ss = SplitScreenManager::instance();
        ss.setAnimationSpeed(1.0f); // instant convergence: one update() = settled
        ss.setGapPixels(4);
    }

    void TearDown() override
    {
        // Leave the singleton in a clean idle state regardless of test outcome.
        auto& ss = SplitScreenManager::instance();
        ss.setAnimationSpeed(1.0f);
        if (ss.isActive())
        {
            ss.deactivate();
            ss.update(); // snaps all panes to full-screen, sets mActive = false
        }
        CompositorController::setImpl(nullptr);
    }

    // Helper: activate with instant-settle and advance one frame.
    void activateAndSettle(const std::vector<std::string>& clients,
                           SplitScreenManager::Layout layout)
    {
        SplitScreenManager::instance().activate(clients, layout, W, H);
        SplitScreenManager::instance().update();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Activation
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, Activate_EmptyClientList_ReturnsFalse)
{
    EXPECT_FALSE(SplitScreenManager::instance()
                     .activate({}, SplitScreenManager::Layout::SideBySide, W, H));
    EXPECT_FALSE(SplitScreenManager::instance().isActive());
}

TEST_F(SplitScreenTest, Activate_ValidClients_IsActiveAndPaneCountCorrect)
{
    EXPECT_CALL(mMock, setFocus("appA")).Times(1);
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    EXPECT_TRUE(SplitScreenManager::instance()
                    .activate({"appA", "appB"}, SplitScreenManager::Layout::SideBySide, W, H));

    EXPECT_TRUE(SplitScreenManager::instance().isActive());
    EXPECT_EQ(2u, SplitScreenManager::instance().paneCount());
}

TEST_F(SplitScreenTest, Activate_SingleClient_ActivatesWithOnePane)
{
    EXPECT_CALL(mMock, setFocus("solo")).Times(1);
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    EXPECT_TRUE(SplitScreenManager::instance()
                    .activate({"solo"}, SplitScreenManager::Layout::SideBySide, W, H));
    EXPECT_EQ(1u, SplitScreenManager::instance().paneCount());
}

TEST_F(SplitScreenTest, Activate_FocusesFirstPaneAutomatically)
{
    EXPECT_CALL(mMock, setFocus("first")).Times(1);
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"first", "second"}, SplitScreenManager::Layout::SideBySide, W, H);

    EXPECT_EQ(0, SplitScreenManager::instance().focusedPane());
}

TEST_F(SplitScreenTest, Activate_ReportsCorrectInitialLayout)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::TopBottom, W, H);

    EXPECT_EQ(SplitScreenManager::Layout::TopBottom,
              SplitScreenManager::instance().currentLayout());
}

// ═══════════════════════════════════════════════════════════════════════════
// SideBySide layout
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, SideBySide_DefaultRatio_CorrectPaneRects)
{
    // gap=4, ratio=0.5
    // leftW  = 1920×0.5 − 2 = 958
    // rightX = 1920×0.5 + 2 = 962,  rightW = 1920−962 = 958
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit("appA",   0, 0, 958u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("appB", 962, 0, 958u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    activateAndSettle({"appA", "appB"}, SplitScreenManager::Layout::SideBySide);
}

TEST_F(SplitScreenTest, SideBySide_CustomRatio_AdjustsPaneWidths)
{
    // ratio=0.6: leftW=1150, rightX=1154, rightW=766
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"appA", "appB"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().setSplitRatio(0.6f);

    EXPECT_CALL(mMock, scaleToFit("appA",    0, 0, 1150u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("appB", 1154, 0,  766u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    SplitScreenManager::instance().update();
}

// ═══════════════════════════════════════════════════════════════════════════
// TopBottom layout
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, TopBottom_DefaultRatio_CorrectPaneRects)
{
    // gap=4, ratio=0.5
    // topH = 1080×0.5 − 2 = 538
    // botY = 1080×0.5 + 2 = 542,  botH = 1080−542 = 538
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit("appA", 0,   0, 1920u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("appB", 0, 542, 1920u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    activateAndSettle({"appA", "appB"}, SplitScreenManager::Layout::TopBottom);
}

TEST_F(SplitScreenTest, TopBottom_CustomRatio_AdjustsPaneHeights)
{
    // ratio=0.4: topH=430, botY=434, botH=646
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::TopBottom, W, H);
    SplitScreenManager::instance().setSplitRatio(0.4f);

    EXPECT_CALL(mMock, scaleToFit("a", 0,   0, 1920u, 430u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("b", 0, 434, 1920u, 646u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    SplitScreenManager::instance().update();
}

// ═══════════════════════════════════════════════════════════════════════════
// Quad layout
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, Quad_FourClients_CorrectQuadrantRects)
{
    // hw = (1920−4)×0.5 = 958,  hh = (1080−4)×0.5 = 538
    // rx = 962, by = 542
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit("tl",   0,   0, 958u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("tr", 962,   0, 958u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("bl",   0, 542, 958u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("br", 962, 542, 958u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    activateAndSettle({"tl", "tr", "bl", "br"}, SplitScreenManager::Layout::Quad);
}

TEST_F(SplitScreenTest, Quad_TwoClients_OnlyTwoPanesCreated)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::Quad, W, H);

    EXPECT_EQ(2u, SplitScreenManager::instance().paneCount());
}

// ═══════════════════════════════════════════════════════════════════════════
// PictureInPicture layout
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, PIP_MainIsFullScreen_PIPIsBottomRightOverlay)
{
    // pipW = 1920×0.3 = 576,  pipH = 1080×0.3 = 324,  margin = 16
    // pip x = 1920−576−16 = 1328,  pip y = 1080−324−16 = 740
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit("main",    0,   0, 1920u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("pip",  1328, 740,  576u,  324u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    activateAndSettle({"main", "pip"}, SplitScreenManager::Layout::PictureInPicture);
}

// ═══════════════════════════════════════════════════════════════════════════
// Split ratio
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, SetSplitRatio_WhenInactive_ReturnsFalse)
{
    EXPECT_FALSE(SplitScreenManager::instance().setSplitRatio(0.4f));
}

TEST_F(SplitScreenTest, SetSplitRatio_BelowMinimum_ClampsTo005)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().setSplitRatio(-5.0f);

    EXPECT_FLOAT_EQ(0.05f, SplitScreenManager::instance().splitRatio());
}

TEST_F(SplitScreenTest, SetSplitRatio_AboveMaximum_ClampsTo095)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().setSplitRatio(100.0f);

    EXPECT_FLOAT_EQ(0.95f, SplitScreenManager::instance().splitRatio());
}

TEST_F(SplitScreenTest, SetSplitRatio_ValidValue_IsStoredExactly)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().setSplitRatio(0.3f);

    EXPECT_FLOAT_EQ(0.3f, SplitScreenManager::instance().splitRatio());
}

// ═══════════════════════════════════════════════════════════════════════════
// Focus management
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, FocusPane_WhenInactive_ReturnsFalse)
{
    EXPECT_FALSE(SplitScreenManager::instance().focusPane(0));
}

TEST_F(SplitScreenTest, FocusPane_ValidIndex_CallsSetFocusWithCorrectClient)
{
    EXPECT_CALL(mMock, setFocus("appA")).Times(1); // on activate
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"appA", "appB"}, SplitScreenManager::Layout::SideBySide, W, H);

    EXPECT_CALL(mMock, setFocus("appB")).Times(1);

    EXPECT_TRUE(SplitScreenManager::instance().focusPane(1));
    EXPECT_EQ(1, SplitScreenManager::instance().focusedPane());
}

TEST_F(SplitScreenTest, FocusPane_NegativeIndex_ReturnsFalse)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);

    EXPECT_FALSE(SplitScreenManager::instance().focusPane(-1));
}

TEST_F(SplitScreenTest, FocusPane_IndexOutOfRange_ReturnsFalse)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);

    EXPECT_FALSE(SplitScreenManager::instance().focusPane(10));
}

// ═══════════════════════════════════════════════════════════════════════════
// Pane swapping
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, SwapPanes_WhenInactive_ReturnsFalse)
{
    EXPECT_FALSE(SplitScreenManager::instance().swapPanes(0, 1));
}

TEST_F(SplitScreenTest, SwapPanes_SameIndex_ReturnsTrueNoPositionChange)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);

    EXPECT_TRUE(SplitScreenManager::instance().swapPanes(0, 0));
}

TEST_F(SplitScreenTest, SwapPanes_TwoPanes_ClientsAppearInSwappedSlots)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"appA", "appB"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().update(); // settle initial layout

    EXPECT_TRUE(SplitScreenManager::instance().swapPanes(0, 1));

    // After swap slot 0 holds "appB" (left pane), slot 1 holds "appA" (right pane)
    EXPECT_CALL(mMock, scaleToFit("appB",   0, 0, 958u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("appA", 962, 0, 958u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    SplitScreenManager::instance().update();
}

TEST_F(SplitScreenTest, SwapPanes_InvalidIndex_ReturnsFalse)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);

    EXPECT_FALSE(SplitScreenManager::instance().swapPanes(0, 99));
    EXPECT_FALSE(SplitScreenManager::instance().swapPanes(-1, 0));
}

// ═══════════════════════════════════════════════════════════════════════════
// Layout switching
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, SetLayout_WhenInactive_ReturnsFalse)
{
    EXPECT_FALSE(SplitScreenManager::instance()
                     .setLayout(SplitScreenManager::Layout::Quad));
}

TEST_F(SplitScreenTest, SetLayout_WhileActive_UpdatesCurrentLayout)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().update();

    EXPECT_TRUE(SplitScreenManager::instance()
                    .setLayout(SplitScreenManager::Layout::TopBottom));
    EXPECT_EQ(SplitScreenManager::Layout::TopBottom,
              SplitScreenManager::instance().currentLayout());
}

TEST_F(SplitScreenTest, SetLayout_SideBySideToTopBottom_RecalculatesRects)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().update();
    SplitScreenManager::instance().setLayout(SplitScreenManager::Layout::TopBottom);

    EXPECT_CALL(mMock, scaleToFit("a", 0,   0, 1920u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("b", 0, 542, 1920u, 538u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    SplitScreenManager::instance().update();
}

// ═══════════════════════════════════════════════════════════════════════════
// Deactivation
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, Deactivate_WhenInactive_ReturnsFalse)
{
    EXPECT_FALSE(SplitScreenManager::instance().deactivate());
}

TEST_F(SplitScreenTest, Deactivate_AnimatesToFullScreen_ThenBecomesInactive)
{
    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).WillRepeatedly(Return(true));

    SplitScreenManager::instance()
        .activate({"a", "b"}, SplitScreenManager::Layout::SideBySide, W, H);
    SplitScreenManager::instance().update();

    EXPECT_TRUE(SplitScreenManager::instance().deactivate());
    EXPECT_TRUE(SplitScreenManager::instance().isActive()); // still animating

    // Final update: both panes snap to full-screen; singleton becomes inactive
    EXPECT_CALL(mMock, scaleToFit("a", 0, 0, 1920u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(mMock, scaleToFit("b", 0, 0, 1920u, 1080u))
        .Times(AtLeast(1)).WillRepeatedly(Return(true));

    SplitScreenManager::instance().update();
    EXPECT_FALSE(SplitScreenManager::instance().isActive());
}

// ═══════════════════════════════════════════════════════════════════════════
// Animation convergence
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, AnimationConverges_SlowSpeed_SettlesAfterManyFrames)
{
    // Use a realistic animation speed and verify final position matches target
    // after enough frames.  We capture the last scaleToFit call per client.
    SplitScreenManager::instance().setAnimationSpeed(0.12f); // realistic speed

    struct CapturedRect { int32_t x, y; uint32_t w, h; };
    CapturedRect lastA{}, lastB{};

    ON_CALL(mMock, scaleToFit("appA", _, _, _, _))
        .WillByDefault([&](const std::string&, int32_t x, int32_t y,
                           uint32_t w, uint32_t h) {
            lastA = {x, y, w, h};
            return true;
        });
    ON_CALL(mMock, scaleToFit("appB", _, _, _, _))
        .WillByDefault([&](const std::string&, int32_t x, int32_t y,
                           uint32_t w, uint32_t h) {
            lastB = {x, y, w, h};
            return true;
        });

    EXPECT_CALL(mMock, setFocus(_)).Times(AnyNumber());

    SplitScreenManager::instance()
        .activate({"appA", "appB"}, SplitScreenManager::Layout::SideBySide, W, H);

    for (int i = 0; i < 200; ++i)
        SplitScreenManager::instance().update();

    // After 200 frames at speed 0.12, must have converged (tolerance ±1 px)
    EXPECT_EQ(0,    lastA.x);
    EXPECT_EQ(0,    lastA.y);
    EXPECT_EQ(958u, lastA.w);
    EXPECT_EQ(1080u, lastA.h);

    EXPECT_EQ(962,   lastB.x);
    EXPECT_EQ(0,     lastB.y);
    EXPECT_EQ(958u,  lastB.w);
    EXPECT_EQ(1080u, lastB.h);
}

// ═══════════════════════════════════════════════════════════════════════════
// Guard: update() when inactive is a no-op
// ═══════════════════════════════════════════════════════════════════════════

TEST_F(SplitScreenTest, Update_WhenNotActive_NeverCallsScaleToFit)
{
    EXPECT_CALL(mMock, scaleToFit(_, _, _, _, _)).Times(0);
    SplitScreenManager::instance().update();
}
