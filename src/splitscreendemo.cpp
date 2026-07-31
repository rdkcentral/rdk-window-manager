/**
 * Split Screen Demo — sample application for the dynamic split-screen POC.
 *
 * Initialises the RDK Window Manager, registers four named compositor slots,
 * activates the SplitScreenManager in SideBySide mode, then enters an
 * interactive render loop.  All layout transitions and animated divider
 * movements are driven from the keyboard via stdin (raw/non-blocking).
 *
 * Usage
 *   ./splitscreendemo [width height]
 *   Default resolution: 1920 x 1080 (overridden by env or WM at run-time)
 *
 * Controls
 *   1   SideBySide layout
 *   2   TopBottom layout
 *   3   Quad layout (four panes)
 *   4   PictureInPicture layout
 *   [   Divider left / up   (ratio − 0.05)
 *   ]   Divider right / down (ratio + 0.05)
 *   f   Focus next pane
 *   s   Swap panes 0 and 1
 *   d   Deactivate split screen  (re-press to re-activate)
 *   q / ESC   Quit
 **/

#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <csignal>

#include <unistd.h>
#include <poll.h>
#include <termios.h>

#include "rdkwindowmanager.h"
#include "compositorcontroller.h"
#include "splitscreenmanager.h"
#include "logger.h"

// ── Demo constants ────────────────────────────────────────────────────────────

static const std::vector<std::string> kDemoClients = {
    "splitdemo_A",   // slot 0  (left / top / TL / main)
    "splitdemo_B",   // slot 1  (right / bottom / TR / PIP)
    "splitdemo_C",   // slot 2  (BL — used by Quad)
    "splitdemo_D"    // slot 3  (BR — used by Quad)
};

static constexpr int kFPS       = 40;
static constexpr int kFrameUs   = 1000000 / kFPS;

// ── Inter-thread command ──────────────────────────────────────────────────────

enum class Cmd
{
    None,
    Layout_SideBySide,
    Layout_TopBottom,
    Layout_Quad,
    Layout_PIP,
    RatioDecrease,
    RatioIncrease,
    FocusNext,
    SwapPanes,
    ToggleActive,
    Quit
};

static std::atomic<Cmd>  gCmd{Cmd::None};
static std::atomic<bool> gRunning{true};

// ── Signal handler ────────────────────────────────────────────────────────────

static void onSignal(int)
{
    gRunning = false;
    gCmd     = Cmd::Quit;
}

// ── Help banner ───────────────────────────────────────────────────────────────

static void printHelp()
{
    std::cout <<
        "\n"
        "╔══════════════════════════════════════╗\n"
        "║   RDK Split Screen Demo — Controls   ║\n"
        "╠══════════════════════════════════════╣\n"
        "║  1  SideBySide layout                ║\n"
        "║  2  TopBottom  layout                ║\n"
        "║  3  Quad       layout  (4 panes)     ║\n"
        "║  4  PiP        layout                ║\n"
        "║  [  Divider ← / ↑  (ratio − 0.05)   ║\n"
        "║  ]  Divider → / ↓  (ratio + 0.05)   ║\n"
        "║  f  Focus next pane                  ║\n"
        "║  s  Swap panes 0 ↔ 1                 ║\n"
        "║  d  Deactivate / re-activate          ║\n"
        "║  q  Quit                             ║\n"
        "╚══════════════════════════════════════╝\n"
        "\n";
}

// ── Input thread ──────────────────────────────────────────────────────────────

static void inputThreadFn()
{
    // Switch stdin to raw / non-blocking single-character mode.
    struct termios saved, raw;
    tcgetattr(STDIN_FILENO, &saved);
    raw = saved;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };

    while (gRunning)
    {
        if (poll(&pfd, 1, 100) <= 0)
            continue;

        char c = 0;
        if (read(STDIN_FILENO, &c, 1) != 1)
            continue;

        switch (c)
        {
        case '1': gCmd = Cmd::Layout_SideBySide; break;
        case '2': gCmd = Cmd::Layout_TopBottom;  break;
        case '3': gCmd = Cmd::Layout_Quad;       break;
        case '4': gCmd = Cmd::Layout_PIP;        break;
        case '[': gCmd = Cmd::RatioDecrease;     break;
        case ']': gCmd = Cmd::RatioIncrease;     break;
        case 'f': gCmd = Cmd::FocusNext;         break;
        case 's': gCmd = Cmd::SwapPanes;         break;
        case 'd': gCmd = Cmd::ToggleActive;      break;
        case 'q':
        case 27 /* ESC */:
            gCmd     = Cmd::Quit;
            gRunning = false;
            break;
        default:  break;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &saved);
}

// ── Command processor (called every frame) ────────────────────────────────────

