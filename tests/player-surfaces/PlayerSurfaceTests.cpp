#include <wayland-client.h>
#include "../../src/compositorcontroller.cpp"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <limits>

namespace RdkWindowManager
{
    class LogCapture
    {
    public:
        static unsigned count;
    };
    unsigned LogCapture::count = 0;

    double seconds() { return 0.0; }
    void Logger::log(LogLevel, const char*, ...) { ++LogCapture::count; }

    EssosInstance* EssosInstance::instance()
    {
        static EssosInstance instance;
        return &instance;
    }
    EssosInstance::EssosInstance() {}
    EssosInstance::~EssosInstance() {}
    void EssosInstance::resolution(uint32_t& width, uint32_t& height)
    {
        width = 1920;
        height = 1080;
    }

    RdkCompositor::RdkCompositor()
        : mWidth(1920), mHeight(1080), mLogicalWidth(1920), mLogicalHeight(1080),
          mPositionX(0), mPositionY(0), mOpacity(1.0), mVisible(true),
          mScaleX(1.0), mScaleY(1.0), mCropX(0), mCropY(0), mCropWidth(0), mCropHeight(0)
    {}
    RdkCompositor::~RdkCompositor() {}
    void RdkCompositor::displayName(std::string& name) const { name = mDisplayName; }
    void RdkCompositor::setVisible(bool visible) { mVisible = visible; }
    void RdkCompositor::setFocused(bool focused) { mFocused = focused; }
    bool RdkCompositor::getSurfaceInfo(int, FireboltSurfaceInfo& info)
    {
        info.surfaceType = SurfaceType::Notification;
        return true;
    }
    bool RdkCompositor::setFireboltSurfaceVisibility(int, bool) { return true; }
    bool RdkCompositor::fireboltSurfaceDestroy(int) { return true; }
    bool RdkCompositor::isKeyPressed() { return false; }
    void RdkCompositor::visible(bool& visible) { visible = mVisible; }
    void RdkCompositor::setPosition(int32_t x, int32_t y) { mPositionX = x; mPositionY = y; }
    void RdkCompositor::position(int32_t& x, int32_t& y) { x = mPositionX; y = mPositionY; }
    void RdkCompositor::size(uint32_t& width, uint32_t& height) { width = mWidth; height = mHeight; }
    void RdkCompositor::logicalSize(uint32_t& width, uint32_t& height) { width = mLogicalWidth; height = mLogicalHeight; }
    void RdkCompositor::setLogicalSize(uint32_t width, uint32_t height) { mLogicalWidth = width; mLogicalHeight = height; }
    void RdkCompositor::setOpacity(double opacity) { mOpacity = opacity; }
    void RdkCompositor::opacity(double& opacity) { opacity = mOpacity; }
    void RdkCompositor::setScale(double scaleX, double scaleY) { mScaleX = scaleX; mScaleY = scaleY; }
    void RdkCompositor::scale(double& scaleX, double& scaleY) { scaleX = mScaleX; scaleY = mScaleY; }
    void RdkCompositor::setCrop(int32_t x, int32_t y, int32_t width, int32_t height)
    {
        mCropX = x; mCropY = y; mCropWidth = width; mCropHeight = height;
        mScaleX = static_cast<double>(width) / mWidth;
        mScaleY = static_cast<double>(height) / mHeight;
    }
    void RdkCompositor::crop(int32_t& x, int32_t& y, int32_t& width, int32_t& height)
    { x = mCropX; y = mCropY; width = mCropWidth; height = mCropHeight; }

    class PlayerSurfaceTests
    {
        class Compositor : public RdkCompositor
        {
        public:
            explicit Compositor(const std::string& name) { mDisplayName = name; }
            void setNaturalSize(uint32_t width, uint32_t height) { mWidth = width; mHeight = height; }
            bool focused() const { return mFocused; }
        private:
            bool createDisplay(const std::string&, const std::string&, uint32_t, uint32_t,
                bool, uint32_t, uint32_t, int32_t, int32_t, const std::string&) override { return true; }
        };

        static std::shared_ptr<RdkCompositor> addDisplay(const std::string& name, int32_t zorder = 0)
        {
            CompositorInfo info;
            info.name = name;
            info.zorder = zorder;
            info.compositor = std::make_shared<Compositor>(name);
            gCompositorList.push_back(info);
            return info.compositor;
        }

