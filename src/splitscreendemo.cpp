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

#include <GLES2/gl2.h>

#include "rdkwindowmanager.h"
#include "compositorcontroller.h"
#include "splitscreenmanager.h"
#include "logger.h"

// ── GLES2 pane overlay renderer ──────────────────────────────────────────────
// The compositor slots created by createDisplay are Wayland surface containers.
// Without a real Wayland client app connected, those surfaces are empty.
// We draw coloured quads directly into the EGL framebuffer at the same
// positions the SplitScreenManager places each slot, giving a visual
// representation of the split-screen layout.

static GLuint gFlatShader  = 0;
static GLint  gFlatPosAttr = -1;
static GLint  gFlatColorUni= -1;
static GLint  gFlatResUni  = -1;

static const char* kFlatVert = R"GLSL(
    attribute vec2 aPos;
    uniform vec2 uRes;
    void main() {
        vec2 ndc = (aPos / uRes) * 2.0 - 1.0;
        gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    }
)GLSL";

static const char* kFlatFrag = R"GLSL(
    precision mediump float;
    uniform vec4 uColor;
    void main() { gl_FragColor = uColor; }
)GLSL";

static void initGLPainter()
{
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
            std::cerr << "[demo] shader compile failed (type=" << type << ")\n";
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER,   kFlatVert);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFlatFrag);
    gFlatShader = glCreateProgram();
    glAttachShader(gFlatShader, vs);
    glAttachShader(gFlatShader, fs);
    glLinkProgram(gFlatShader);
    glDeleteShader(vs);
    glDeleteShader(fs);
    gFlatPosAttr  = glGetAttribLocation (gFlatShader, "aPos");
    gFlatColorUni = glGetUniformLocation(gFlatShader, "uColor");
    gFlatResUni   = glGetUniformLocation(gFlatShader, "uRes");
    std::cout << "[demo] GL painter ready (shader=" << gFlatShader << ")\n";
}

// RGBA fill colours for each demo slot (semi-transparent).
static const float kPaneColors[4][4] = {
    { 0.80f, 0.20f, 0.20f, 0.55f },  // A – red
    { 0.20f, 0.40f, 0.90f, 0.55f },  // B – blue
    { 0.20f, 0.80f, 0.30f, 0.55f },  // C – green
    { 0.90f, 0.80f, 0.10f, 0.55f },  // D – yellow
};

static void drawQuadGL(float x, float y, float w, float h)
{
    float v[] = { x, y,  x+w, y,  x, y+h,  x+w, y+h };
    glVertexAttribPointer(gFlatPosAttr, 2, GL_FLOAT, GL_FALSE, 0, v);
    glEnableVertexAttribArray(gFlatPosAttr);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(gFlatPosAttr);
}