static void processCmd(RdkWindowManager::SplitScreenManager& ss,
                       uint32_t screenW, uint32_t screenH,
                       int& focusedPane)
{
    using Layout = RdkWindowManager::SplitScreenManager::Layout;

    Cmd cmd = gCmd.exchange(Cmd::None);

    switch (cmd)
    {
    case Cmd::Layout_SideBySide:
        if (!ss.isActive())
            ss.activate({kDemoClients[0], kDemoClients[1]},
                        Layout::SideBySide, screenW, screenH);
        else
            ss.setLayout(Layout::SideBySide);
        std::cout << "[demo] Layout → SideBySide\n";
        break;

    case Cmd::Layout_TopBottom:
        if (!ss.isActive())
            ss.activate({kDemoClients[0], kDemoClients[1]},
                        Layout::TopBottom, screenW, screenH);
        else
            ss.setLayout(Layout::TopBottom);
        std::cout << "[demo] Layout → TopBottom\n";
        break;

    case Cmd::Layout_Quad:
        if (!ss.isActive())
            ss.activate(kDemoClients, Layout::Quad, screenW, screenH);
        else
            ss.setLayout(Layout::Quad);
        std::cout << "[demo] Layout → Quad\n";
        break;

    case Cmd::Layout_PIP:
        if (!ss.isActive())
            ss.activate({kDemoClients[0], kDemoClients[1]},
                        Layout::PictureInPicture, screenW, screenH);
        else
            ss.setLayout(Layout::PictureInPicture);
        std::cout << "[demo] Layout → PictureInPicture\n";
        break;

    case Cmd::RatioDecrease:
        if (ss.isActive())
        {
            ss.setSplitRatio(ss.splitRatio() - 0.05f);
            std::cout << "[demo] Split ratio → " << ss.splitRatio() << "\n";
        }
        break;

    case Cmd::RatioIncrease:
        if (ss.isActive())
        {
            ss.setSplitRatio(ss.splitRatio() + 0.05f);
            std::cout << "[demo] Split ratio → " << ss.splitRatio() << "\n";
        }
        break;

    case Cmd::FocusNext:
        if (ss.isActive())
        {
            focusedPane = (focusedPane + 1) % static_cast<int>(ss.paneCount());
            ss.focusPane(focusedPane);
            std::cout << "[demo] Focus → pane " << focusedPane << "\n";
        }
        break;

    case Cmd::SwapPanes:
        if (ss.isActive())
        {
            ss.swapPanes(0, 1);
            std::cout << "[demo] Swapped panes 0 ↔ 1\n";
        }
        break;

    case Cmd::ToggleActive:
        if (ss.isActive())
        {
            ss.deactivate();
            std::cout << "[demo] Deactivated — press 1-4 to re-activate\n";
        }
        else
        {
            ss.activate({kDemoClients[0], kDemoClients[1]},
                        Layout::SideBySide, screenW, screenH);
            std::cout << "[demo] Re-activated (SideBySide)\n";
        }
        break;

    case Cmd::Quit:
        gRunning = false;
        break;

    default:
        break;
    }
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    std::cout << "RDK Window Manager — Split Screen Demo\n";
    printHelp();

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    // Initialise the window manager (Essos, GL, Wayland socket, etc.)
    RdkWindowManager::initialize();

    // Resolve screen dimensions (WM may override via environment).
    uint32_t screenW = (argc >= 3) ? static_cast<uint32_t>(std::atoi(argv[1]))
                                   : 1920u;
    uint32_t screenH = (argc >= 3) ? static_cast<uint32_t>(std::atoi(argv[2]))
                                   : 1080u;

    uint32_t wmW = 0, wmH = 0;
    if (RdkWindowManager::CompositorController::getScreenResolution(wmW, wmH)
        && wmW > 0 && wmH > 0)
    {
        screenW = wmW;
        screenH = wmH;
    }
    std::cout << "[demo] Screen resolution: " << screenW << "x" << screenH << "\n";

    // Register a compositor display slot for each demo client.
    // Real Wayland applications would connect to these display names;
    // for this demo the slots exist so SplitScreenManager can position them.
    for (const auto& client : kDemoClients)
    {
        bool ok = RdkWindowManager::CompositorController::createDisplay(
            client, client,
            screenW, screenH,
            /*virtualDisplay=*/true,
            screenW, screenH,
            /*topmost=*/false,
            /*focus=*/false);

        std::cout << "[demo]   createDisplay('" << client << "'): "
                  << (ok ? "OK" : "FAILED") << "\n";
    }

    // Activate split screen — start with 2 panes, SideBySide.
    auto& ss = RdkWindowManager::SplitScreenManager::instance();
    ss.setAnimationSpeed(0.15f);  // ~280 ms settle at 40 fps
    ss.setGapPixels(4);

    ss.activate({kDemoClients[0], kDemoClients[1]},
                RdkWindowManager::SplitScreenManager::Layout::SideBySide,
                screenW, screenH);

    std::cout << "[demo] Split screen active.  Use the keys shown above.\n\n";

    // Start the input reader thread.
    std::thread inputThr(inputThreadFn);

    int focusedPane = 0;

    // ── Main render loop ─────────────────────────────────────────────────────
    //
    // Mirrors RdkWindowManager::run() but intercepts between update() and
    // draw() to process demo commands before the frame is rendered.
    //
    while (gRunning)
    {
        // 1. Process Wayland events, key-repeats, and advance the
        //    SplitScreenManager animation (wired inside update()).
        RdkWindowManager::update();

        // 2. Handle interactive demo commands from the input thread.
        processCmd(ss, screenW, screenH, focusedPane);

        // 3. Draw — calls EssosInstance::update() (swap previous frame),
        //    sets up the GL viewport, then composites all panes.
        RdkWindowManager::draw();

        // 4. Pace to the target frame rate.
        usleep(kFrameUs);
    }

    // ── Cleanup ──────────────────────────────────────────────────────────────

    inputThr.join();

    if (ss.isActive())
    {
        ss.setAnimationSpeed(1.0f);
        ss.deactivate();
        ss.update(); // settle immediately
    }

    for (const auto& client : kDemoClients)
        RdkWindowManager::CompositorController::kill(client);

    RdkWindowManager::deinitialize();

    std::cout << "[demo] Exited cleanly.\n";
    return 0;
}
