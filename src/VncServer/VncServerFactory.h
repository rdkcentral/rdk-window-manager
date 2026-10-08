/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#ifndef VNCSERVER_FACTORY_H
#define VNCSERVER_FACTORY_H

#include <cstdint>

namespace RdkWindowManager
{
    /**
     * @class VncServerFactory
     * @brief Factory to manage VNC server initialization based on build configuration.
     *
     * Encapsulates the decision logic for starting either the internal VNC server
     * (listening on port 5900 directly) or the VNCServer2 bridge mode (connecting
     * to an external VNCServer2 on port 9998).
     *
     * This class separates VNCServer2-specific logic from the core VncServer class,
     * eliminating the need for scattered #ifdef directives.
     */
    class VncServerFactory
    {
    public:
        /**
         * @brief Get singleton instance of the factory.
         * @return Reference to the VncServerFactory singleton.
         */
        static VncServerFactory& getInstance();

        /**
         * @brief Initialize and start the appropriate VNC server.
         *
         * Based on build configuration (ENABLE_RDKWINDOWMANAGER_VNCSERVER2),
         * this method will either start the internal VNC server or the bridge server.
         *
         * @param width  Display width for framebuffer
         * @param height Display height for framebuffer
         * @return true if server started successfully, false otherwise.
         */
        bool initializeVncServer(uint32_t width, uint32_t height);

        /**
         * @brief Stop the running VNC server.
         *
         * Stops whichever server mode was started (internal or bridge).
         */
        void stopVncServer();

    private:
        VncServerFactory() = default;
        ~VncServerFactory() = default;

        // Prevent copying
        VncServerFactory(const VncServerFactory&) = delete;
        VncServerFactory& operator=(const VncServerFactory&) = delete;

#ifdef ENABLE_RDKWINDOWMANAGER_VNCSERVER2
        /**
         * @brief Initialize bridge mode (VNCServer2 integration).
         *
         * Starts VncBridgeServer which connects to external VNCServer2.
         * No internal listener on port 5900.
         *
         * @param width  Display width for framebuffer
         * @param height Display height for framebuffer
         * @return true if bridge initialized successfully, false otherwise.
         */
        bool initializeBridgeMode(uint32_t width, uint32_t height);
#else
        /**
         * @brief Initialize internal server mode.
         *
         * Starts internal VNC server listening on port 5900.
         *
         * @param width  Display width for framebuffer
         * @param height Display height for framebuffer
         * @return true if internal server initialized successfully, false otherwise.
         */
        bool initializeInternalServerMode(uint32_t width, uint32_t height);
#endif

        bool mInitialized = false;
    };

} // namespace RdkWindowManager

#endif // VNCSERVER_FACTORY_H