    public:
        static int run()
        {
            auto home = addDisplay("home", 120);
            auto app = addDisplay("app");
            auto player = addDisplay("westeros-asplayer-0");
            addDisplay("westeros-asplayer-1");
            auto subtitles = addDisplay("westeros-asplayer-subtitles", 1000);
            assert(CompositorController::setFocus("home"));
            assert(CompositorController::setFireboltSurfaceVisibility("app", 6, true));
            assert(!std::static_pointer_cast<Compositor>(home)->focused());
            assert(std::static_pointer_cast<Compositor>(app)->focused());
            assert(CompositorController::fireboltSurfaceDestroy("app", 6));
            assert(std::static_pointer_cast<Compositor>(home)->focused());
            assert(!std::static_pointer_cast<Compositor>(app)->focused());
            assert("home" == CompositorController::getPlayerOwner(0));
            assert(!CompositorController::takeAutomaticPlayerOwnership("app"));
            CompositorController::PlayerSurfaceConfig config;
            config.visible = true;
            assert(CompositorController::configurePlayerSurface("HOME", 0, config));
            assert(CompositorController::takePlayerOwnership("HOME", 1));
            assert(CompositorController::setFocus("app"));
            assert("home" == CompositorController::getPlayerOwner(0));
            CompositorController::updatePlayerSurfaces();
            assert(!CompositorController::getSubtitlesMuted());

            ClientInfo playerInfo{}, subtitleInfo{}, ownerInfo{};
            assert(CompositorController::getClientInfo("home", ownerInfo));
            assert(CompositorController::getClientInfo("westeros-asplayer-0", playerInfo));
            assert(CompositorController::getClientInfo("westeros-asplayer-subtitles", subtitleInfo));
            assert(playerInfo.zorder < ownerInfo.zorder);
            assert(subtitleInfo.zorder > playerInfo.zorder);
            assert(subtitleInfo.zorder < ownerInfo.zorder);
            const auto logsBeforeNoOp = LogCapture::count;
            for (unsigned repeat = 0; repeat < 60; ++repeat)
            {
                assert(CompositorController::setZorder("westeros-asplayer-0", playerInfo.zorder));
                assert(CompositorController::setZorder("westeros-asplayer-subtitles", subtitleInfo.zorder));
            }
            assert(logsBeforeNoOp == LogCapture::count);
            config.zorder = 1;
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getClientInfo("westeros-asplayer-0", playerInfo));
            assert(CompositorController::getClientInfo("westeros-asplayer-subtitles", subtitleInfo));
            assert(playerInfo.zorder > ownerInfo.zorder);
            assert(subtitleInfo.zorder > playerInfo.zorder);
            assert(playerInfo.zorder < kOverlayZOrderThreshold);
            assert(subtitleInfo.zorder >= kOverlayZOrderThreshold);
            config.zorder = 0;

            config.subtitlesMuted = true;
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            CompositorController::updatePlayerSurfaces();
            bool visible = true;
            subtitles->visible(visible);
            assert(!visible && CompositorController::getSubtitlesMuted());

            assert(CompositorController::configurePlayerSurface("home", 1, config));
            assert(CompositorController::takePlayerOwnership("home", 2));

            config.subtitlesMuted = false;
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            home->setPosition(120, 80);
            home->setLogicalSize(480, 270);
            CompositorController::updatePlayerSurfaces();
            double scaleX = 0.0, scaleY = 0.0;
            subtitles->scale(scaleX, scaleY);
            assert(0.25 == scaleX && 0.25 == scaleY);
            player->scale(scaleX, scaleY);
            assert(0.25 == scaleX && 0.25 == scaleY);
            int32_t outputX = 0, outputY = 0;
            subtitles->position(outputX, outputY);
            assert(120 == outputX && 80 == outputY);

            config.x = 96;
            config.y = 48;
            config.width = 960;
            config.height = 540;
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            CompositorController::updatePlayerSurfaces();
            subtitles->position(outputX, outputY);
            subtitles->scale(scaleX, scaleY);
            assert(144 == outputX && 92 == outputY);
            assert(0.125 == scaleX && 0.125 == scaleY);
            assert(!CompositorController::getSubtitlesMuted());
            config.visible = false;
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getSubtitlesMuted());
            config.visible = true;
            config.x = 0;
            config.y = 0;

            std::static_pointer_cast<Compositor>(home)->setNaturalSize(1280, 720);
            config.width = 1280;
            config.height = 720;
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            CompositorController::updatePlayerSurfaces();
            subtitles->scale(scaleX, scaleY);
            assert(0.25 == scaleX && 0.25 == scaleY);
            home->setOpacity(0.0);
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getSubtitlesMuted());
            home->setOpacity(1.0);

            home->setVisible(false);
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getSubtitlesMuted());
            home->setVisible(true);
            assert(CompositorController::takePlayerOwnership("app", 1));
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getSubtitlesMuted());
            assert(CompositorController::configurePlayerSurface("home", 0, config));
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getSubtitlesMuted());

            assert(CompositorController::takePlayerOwnership("home", 1));
            CompositorController::updatePlayerSurfaces();
            assert(!CompositorController::getSubtitlesMuted());
            CompositorController::releasePlayerOwnership("HOME");
            CompositorController::updatePlayerSurfaces();
            assert(CompositorController::getSubtitlesMuted());
            assert("app" == CompositorController::getPlayerOwner(0));
            CompositorController::releasePlayerOwnership("app");
            assert(CompositorController::getPlayerOwner(0).empty());
            assert(!CompositorController::configurePlayerSurface("home", 2, config));
            assert(!CompositorController::takePlayerOwnership("home", 4));
            assert(!CompositorController::takePlayerOwnership("unknown", 1));
            assert(!CompositorController::configurePlayerSurface("unknown", 0, config));
            config.cropWidth = std::numeric_limits<double>::quiet_NaN();
            assert(!CompositorController::configurePlayerSurface("home", 0, config));
            std::puts("Player surface caption regression checks passed");
            return 0;
        }
    };
}

int main()
{
    return RdkWindowManager::PlayerSurfaceTests::run();
}