// Called every frame AFTER RdkWindowManager::draw() but BEFORE present().
//
// Receives only the clients that are currently active in the split-screen
// manager (from SplitScreenManager::getClients()) so that idle compositor
// slots do not render full-screen coloured quads on top of the layout.
//
// scaleToFit() positions compositors via setPosition() + setScale().
// getBounds() reads back position() and size() — but size() returns the
// ORIGINAL logical size (1920×1080 from createDisplay), not the scaled
// display size.  The correct visual width/height is:
//   visual_w = logical_w * scaleX     (scaleX = target_w / logical_w)
// So we call getScale() and multiply to recover the true on-screen rect.
//
// We also reset GL state that the WM compositor may leave dirty (depth
// test, scissor, FBO binding) so our 2-D quads always reach the display.
static void drawPaneOverlays(const std::vector<std::string>& clients,
                              uint32_t screenW, uint32_t screenH,
                              int focusedPane)
{
    if (gFlatShader == 0 || clients.empty()) return;

    // ── Reset GL state ───────────────────────────────────────────────────
    // The WM draw path may leave an FBO bound, depth test on, or scissor
    // active.  Reset everything we need for a simple 2-D overlay pass.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, static_cast<GLsizei>(screenW),
                     static_cast<GLsizei>(screenH));
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // ── Draw panes ───────────────────────────────────────────────────────
    glUseProgram(gFlatShader);
    glUniform2f(gFlatResUni, static_cast<float>(screenW),
                              static_cast<float>(screenH));
    glEnable(GL_BLEND);
    // Blend RGB with standard src-alpha, but keep dest alpha untouched (GL_ZERO,
    // GL_ONE).  Without this the alpha channel of the DRM framebuffer degrades
    // with every semi-transparent quad, which on RPi4 DRM/ARGB8888 planes causes
    // the plane to composite against black and appear black on the display.
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_ZERO,      GL_ONE);

    for (int i = 0; i < static_cast<int>(clients.size()); ++i)
    {
        // getBounds returns the logical position (correct) and the
        // original createDisplay size (needs scaling to get display size).
        uint32_t bx = 0, by = 0, logW = 0, logH = 0;
        if (!RdkWindowManager::CompositorController::getBounds(
                clients[i], bx, by, logW, logH))
            continue;

        // Recover actual on-screen size: visual = logical × scale.
        double sx = 1.0, sy = 1.0;
        RdkWindowManager::CompositorController::getScale(clients[i], sx, sy);
        auto bw = static_cast<uint32_t>(logW * sx);
        auto bh = static_cast<uint32_t>(logH * sy);

        if (bw < 4 || bh < 4) continue;  // skip if not yet animated in

        const float* c = kPaneColors[i % 4];
        float alpha = (i == focusedPane) ? 0.85f : c[3];
        glUniform4f(gFlatColorUni, c[0], c[1], c[2], alpha);
        drawQuadGL(static_cast<float>(bx), static_cast<float>(by),
                   static_cast<float>(bw), static_cast<float>(bh));

        // White border on the focused pane.
        if (i == focusedPane)
        {
            constexpr float B = 6.0f;
            glUniform4f(gFlatColorUni, 1.0f, 1.0f, 1.0f, 0.9f);
            drawQuadGL(static_cast<float>(bx),        static_cast<float>(by),
                       static_cast<float>(bw), B);                       // top
            drawQuadGL(static_cast<float>(bx),        static_cast<float>(by + bh) - B,
                       static_cast<float>(bw), B);                       // bottom
            drawQuadGL(static_cast<float>(bx),        static_cast<float>(by),
                       B, static_cast<float>(bh));                       // left
            drawQuadGL(static_cast<float>(bx + bw) - B, static_cast<float>(by),
                       B, static_cast<float>(bh));                       // right
        }
    }

    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);  // restore WM pre-multiplied convention
    glUseProgram(0);
}

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

    // Compile the GLES2 overlay shader (GL context is live after initialize).
    initGLPainter();

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
    // Each iteration:
    //   1. Advance the split-screen animation and process input events.
    //   2. Process interactive demo commands.
    //   3. Clear + composite all pane FBOs into the back buffer.
    //   4. Draw the coloured pane overlay so the layout is visible even
    //      without real Wayland client applications connected.
    //   5. Swap the back buffer to the display and run the Essos event loop.
    //
    // Keeping the swap (present) at the END of the frame ensures the overlay
    // drawn in step 4 is always visible — placing it at the start of draw()
    // would defer the overlay by one frame and can cause a blank screen on
    // DRM platforms where the very first page-flip after a mode change
    // presents an uninitialised buffer.
    //
    while (gRunning)
    {
        // 1. Advance animation and process Wayland/key events.
        RdkWindowManager::update();

        // 2. Handle interactive demo commands from the input thread.
        processCmd(ss, screenW, screenH, focusedPane);

        // 3. Clear back buffer and composite all pane FBOs.
        RdkWindowManager::draw();

        // 4. Overlay coloured quads so the pane layout is visible even
        //    without real Wayland client applications connected.
        //    Only draw panes that are currently managed by the split-screen
        //    manager so inactive slots don't cover the active layout.
        {
            std::vector<std::string> activeClients = ss.getClients();
            drawPaneOverlays(activeClients, screenW, screenH,
                             ss.isActive() ? ss.focusedPane() : focusedPane);
        }

        // 5. Swap the back buffer (which now contains composited FBOs +
        //    overlay) to the display and pump the Wayland event loop.
        RdkWindowManager::present();

        // 6. Pace to the target frame rate.
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
