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
 *
 * Uses some sample wayland code which is:
 * Copyright (c) 2011 Benjamin Franzke
 * Licensed under the MIT License
 **/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <poll.h>
#include <errno.h>
#include <mqueue.h>
#include <string.h>
#include <setjmp.h>
#include <vector>
#include <map>
#include <string>
#include <curl/curl.h>
#include <cstring>
#include <string>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <regex>
#include <string>
#include <iostream>

#include <semaphore.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include "testrdkwm.h"
#ifdef RDK_WINDOW_MANAGER_LOGGER
#include "testlogmonitor.h"
#endif

/* X11/Xlib.h conflicting with Bool datatype, to overcome this issue, added below workaround */
#if defined(RAPIDJSON_WRITER_H_) && defined (Bool)
#undef Bool
#endif /* RAPIDJSON_WRITER_H_ && Bool */

#define handle_error(msg)                       do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define RDKWM_TESTAPP_NAME                      "rdkwmtestapp"
#define RDKWM_TEST_MESSAGEQUEUE_NAME            "/RdkWmTestApp_msgq"
#define RDKWM_TESTAPP_REPORT_PATH               "/opt/logs/rdkwmtest"
#define RED_SIZE                (8)
#define GREEN_SIZE              (8)
#define BLUE_SIZE               (8)
#define ALPHA_SIZE              (8)
#define DEPTH_SIZE              (0)

#define RDKWM_TEST_RESOLUTION_DEFAULT_DISPLAY_WIDTH     1280
#define RDKWM_TEST_RESOLUTION_DEFAULT_DISPLAY_HEIGHT    720

#define RDKWM_TEST_RESOLUTION_DEFAULT_SURFACE_WIDTH     1920
#define RDKWM_TEST_RESOLUTION_DEFAULT_SURFACE_HEIGHT    1080

#define RDKWM_TEST_DISPLAY_DEFAULT_VALUE     0
#define RDKWM_TEST_ENABLE_VIRTUAL_DISPLAY    1

#define FIREBOLT_SHELL_FIREBOLT_SURFACEID_STANDARD   1
#define RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS           200
#define RDKWM_TEST_DEFAULT_WAITTIME             (10)

#define RDKWM_TEST_ENABLE_VISIBLITY         1
#define RDKWM_TEST_DISABLE_VISIBLITY        0

#define RDKWM_TEST_SECURITY_TOKEN_SIZE      (1024)
#define RDKWM_TEST_MAX_CURL_RETRIES         (3)

#define RDKWM_TEST_JSON_RPC_URL             "http://127.0.0.1:9998/jsonrpc"
#define RDKWM_TEST_ACTIVATE_METHOD          "Controller.1.activate"
#define RDKWM_TEST_ACTIVATE_CALLSIGN        "org.rdk.RDKWindowManager"
#define RDKWM_TEST_ACTIVATE_REQUEST_ID      (4)

#define RDKWM_TEST_CREATEDISPLAY_METHOD     "org.rdk.RDKWindowManager.1.createDisplay"
#define RDKWM_TEST_CREATEDISPLAY_CALLSIGN     RDKWM_TESTAPP_NAME
#define RDKWM_TEST_CREATEDISPLAY_REQUEST_ID (3)

#define RDKWM_TEST_GETAPPS                  "org.rdk.RDKWindowManager.1.getApps"
#define RDKWM_TEST_GETAPPS_REQUEST_ID       (3)

#define RDKWM_TEST_NSEC_PER_MILLISEC        (1000u * 1000u)
#define RDKWM_TEST_MILLISEC_PER_SECOND      (1000u)
#define RDKWM_TEST_NSEC_PER_SECOND          (RDKWM_TEST_NSEC_PER_MILLISEC * RDKWM_TEST_MILLISEC_PER_SECOND)

//#define RDKWM_TEST_DEBUG_TEST_SELECTION

static void wmTestDestroyContext(RdkWmTestAppCtx *ctx);
static void wmTestInitialiseContext(RdkWmTestAppCtx *context);
static bool rdkWmTestVerifyDisplayOutput(uint32_t waitTimeInSecs);

#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
RdkWmTestReturnStatus testFireboltWmExtensionToggleVisibility(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionSetBounds(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionGetClients(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionFocusedClient(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionSetZorder(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionSetOpacity(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionSetCrop(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionSetClientDisplayBounds(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionFullOpaqueMode(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltWmExtensionSetGetClientOwnerId(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
RdkWmTestReturnStatus testFireboltShellGetFireboltSurface(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
static bool rdkWmShellGetFireboltSurface(RdkWmTestAppCtx *ctx, firebolt_shell_firebolt_surface_type surfaceType, int32_t surfaceId);
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
RdkWmTestReturnStatus testFireboltSurfaceExtensionToggleVisibility(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetBoundary(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetCrop(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetZorder(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetName(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetOpacity(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
RdkWmTestReturnStatus testFireboltSurfaceExtensionVideoPinHole(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);

#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */
static bool rdkWmGetProperties(RdkWmTestAppCtx *ctx, RdkWmTestMessage *msg, RdkWmTestMessageTypeEnum message, uint32_t surfaceId, uint32_t timeoutInMilliSecs);
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

RdkWmTestReturnStatus testWmThunderPluginGetApps(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);

static RdkWmTestcase gRdkWmTests[] = {
#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
           { "testFireboltWmExtensionToggleVisibility",
             "Test firebolt_wm extension get client visibility and toggle and set the visiblity",
             testFireboltWmExtensionToggleVisibility,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_VISIBLITY_TOGGLE,
               .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE,
                                   .property = {.opacity = { .numEntries = 1, .values = {1.0}}}}
             }
           },
           { "testFireboltWmExtensionToggleBounds",
             "Test firebolt_wm extension get Window boundary and set the user values or toggle the window bounds value",
             testFireboltWmExtensionSetBounds,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS_TOGGLE,
              .u = {.wmProperties ={.x= 20,.y= 20,.width= 1500,.height=900}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                     .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1}}
            }
           },
           { "testFireboltWmExtensionGetClients",
             "Test firebolt_wm extension get the list of clients and checks the expected clients matches",
             testFireboltWmExtensionGetClients,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_NOT_NEEDED}
           },
           { "testFireboltWmExtensionGetFocusedClient",
             "Test firebolt_wm extension set the client focus and verifies if the focused client is correctly retrieved.",
             testFireboltWmExtensionFocusedClient,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_GET_FOCUSED_CLIENT,
               .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                   .property = {.visible = 1}}}
           },
           { "testFireboltWmExtensionSetGetFocusedClient",
             "Test firebolt_wm extension set the client focus and verifies if the focused client is correctly retrieved.",
             testFireboltWmExtensionFocusedClient,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_GET_FOCUSED_CLIENT,
               .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                   .property = {.visible = 1}}}
           },
           { "testFireboltWmExtensionSetZorder",
             "Test firebolt_wm extension set input window zorder value",
             testFireboltWmExtensionSetZorder,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_ZORDER,
             .u={.zOrder=5},
             .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_RESET_BOUNDS_MODE,
                                   .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1}}}
           },
           { "testFireboltWmExtensionSetBounds",
             "Test firebolt_wm extension set the user values for window bounds value",
             testFireboltWmExtensionSetBounds,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS,
             .u = {.wmProperties ={.x= 0,.y= 0,.width= 720,.height=576}},
             .prerequisite = {.condition = (RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE),
                                   .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1}}
            }
           },
           { "testFireboltWmExtensionSetOpacity",
             "Test firebolt_wm extension set the user values for window opacity value",
             testFireboltWmExtensionSetOpacity,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_OPACITY,
             .u = {.opacity = { .numEntries = 5, .values = {0.0, 0.25, 0.5, 0.75, 1.0}}},
             .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                   .property = {.visible = 1}}}
           },
           { "testFireboltWmExtensionSetCrop",
             "Test firebolt_wm extension set the user values for window Crop value",
             testFireboltWmExtensionSetCrop,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CROP,
             .u = {.wmProperties ={.x = 0, .y = 0, .width =1024 , .height = 512}},
             .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                   .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1}}}
           },
           { "testFireboltWmExtensionSetCrop1",
             "Test firebolt_wm extension set the user values for window Crop value",
             testFireboltWmExtensionSetCrop,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CROP,
             .u = {.wmProperties ={.x = 0, .y = 0, .width =720 , .height = 512 }},
             .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                   .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1}}}
           },
           { "testFireboltWmExtensionSetClientDisplayBounds",
             "Test firebolt_wm extension set the user values for window DisplayBounds value",
             testFireboltWmExtensionSetClientDisplayBounds,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CLIENT_DISPLAY_BOUNDS,
             .u = {.wmProperties ={.width =1400, .height =1020 }},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                     .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1}}}
           },
           { "testFireboltWmExtensionFullOpaqueMode",
             "Test Opaque - run this test with opaque for 1min ",
             testFireboltWmExtensionFullOpaqueMode,
             {.inputParamType = RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_OPACITY,
             .u = {.opacity = { .numEntries = 1, .values = {1.0}}},
             .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE,
                                   .property = {.visible = 1}}}
           },
           { "testFireboltWmExtensionSetGetClientOwnerId",
             "Test firebolt_wm extension get the client owner id",
             testFireboltWmExtensionSetGetClientOwnerId,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_CLIENT_OWNERID,
             .u = {.ownerId = 100},
                .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE,
                                    .property = {.opacity = { .numEntries = 1, .values = {1.0}}}}
                }
            },
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
           { "testFireboltShellGetFireboltSurface",
             "Test firebolt_shell extension to get the firebolt surface from the inupt surface type and surface id",
             testFireboltShellGetFireboltSurface,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSHELL_GET_FB_SURFACE,
              .prerequisite = {.property = {.surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
           { "testFireboltSurfaceExtensionToggleVisibility",
             "Test firebolt_surface extension get surface visibility and toggle and set the visiblity",
             testFireboltSurfaceExtensionToggleVisibility,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VISIBLITY_TOGGLE,
               .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.opacity ={ .numEntries = 1, .values = {0.75}},.surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD,.surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionToggleBoundary",
             "Test firebolt_surface extension get surface boundary and set the user values or toggle the surface bounds value",
             testFireboltSurfaceExtensionSetBoundary,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS_TOGGLE,
              .u={.surface={.x= 20, .y=20, .width = 1400, .height = 700}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                     .property = {.opacity = { .numEntries = 1, .values = {0.50}}, .visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionToggleCrop",
             "Test firebolt_surface extension get surface crop and set the user values or toggle the surface crop value",
             testFireboltSurfaceExtensionSetCrop,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP_TOGGLE,
              .u={.surface={ .x= 20, .y=20, .width = 700, .height = 500}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.opacity = { .numEntries = 1, .values = {1.0}}, .visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionSetName",
             "Test firebolt_surface extension the user input surface name and ensure the surface name is set correctly",
             testFireboltSurfaceExtensionSetName,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_NAME,
              .u={.surface={ .name= "TestSurface_01"}},
              .prerequisite = {.condition = RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                    .property = {.surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionSetZorder",
             "Test firebolt_surface extension the user input surface zorder and ensure the surface zorder is set correctly",
             testFireboltSurfaceExtensionSetZorder,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_ZORDER,
              .u={.surface={ .zOrder= 5}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.opacity = { .numEntries = 1, .values = {0.75}}, .visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionSetBoundary",
             "Test firebolt_surface extension set the user values for surface bounds value",
             testFireboltSurfaceExtensionSetBoundary,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS,
              .u={.surface={ .x= 20, .y=20, .width = 1500, .height = 700,}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                     .property = {.visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionSetCrop",
             "Test firebolt_surface extension set the user values for surface crop value",
             testFireboltSurfaceExtensionSetCrop,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP,
              .u={.surface={ .x= 20, .y=20, .width = 1900, .height = 1080}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_OPAQUE_MODE|RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.opacity = { .numEntries = 1, .values = {0.75}}, .visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionOpacity",
             "Test firebolt_surface extension the user input surface opacity and ensure the surface opacity is set correctly values are between 0.0 to 1.0",
             testFireboltSurfaceExtensionSetOpacity,
            {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_OPACITY,
              .u={.surface={ .opacity= 0.75}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_STANDARD, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionVideoPinHoleFixedSize",
             "Test firebolt_surface extension the video surface undergoes the hole punch",
            testFireboltSurfaceExtensionVideoPinHole,
           {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VIDEO_HOLE_PUNCH,
              .u={.surface={ .opacity= 1, .width = 512 , .height = 512 }},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO, .surfaceId = 1}}}
           },
           { "testFireboltSurfaceExtensionVideoPinHoleAtResolution",
             "Test firebolt_surface extension the video surface undergoes the hole punch for full screen resolution",
            testFireboltSurfaceExtensionVideoPinHole,
           {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VIDEO_HOLE_PUNCH,
              .u={.surface={ .opacity= 1}},
              .prerequisite = {.condition = RDKWM_TEST_RUNS_ON_VISIBILITY_MODE|RDKWM_TEST_CONVERT_SURFACE_TYPE,
                                   .property = {.visible = 1, .surfaceType = FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO, .surfaceId = 1}}}
           },
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */
           { "testWmThunderPluginGetApps",
             "Test Thunder Plugin getApps - gets the list of Apps and checks the expected clients match",
             testWmThunderPluginGetApps,
             {.inputParamType =RDKWM_TEST_INPUT_PARAM_TYPE_NOT_NEEDED}
           },
           {
             "", "", (RdkWmTestcaseFunc)NULL
           }
        };

const char* gRdkWmTestResult[RDKWM_TEST_RESULT_MAX] =
{
    "skipped", "passed", "failed", "stopped"
};

static bool gTestAppRunning = false;
static jmp_buf gTestEnv;

sem_t sem;
struct timespec ts;
uint32_t semwait_counter;

/*Flag to ignore the outputMode values*/
static bool isVirtualModeEnabled = false;

static void *rdkWmTestExecutorThreadRoutine(void* param);
static bool rdkWmTestCreateExecutorThread(RdkWmTestAppCtx *ctx);
static bool rdkWmTestDestoryExecutorThread(RdkWmTestAppCtx *ctx);
static bool rdkWmTestSetupMessageQueue(RdkWmTestAppCtx *ctx, const char *msgQueue);
static bool rdkWmTestDestroyMessageQueue(RdkWmTestAppCtx *ctx, const char *msgQueue);
static bool rdkWmTestSendMessage(RdkWmTestAppCtx *ctx, RdkWmTestMessage *msg, uint32_t timeoutInMilliSecs);
static bool rdkWmTestReceiveMessage(RdkWmTestAppCtx *ctx, RdkWmTestMessage *msg, uint32_t timeoutInMilliSecs);
static bool rdkWmTestReport(RdkWmTestAppCtx *ctx, RdkWmTestReportFileType fileType);

static bool RDKWmtestSetupGraphics(RdkWmTestAppCtx *ctx);
static int RDKWmtestTermGraphics(RdkWmTestAppCtx *ctx);
static int createShmBuffer(RdkWmTestAppCtx *ctx, struct wl_buffer **buffer,uint32_t format);
static bool createSurface(RdkWmTestAppCtx *ctx);
static void resizeSurface(RdkWmTestAppCtx *ctx, int dx, int dy, int width, int height);
static int destroySurface(RdkWmTestAppCtx *ctx);
static void drawFrame(RdkWmTestAppCtx *ctx);
static void RdkWmtestRenderGraphics(RdkWmTestAppCtx *ctx);

static CURL* rdkWmTestInitializeCurl();
static void rdkWmTestCleanupCurl(CURL* curl, struct curl_slist* headers);
static bool rdkWmTestFetchSecurityToken(std::string &token);
static void rdkWmTestSetupCurlOptions(CURL* curl, const std::string& url, const std::string& jsonData, const std::string& security_token, struct curl_slist** headers, std::string& response);
static std::string rdkWmTestHandleCurlResponse(const std::string& method, CURL* curl, CURLcode res, const std::string& response);
static size_t rdkWmTestWriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
static std::string rdkWmTestSendCurlRequest(const std::string& method, const std::string& jsonData);
static const char* rdkWmTrimWhitespaceAndSkip(const char* start);
static void rdkWmTestDefaultDisplay(RdkWmTestWmDisplay *params);
bool rdkWmTestCurlRequest(const std::string& method, const std::string& callsign, RdkWmTestCurlThunderPluginEnum param_type, void* param_value, int requestId, std::string &responseString);

static RdkWmTestReturnStatus rdkWmTestUpdatePropertiesStates(RdkWmTestAppCtx *ctx, RdkWmTestMessageTypeEnum type, void *property);
static RdkWmTestReturnStatus rdkWmTestPerformPreCondition(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase, RdkWmTestMessageTypeEnum message);

#ifdef RDK_WINDOW_MANAGER_LOGGER
static void* rdkWMTestSubscribeLogmonitor(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);
static bool logMonitorCallback(void *ctx, void *userParam, const char *log);
#endif

#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
static void testFireboltShellFbVideoSurfaceIdListener(void *data, struct firebolt_shell *firebolt_shell, const char *videoId);
static void testFireboltShellOnFocusListener(void *data, struct firebolt_shell *firebolt_shell, const char *client_id);
static void testFireboltShellOnBlurListener(void *data, struct firebolt_shell *firebolt_shell, const char *client_id);

static const struct firebolt_shell_listener fbShellListener = {
            .firebolt_video_surface_id = testFireboltShellFbVideoSurfaceIdListener,
            .on_focus = testFireboltShellOnFocusListener,
            .on_blur = testFireboltShellOnBlurListener
        };
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
static void testFireboltSurfaceGetPropertiesListener(void *data, struct firebolt_surface *firebolt_surface, int32_t surfaceId,
                                        int32_t x, int32_t y, uint32_t width, uint32_t height,
                                        wl_fixed_t opacity, int32_t zorder, int32_t visible,
                                        wl_fixed_t cropX, wl_fixed_t cropY, wl_fixed_t cropWidth,
                                        wl_fixed_t cropHeight,const char *name);

static const struct firebolt_surface_listener  gTestFireboltSurfaceListener = {
           .surface_properties = testFireboltSurfaceGetPropertiesListener
        };
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
static void testFireboltWmGetClientPropertiesListener(void *data, struct firebolt_wm *firebolt_wm, const char *id,
                               int32_t x, int32_t y, uint32_t width, uint32_t height,
                               wl_fixed_t opacity, int32_t zorder, int32_t visible,
                               wl_fixed_t cropX, wl_fixed_t cropY, wl_fixed_t cropWidth,
                               wl_fixed_t cropHeight, int32_t texture);
static void testFireboltWmGetFocusedClientListener(void *data, struct firebolt_wm *firebolt_wm, const char *id);
static void testFireboltWmGetClientsListener(void *data, struct firebolt_wm *firebolt_wm, const char *id);
static void testFireboltWmGetClientOwnerIdListener(void *data, struct firebolt_wm *firebolt_wm, const char *id, const int32_t ownerId);

static const struct firebolt_wm_listener gTestFireboltWmListener = {
           .client_properties = testFireboltWmGetClientPropertiesListener,
           .focused_client    = testFireboltWmGetFocusedClientListener,
           .clients           = testFireboltWmGetClientsListener,
           .client_owner     = testFireboltWmGetClientOwnerIdListener
        };
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

static void registryHandleGlobal(void *data,
                                 struct wl_registry *registry, uint32_t id,
                                 const char *interface, uint32_t version);
static void registryHandleGlobalRemove(void *data,
                                       struct wl_registry *registry,
                                       uint32_t name);

static const struct wl_registry_listener registryListener = {
            .global = registryHandleGlobal,
            .global_remove = registryHandleGlobalRemove
        };

void shm_format(void *data, struct wl_shm *wlShm, uint32_t format)
{
}
struct wl_shm_listener shm_listener = {
    shm_format
};

static void outputGeometry( void *data, struct wl_output *output, int32_t x, int32_t y,
                            int32_t physical_width, int32_t physical_height, int32_t subpixel,
                            const char *make, const char *model, int32_t transform )
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RDKWM_TEST_INFO(("Test outputGeometry Callback ctx@%p x %d y %d w %d h %d transform %d",
                       ctx, x ,y,physical_width,physical_height, transform));

}

static void outputMode( void *data, struct wl_output *output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh )
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RDKWM_TEST_INFO(("outputMode: Callback ctx@%p output@%p flags:%u width:%d height:%d refresh:%d)",
                    ctx, output, flags, width, height, refresh));

    if(output != ctx->wlOutput)
    {
        RDKWM_TEST_WARN(("outputMode: Callback not matching with client output@%p!", output));
        return;
    }
    if ( flags & WL_OUTPUT_MODE_CURRENT )
    {
        if ((isVirtualModeEnabled != true) && ((width !=  ctx->display.outputDisplayWidth) || (height != ctx->display.outputDisplayHeight)))
        {
            ctx->display.outputDisplayWidth= width;
            ctx->display.outputDisplayHeight= height;

            resizeSurface( ctx, 0, 0, ctx->display.outputDisplayWidth, ctx->display.outputDisplayHeight);

            if (ctx->display.bNotifyOutputModeEvent)
            {
                RdkWmTestMessage cbMsg;
                cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_WL_CB_OUTPUT_MODE;
                memset((char *)&cbMsg.u.wlOutputInfo, 0, sizeof(cbMsg.u.wlOutputInfo));
                cbMsg.u.wlOutputInfo.width      = ctx->display.outputDisplayWidth;
                cbMsg.u.wlOutputInfo.height     = ctx->display.outputDisplayHeight;
                RDKWM_TEST_INFO(("outputMode: SendMsg window to (%d,%d)\n", cbMsg.u.wlOutputInfo.width, cbMsg.u.wlOutputInfo.height ));

                if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
                {
                    RDKWM_TEST_ERROR(("Test wayland@.callback outputMode ctx@%p message send failed", ctx));
                }
                else
                {
                    RDKWM_TEST_INFO(("Test wayland@.callback outputMode ctx@%p message sent", ctx));
                }
            }
        }
    }
    return;
}

static void outputDone( void *data, struct wl_output *output )
{
    RDKWM_TEST_INFO(("Test wayland@.callback outputDone output@%p ", output));
}

static void outputScale( void *data, struct wl_output *output, int32_t factor )
{
    RDKWM_TEST_INFO(("Test wayland@.callback outputScale output@%p factor %d", output ,factor));
}

static const struct wl_output_listener outputListener = {
   outputGeometry,
   outputMode,
   outputDone,
   outputScale
};

static void registryHandleGlobal(void *data,
                                 struct wl_registry *registry, uint32_t id,
                                 const char *interface, uint32_t version)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    uint32_t len;

    RDKWM_TEST_INFO(("id:%d interface:%s version:%d", id, interface, version));

    len = strlen(interface);
    if((len == strlen(wl_shm_interface.name)) && !(strncmp(interface, wl_shm_interface.name, len)))
    {
        ctx->wlShm = (struct wl_shm *)wl_registry_bind(registry, id, &wl_shm_interface, 1);
        if(NULL != ctx->wlShm)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->wlShm));
            wl_shm_add_listener(ctx->wlShm, &shm_listener, NULL);
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }

    if((len == strlen(wl_compositor_interface.name)) && !(strncmp(interface, wl_compositor_interface.name, len)))
    {
        ctx->wlCompositor = (struct wl_compositor *)wl_registry_bind(registry, id, &wl_compositor_interface, 3);
        if(NULL != ctx->wlCompositor)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->wlCompositor));
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }

    if((len == strlen(wl_shell_interface.name)) && !(strncmp(interface, wl_shell_interface.name, len)))
    {
        ctx->wlShell = (struct wl_shell *)wl_registry_bind(registry, id, &wl_shell_interface, 1);
        if(NULL != ctx->wlShell)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->wlShell));
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }

    if ((len==strlen(wl_output_interface.name)) && !(strncmp(interface, wl_output_interface.name, len)))
    {
        ctx->wlOutput= (struct wl_output*)wl_registry_bind(registry, id, &wl_output_interface, 2);
        if(NULL != ctx->wlOutput)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->wlOutput));
            wl_output_add_listener(ctx->wlOutput, &outputListener, ctx);
            wl_display_roundtrip(ctx->wlDisplay);
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }
#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
    if((len == strlen(firebolt_surface_interface.name)) && !strncmp(interface, firebolt_surface_interface.name, len))
    {
        ctx->fbSurface = (struct firebolt_surface *)wl_registry_bind(registry, id, &firebolt_surface_interface, 1);
        if(NULL != ctx->fbSurface)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->fbSurface));
            firebolt_surface_add_listener(ctx->fbSurface, &gTestFireboltSurfaceListener, ctx);
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
    if((len == strlen(firebolt_shell_interface.name)) && !strncmp(interface, firebolt_shell_interface.name, len))
    {
        ctx->fbShell = (struct firebolt_shell *)wl_registry_bind(registry, id, &firebolt_shell_interface, 1);
        if(NULL != ctx->fbShell)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->fbShell));
            firebolt_shell_add_listener(ctx->fbShell, &fbShellListener, ctx);
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
    if((len == strlen(firebolt_wm_interface.name)) && !strncmp(interface, firebolt_wm_interface.name, len))
    {
        ctx->fbWm = (struct firebolt_wm*)wl_registry_bind(registry, id, &firebolt_wm_interface, 1);
        if(NULL != ctx->fbWm)
        {
            RDKWM_TEST_INFO(("wl_registry_bind id:%d interface:%s version:%d client object:%p success", id, interface, version, ctx->fbWm));
            firebolt_wm_add_listener(ctx->fbWm, &gTestFireboltWmListener, ctx);
        }
        else
        {
            RDKWM_TEST_ERROR(("wl_registry_bind id:%d interface:%s version:%d client failed", id, interface, version));
        }
    }
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

    return;
}

static void registryHandleGlobalRemove(void *data,
                                       struct wl_registry *registry,
                                       uint32_t name)
{
    /* To be implemented */
    return;
}

static void shellSurfaceConfigure(void *data,
    struct wl_shell_surface *shell_surface,
    uint32_t edges, int32_t width, int32_t height) { }

static void shellSurfacePing(void *data,
    struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static const struct wl_shell_surface_listener mShellSurfaceListener =
{
    .ping = shellSurfacePing,
    .configure = shellSurfaceConfigure,
};

/*
 * This function maintains the current states of properties visibility(isHidden), opacity(isTransparent)
 * crop(isCrop), bounds(isBounds)
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instance.
 *.- RdkWmTestMessageTypeEnum type: Test message type enum
 * - void *property: Holds either RdkTestFbWmClientInfo or RdkTestFbSurfaceInfo based on message type
 *
 * Output:
 * - The function does not return any value but updates values of isHidden, isTransparent, isCrop, isBounds in RdkWmTestWmDisplay
 */

static RdkWmTestReturnStatus rdkWmTestUpdatePropertiesStates(RdkWmTestAppCtx *ctx, RdkWmTestMessageTypeEnum type, void *property)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_PASS;

    if (type == RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES)
    {
        RdkTestFbWmClientInfo *clientInfo = (RdkTestFbWmClientInfo *)property;

        ctx->display.isHidden = (clientInfo->visible == 0);
        ctx->display.isTransparent = (clientInfo->opacity == 0.f);

        if (clientInfo->cropX == 0.f && clientInfo->cropY == 0.f &&
            clientInfo->cropWidth == 0.f && clientInfo->cropHeight == 0.f)
        {
            ctx->display.isCropMode = false;
        }
        else
        {
            ctx->display.isCropMode = true;
        }

        if (clientInfo->x == 0 && clientInfo->y == 0 &&
            clientInfo->width == ctx->display.displayWidth &&
            clientInfo->height == ctx->display.displayHeight)
        {
            ctx->display.isBoundMode = false;
        }
        else
        {
            ctx->display.isBoundMode = true;
        }

        RDKWM_TEST_INFO(("WM: isHidden %d isTransparent %d isCropMode %d isBoundMode %d", ctx->display.isHidden, ctx->display.isTransparent, ctx->display.isCropMode, ctx->display.isBoundMode));
    }
    else if (type == RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES)
    {
        RdkTestFbSurfaceInfo *surfaceInfo = (RdkTestFbSurfaceInfo *)property;

        ctx->display.surface.isHidden = (surfaceInfo->visible == 0);
        ctx->display.surface.isTransparent = (surfaceInfo->opacity == 0);

        if (surfaceInfo->cropX == 0.f && surfaceInfo->cropY == 0.f &&
            ((surfaceInfo->cropWidth == 0.f && surfaceInfo->cropHeight == 0.f) || 
            (surfaceInfo->cropWidth == (float)RDKWM_TEST_RESOLUTION_DEFAULT_SURFACE_WIDTH && \
            surfaceInfo->cropHeight == (float)RDKWM_TEST_RESOLUTION_DEFAULT_SURFACE_HEIGHT)))
        {
            ctx->display.surface.isCropMode = false;
        }
        else
        {
            ctx->display.surface.isCropMode = true;
        }

        if (surfaceInfo->x == 0 && surfaceInfo->y == 0 &&
            surfaceInfo->width == RDKWM_TEST_RESOLUTION_DEFAULT_SURFACE_WIDTH &&
            surfaceInfo->height == RDKWM_TEST_RESOLUTION_DEFAULT_SURFACE_HEIGHT)
        {
            ctx->display.surface.isBoundMode = false;
        }
        else
        {
            ctx->display.surface.isBoundMode = true;
        }

        RDKWM_TEST_INFO(("Surface: isHidden %d isTransparent %d isCropMode %d isBoundMode %d", ctx->display.surface.isHidden, ctx->display.surface.isTransparent, ctx->display.surface.isCropMode, ctx->display.surface.isBoundMode));
    }
    else
    {
        /* Handle any other types or return an error if needed. */
        RDKWM_TEST_ERROR(("Unsupported message type"));
        ret = RDKWM_TEST_RESULT_FAIL;
        goto test_fail;
    }

test_fail:
    return ret;
}

/*
 * This function checks the test Conditions given by Test apis and updates the updateConditions accordingly
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instance.
 * - RdkWmTestcase *testCase Test case details
 *.- bool isSurface: Tells if it is a surface test api or wm test api
 *
 * Output:
 * - uint32_t *updateConditions: flags for different modes like visibility, transparency, crop, bounds, and
 *.- surface type conversion
 */
static void rdkWmTestCheckConditions(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase, bool isSurface, uint32_t *updateConditions)
{
    bool isHidden = isSurface ? ctx->display.surface.isHidden : ctx->display.isHidden;
    bool isTransparent = isSurface ? ctx->display.surface.isTransparent : ctx->display.isTransparent;
    bool isCropMode = isSurface ? ctx->display.surface.isCropMode : ctx->display.isCropMode;
    bool isBoundMode = isSurface ? ctx->display.surface.isBoundMode : ctx->display.isBoundMode;

    RDKWM_TEST_INFO(("testCase->testInputs.prerequisite.condition: 0x%08X", testCase->testInputs.prerequisite.condition));

    if ((testCase->testInputs.prerequisite.condition & RDKWM_TEST_RUNS_ON_VISIBILITY_MODE) && isHidden)
    {
        *updateConditions |= RDKWM_TEST_RUNS_ON_VISIBILITY_MODE;
    }
    else if ((testCase->testInputs.prerequisite.condition & RDKWM_TEST_RUNS_ON_HIDDEN_MODE) && !isHidden)
    {
        *updateConditions |= RDKWM_TEST_RUNS_ON_HIDDEN_MODE;
    }

    if ((testCase->testInputs.prerequisite.condition & RDKWM_TEST_RUNS_ON_TRANSPARENT_MODE) && !isTransparent)
    {
        *updateConditions |= RDKWM_TEST_RUNS_ON_TRANSPARENT_MODE;
    }
    else if (testCase->testInputs.prerequisite.condition & RDKWM_TEST_RUNS_ON_OPAQUE_MODE)
    {
        *updateConditions |= RDKWM_TEST_RUNS_ON_OPAQUE_MODE;
    }

    if (testCase->testInputs.prerequisite.condition & RDKWM_TEST_RUNS_ON_CROPPED_MODE)
    {
        *updateConditions |= RDKWM_TEST_RUNS_ON_CROPPED_MODE;
    }

    if (testCase->testInputs.prerequisite.condition & RDKWM_TEST_RESET_CROPPED_MODE && isCropMode)
    {
        *updateConditions |= RDKWM_TEST_RESET_CROPPED_MODE;
    }

    if (testCase->testInputs.prerequisite.condition & RDKWM_TEST_RUNS_ON_BOUNDS_MODE)
    {
        if (!isBoundMode)
        {
            *updateConditions |= RDKWM_TEST_RUNS_ON_BOUNDS_MODE;
        }
        else if ((0 == testCase->testInputs.prerequisite.property.x) &&
                   (0 == testCase->testInputs.prerequisite.property.y) &&
                   (ctx->display.displayWidth == testCase->testInputs.prerequisite.property.width) &&
                   (ctx->display.displayHeight == testCase->testInputs.prerequisite.property.height))
        {
            RDKWM_TEST_INFO(("Test is already in bounds mode with values from preInputs"));
        }
        else
        {
            *updateConditions |= RDKWM_TEST_RUNS_ON_BOUNDS_MODE;
        }
    }

    if (testCase->testInputs.prerequisite.condition & RDKWM_TEST_RESET_BOUNDS_MODE && isBoundMode)
    {
        *updateConditions |= RDKWM_TEST_RESET_BOUNDS_MODE;
    }

    if ((testCase->testInputs.prerequisite.condition & RDKWM_TEST_CONVERT_SURFACE_TYPE) && (testCase->testInputs.prerequisite.property.surfaceType != ctx->display.surface.fbSurfaceType))
    {
        *updateConditions |= RDKWM_TEST_CONVERT_SURFACE_TYPE;
    }

    RDKWM_TEST_INFO(("updateConditions: 0x%08X", *updateConditions));

}

/*
 * This function sets one or more properties(visibility, opacity, crops & bounds) based on the testCondition
 * provided along with the test inputs. The values are sent using preInputs.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instance.
 * - RdkWmTestcase *testCase Test case details
 *.- RdkWmTestMessageTypeEnum message: Message Type Enum
 *
 * Output:
 * - The function does not return any value but sets the preInput properties based on the testConditions provided
 */
static RdkWmTestReturnStatus rdkWmTestPerformPreCondition(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase, RdkWmTestMessageTypeEnum message)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_PASS;
    RdkWmTestMessage      getMsg;
    RdkTestFbWmClientInfo clientInfo;
    RdkTestFbSurfaceInfo  surfaceInfo;
    uint32_t              updateConditions = 0;

    if (message == RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES)
    {
        rdkWmTestCheckConditions(ctx, testCase, false, &updateConditions);
    }
    else if (message == RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES)
    {
        rdkWmTestCheckConditions(ctx, testCase, true, &updateConditions);
    }

    if (updateConditions != 0)
    {
        if ((message == RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES) && rdkWmGetProperties(ctx, &getMsg, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES, 0, RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
        {
            /* Callback message received */
            memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

            /* Precondition properties value updated here conditionaly, if relevant property update bit is set in the updateConditions variable */
            if(((updateConditions & RDKWM_TEST_RUNS_ON_VISIBILITY_MODE) || \
                (updateConditions & RDKWM_TEST_RUNS_ON_HIDDEN_MODE)))
            {
                clientInfo.visible = testCase->testInputs.prerequisite.property.visible;
            }
            if(((updateConditions & RDKWM_TEST_RUNS_ON_TRANSPARENT_MODE) || \
                (updateConditions & RDKWM_TEST_RUNS_ON_OPAQUE_MODE)) && \
                (testCase->testInputs.prerequisite.property.opacity.numEntries))
            {
                /* Only one value of opacity supported in precondition */
                clientInfo.opacity = testCase->testInputs.prerequisite.property.opacity.values[0];
            }
            if(updateConditions & RDKWM_TEST_RUNS_ON_CROPPED_MODE)
            {
                clientInfo.cropX= testCase->testInputs.prerequisite.property.cropX;
                clientInfo.cropY= testCase->testInputs.prerequisite.property.cropY;
                clientInfo.cropWidth= testCase->testInputs.prerequisite.property.cropWidth;
                clientInfo.cropHeight= testCase->testInputs.prerequisite.property.cropHeight;
            }
            if(updateConditions & RDKWM_TEST_RESET_CROPPED_MODE)
            {
                clientInfo.cropX= 0;
                clientInfo.cropY= 0;
                clientInfo.cropWidth= 0;
                clientInfo.cropHeight= 0;
            }
            if(updateConditions & RDKWM_TEST_RUNS_ON_BOUNDS_MODE)
            {
                clientInfo.x= testCase->testInputs.prerequisite.property.x;
                clientInfo.y= testCase->testInputs.prerequisite.property.y;
                clientInfo.width= testCase->testInputs.prerequisite.property.width;
                clientInfo.height= testCase->testInputs.prerequisite.property.height;
            }
            if(updateConditions & RDKWM_TEST_RESET_BOUNDS_MODE)
            {
                clientInfo.x= 0;
                clientInfo.y= 0;
                clientInfo.width= ctx->display.displayWidth;
                clientInfo.height= ctx->display.displayHeight;
            }

            RDKWM_TEST_INFO(("WM: Set properties coming through test conditions"));
            firebolt_wm_set_properties(ctx->fbWm, (const char*)ctx->display.clientName, clientInfo.x, clientInfo.y,
                        clientInfo.width, clientInfo.height, ctx->display.virtualWidth, ctx->display.virtualHeight, wl_fixed_from_double(clientInfo.opacity),
                        clientInfo.zorder, clientInfo.visible, wl_fixed_from_double(clientInfo.cropX), wl_fixed_from_double(clientInfo.cropY),
                        wl_fixed_from_double(clientInfo.cropWidth), wl_fixed_from_double(clientInfo.cropHeight));

        }
        else if ((message == RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES) && rdkWmGetProperties(ctx, &getMsg, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES, testCase->testInputs.u.surface.surfaceId, RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
        {
            /* Callback message received */
            memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));

            if ((updateConditions & RDKWM_TEST_CONVERT_SURFACE_TYPE) && (testCase->testInputs.prerequisite.property.surfaceType != ctx->display.surface.fbSurfaceType))
            {
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
                RDKWM_TEST_INFO(("Convert surface type"));
                if (rdkWmShellGetFireboltSurface(ctx, testCase->testInputs.prerequisite.property.surfaceType, testCase->testInputs.prerequisite.property.surfaceId))
                {
                    RDKWM_TEST_ERROR(("Failed to convert fireboltSurface error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "Failed to convert fireboltSurface error@%d", __LINE__);
                    goto test_fail;
                }
                else
                {
                    ctx->display.surface.fbSurfaceType = testCase->testInputs.prerequisite.property.surfaceType;
                }
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */
            }
            /* Precondition properties value updated here conditionaly, if relevant property update bit is set in the updateConditions variable */
            if(((updateConditions & RDKWM_TEST_RUNS_ON_VISIBILITY_MODE) || \
                (updateConditions & RDKWM_TEST_RUNS_ON_HIDDEN_MODE)))
            {
                surfaceInfo.visible = testCase->testInputs.prerequisite.property.visible;
                firebolt_surface_set_visible(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId, surfaceInfo.visible);
            }
            if(((updateConditions & RDKWM_TEST_RUNS_ON_TRANSPARENT_MODE) || \
                (updateConditions & RDKWM_TEST_RUNS_ON_OPAQUE_MODE)) && \
                (testCase->testInputs.prerequisite.property.opacity.numEntries))
            {
                /* Only one value of opacity supported in precondition */
                surfaceInfo.opacity = testCase->testInputs.prerequisite.property.opacity.values[0];
                firebolt_surface_set_opacity(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,wl_fixed_from_double(surfaceInfo.opacity));
            }
            if(updateConditions & RDKWM_TEST_RUNS_ON_CROPPED_MODE)
            {
                surfaceInfo.cropX= testCase->testInputs.prerequisite.property.cropX;
                surfaceInfo.cropY= testCase->testInputs.prerequisite.property.cropY;
                surfaceInfo.cropWidth= testCase->testInputs.prerequisite.property.cropWidth;
                surfaceInfo.cropHeight= testCase->testInputs.prerequisite.property.cropHeight;
                firebolt_surface_set_crop(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                    wl_fixed_from_int(surfaceInfo.cropX),wl_fixed_from_int(surfaceInfo.cropY),
                    wl_fixed_from_int(surfaceInfo.cropWidth), wl_fixed_from_int(surfaceInfo.cropHeight));
            }
            if(updateConditions & RDKWM_TEST_RESET_CROPPED_MODE)
            {
                surfaceInfo.cropX= 0;
                surfaceInfo.cropY= 0;
                surfaceInfo.cropWidth= 0;
                surfaceInfo.cropHeight= 0;
                firebolt_surface_set_crop(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                    wl_fixed_from_int(surfaceInfo.cropX),wl_fixed_from_int(surfaceInfo.cropY),
                    wl_fixed_from_int(surfaceInfo.cropWidth), wl_fixed_from_int(surfaceInfo.cropHeight));
            }
            if(updateConditions & RDKWM_TEST_RUNS_ON_BOUNDS_MODE)
            {
                surfaceInfo.x= testCase->testInputs.prerequisite.property.x;
                surfaceInfo.y= testCase->testInputs.prerequisite.property.y;
                surfaceInfo.width= testCase->testInputs.prerequisite.property.width;
                surfaceInfo.height= testCase->testInputs.prerequisite.property.height;
                firebolt_surface_set_bounds(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                    surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height);
            }
            if(updateConditions & RDKWM_TEST_RESET_BOUNDS_MODE)
            {
                surfaceInfo.x= 0;
                surfaceInfo.y= 0;
                surfaceInfo.width= ctx->display.displayWidth;
                surfaceInfo.height= ctx->display.displayHeight;
                firebolt_surface_set_bounds(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                    surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height);
            }
        }
        else
        {
            RDKWM_TEST_ERROR(("%d>:Got Unexpected Message", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "%d>:Got Unexpected Message", __LINE__);
            ret = RDKWM_TEST_RESULT_FAIL;
            goto test_fail;
        }
    }

test_fail:
    return ret;
}

#ifdef RDK_WINDOW_MANAGER_LOGGER
// Callback function
static bool logMonitorCallback(void *ctx, void *userParam, const char *log)
{
    if (ctx == nullptr || log == nullptr || userParam == nullptr)
    {
        RDKWM_TEST_ERROR(("Invalid context or log."));
        return false;
    }

    RdkWmTestAppCtx* appCtx = static_cast<RdkWmTestAppCtx*>(ctx);
    RdkWmTestcase* testCase = static_cast<RdkWmTestcase*>(userParam);

    // Now search for the rdkwmtestapp (app ID) within the log message
    std::regex appIdPattern(R"(\brdkwmtestapp\S*)");
    std::smatch appIdMatch;
    std::string logstr(log);

    std::string appId;
    if (std::regex_search(logstr, appIdMatch, appIdPattern)) {
        appId = appIdMatch[0].str(); // Extract the application ID
        RDKWM_TEST_INFO(("Application ID (rdkwmtestapp) found: %s", appId.c_str()));
    }

    if(strcmp(appId.c_str(),appCtx->display.clientName) == 0)
    {
        RDKWM_TEST_INFO(("appId %s client name  %s", appId.c_str(),appCtx->display.clientName));
        testCase->runStatus.wmLogMessage.push_back(log);
    }

    return true;
}

static void* rdkWMTestSubscribeLogmonitor(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    void* handle = NULL;
    RdkTestLogMonitorSubscribeConfig subscribeCfg;

    // Set callback information
    subscribeCfg.context = ctx;
    subscribeCfg.userParam = testCase;
    subscribeCfg.callback = logMonitorCallback;

    // Subscribe to the log monitor
    handle = rdkTestLogMonitorSubscribe(subscribeCfg);

    RDKWM_TEST_INFO(("subscribe done"));

    return handle;
}
#endif


#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
static void testFireboltShellFbVideoSurfaceIdListener(void *data, struct firebolt_shell *firebolt_shell, const char *videoId)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;
    RDKWM_TEST_INFO(("Test firebolt_shell@.callback firebolt video surface id:%s", videoId));
    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_VIDEO_SURFACE_ID;
    memset(cbMsg.u.videoSufaceID, 0, sizeof(cbMsg.u.videoSufaceID));
    if (videoId != NULL)
    {
        strncpy(cbMsg.u.videoSufaceID, videoId, sizeof(cbMsg.u.videoSufaceID) - 1);
        cbMsg.u.videoSufaceID[sizeof(cbMsg.u.videoSufaceID) - 1] = '\0';

        if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
        {
            RDKWM_TEST_ERROR(("Test firebolt_shell@.callback firebolt video surface ctx@%p message send failed", ctx));
        }
        else
        {
            RDKWM_TEST_INFO(("Test firebolt_shell@.callback firebolt video surface ctx@%p message sent", ctx));
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Test firebolt_shell@.callback firebolt video surface received NULL videoId"));
    }
    /* To be implemented */
}

static void testFireboltShellOnFocusListener(void *data, struct firebolt_shell *firebolt_shell, const char *client_id)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;

    RDKWM_TEST_INFO(("Test firebolt_shell@.callback on_focus for client: %s", client_id));

/* NAN - Commented, TBD */
#if 0
    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_ON_FOCUS;
    strncpy(cbMsg.u.clientId, client_id ? client_id : "", sizeof(cbMsg.u.clientId) - 1);
    cbMsg.u.clientId[sizeof(cbMsg.u.clientId) - 1] = '\0';

    if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
    {
        RDKWM_TEST_ERROR(("Test firebolt_shell@.callback on_focus ctx@%p message send failed", ctx));
    }
#endif /* #if 0 */
}

static void testFireboltShellOnBlurListener(void *data, struct firebolt_shell *firebolt_shell, const char *client_id)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;

    RDKWM_TEST_INFO(("Test firebolt_shell@.callback on_blur for client: %s", client_id));

/* NAN - Commented, TBD */
#if 0
    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_ON_BLUR;
    strncpy(cbMsg.u.clientId, client_id ? client_id : "", sizeof(cbMsg.u.clientId) - 1);
    cbMsg.u.clientId[sizeof(cbMsg.u.clientId) - 1] = '\0';

    if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
    {
        RDKWM_TEST_ERROR(("Test firebolt_shell@.callback on_blur ctx@%p message send failed", ctx));
    }
#endif /* #if 0 */
}
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
/*
 * This function serves as a callback listener for surface property updates from the Firebolt Surface extension.
 * It logs the properties of the surface and sends a message containing these properties back to the
 * test application context.
 *
 * Input:
 * - void *data: A pointer to the context structure (RdkWmTestAppCtx) which contains information
 *   about the test application.
 * - struct firebolt_surface *firebolt_surface: A pointer to the Firebolt Surface instance.
 * - int32_t surfaceId: The ID of the surface.
 * - int32_t x: The x-coordinate of the surface.
 * - int32_t y: The y-coordinate of the surface.
 * - uint32_t width: The width of the surface.
 * - uint32_t height: The height of the surface.
 * - wl_fixed_t opacity: The opacity of the surface.
 * - int32_t zorder: The z-order of the surface.
 * - int32_t visible: Visibility status of the surface.
 * - wl_fixed_t cropX: The x-coordinate of the cropping area.
 * - wl_fixed_t cropY: The y-coordinate of the cropping area.
 * - wl_fixed_t cropWidth: The width of the cropping area.
 * - wl_fixed_t cropHeight: The height of the cropping area.
 * - const char *name: The name of the surface.
 *
 * Output:
 * - The function does not return a value, but it sends a message with the surface properties back to the test application context.
 */
static void testFireboltSurfaceGetPropertiesListener(void *data, struct firebolt_surface *firebolt_surface, int32_t surfaceId,
                                        int32_t x, int32_t y, uint32_t width, uint32_t height,
                                        wl_fixed_t opacity, int32_t zorder, int32_t visible,
                                        wl_fixed_t cropX, wl_fixed_t cropY, wl_fixed_t cropWidth,
                                        wl_fixed_t cropHeight,const char *name)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;

    RDKWM_TEST_INFO(("Test ctx@%p firebolt_surface@.callback: surface_properties" \
                    " {surfaceId:%d x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} name:%s",
                    ctx, surfaceId, x, y, width, height, wl_fixed_to_double(opacity), zorder, visible, wl_fixed_to_double(cropX),
                    wl_fixed_to_double(cropY), wl_fixed_to_double(cropWidth),wl_fixed_to_double(cropHeight),name));

    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES;
    memset((char *)&cbMsg.u.fbSurfaceInfo, 0, sizeof(cbMsg.u.fbSurfaceInfo));

    if (name[0] != 0)
    {
        strncpy(cbMsg.u.fbSurfaceInfo.name, name, sizeof(cbMsg.u.fbSurfaceInfo.name)-1);
    }
    cbMsg.u.fbSurfaceInfo.x          = x;
    cbMsg.u.fbSurfaceInfo.y          = y;
    cbMsg.u.fbSurfaceInfo.width      = width;
    cbMsg.u.fbSurfaceInfo.height     = height;
    cbMsg.u.fbSurfaceInfo.opacity    = wl_fixed_to_double(opacity);
    cbMsg.u.fbSurfaceInfo.zorder     = zorder;
    cbMsg.u.fbSurfaceInfo.visible    = visible;
    cbMsg.u.fbSurfaceInfo.cropX      = wl_fixed_to_double(cropX);
    cbMsg.u.fbSurfaceInfo.cropY      = wl_fixed_to_double(cropY);
    cbMsg.u.fbSurfaceInfo.cropWidth  = wl_fixed_to_double(cropWidth);
    cbMsg.u.fbSurfaceInfo.cropHeight = wl_fixed_to_double(cropHeight);

    if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
    {
        RDKWM_TEST_ERROR(("Test firebolt_surface@.callback surface_properties ctx@%p message send failed", ctx));
    }
    else
    {
        RDKWM_TEST_INFO(("Test firebolt_surface@.callback surface_properties ctx@%p message sent", ctx));
    }
}
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
/*
 * This function serves as a callback listener for client property updates from the Firebolt WM extension.
 * It logs the properties of the client and sends a message containing the updated client properties
 * back to the test application context.
 *
 * Input:
 * - void *data: A pointer to the context structure (RdkWmTestAppCtx) which contains information
 *   about the test application.
 * - struct firebolt_wm *firebolt_wm: A pointer to the Firebolt WM instance.
 * - const char *id: The ID of the client whose properties are being updated.
 * - int32_t x: The x-coordinate of the client.
 * - int32_t y: The y-coordinate of the client.
 * - uint32_t width: The width of the client.
 * - uint32_t height: The height of the client.
 * - wl_fixed_t opacity: The opacity of the client.
 * - int32_t zorder: The z-order of the client.
 * - int32_t visible: Visibility flag for the client.
 * - wl_fixed_t cropX: The x-coordinate of the crop area.
 * - wl_fixed_t cropY: The y-coordinate of the crop area.
 * - wl_fixed_t cropWidth: The width of the crop area.
 * - wl_fixed_t cropHeight: The height of the crop area.
 * - int32_t texture: The texture ID of the client.
 *
 * Output:
 * - The function does not return a value, but it sends a message with the updated client properties
 *   back to the test application context.
 */
static void testFireboltWmGetClientPropertiesListener(void *data, struct firebolt_wm *firebolt_wm, const char *id,
                               int32_t x, int32_t y, uint32_t width, uint32_t height,
                               wl_fixed_t opacity, int32_t zorder, int32_t visible,
                               wl_fixed_t cropX, wl_fixed_t cropY, wl_fixed_t cropWidth,
                               wl_fixed_t cropHeight, int32_t texture)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;

    RdkWmTestMessage cbMsg;
    RDKWM_TEST_INFO(("Test ctx@%p firebolt_wm@.callback: client_properties" \
            " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
            " crop{x:%f y:%f width:%f height:%f} texture:%d",
            ctx, id, x, y, width, height, wl_fixed_to_double(opacity), zorder, visible, wl_fixed_to_double(cropX),
            wl_fixed_to_double(cropY), wl_fixed_to_double(cropWidth),wl_fixed_to_double(cropHeight),texture));

    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES;
    memset((char *)&cbMsg.u.fbWmClientInfo, 0, sizeof(cbMsg.u.fbWmClientInfo));
    strncpy(cbMsg.u.fbWmClientInfo.id, id, sizeof(cbMsg.u.fbWmClientInfo.id)-1);
    cbMsg.u.fbWmClientInfo.x          = x;
    cbMsg.u.fbWmClientInfo.y          = y;
    cbMsg.u.fbWmClientInfo.width      = width;
    cbMsg.u.fbWmClientInfo.height     = height;
    cbMsg.u.fbWmClientInfo.opacity    = wl_fixed_to_double(opacity);
    cbMsg.u.fbWmClientInfo.zorder     = zorder;
    cbMsg.u.fbWmClientInfo.visible    = visible;
    cbMsg.u.fbWmClientInfo.cropX      = wl_fixed_to_double(cropX);
    cbMsg.u.fbWmClientInfo.cropY      = wl_fixed_to_double(cropY);
    cbMsg.u.fbWmClientInfo.cropWidth  = wl_fixed_to_double(cropWidth);
    cbMsg.u.fbWmClientInfo.cropHeight = wl_fixed_to_double(cropHeight);
    cbMsg.u.fbWmClientInfo.texture    = texture;
    if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
    {
        RDKWM_TEST_ERROR(("Test firebolt_wm@.callback client_properties ctx@%p message send failed", ctx));
    }
    else
    {
        RDKWM_TEST_INFO(("Test firebolt_wm@.callback client_properties ctx@%p message sent", ctx));
    }
    return;
}

/*
 * This function serves as a callback listener for focused client updates from the Firebolt WM extension.
 * It logs the ID of the focused client and sends a message containing the focused client ID
 * back to the test application context.
 *
 * Input:
 * - void *data: A pointer to the context structure (RdkWmTestAppCtx) which contains information
 *   about the test application.
 * - struct firebolt_wm *firebolt_wm: A pointer to the Firebolt WM instance.
 * - const char *id: The ID of the focused client.
 *
 * Output:
 * - The function does not return a value, but it sends a message with the focused client ID back to the test application context.
 */
static void testFireboltWmGetFocusedClientListener(void *data, struct firebolt_wm *firebolt_wm, const char *id)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;
    RDKWM_TEST_INFO(("Test firebolt_wm@.callback focused_client Id:%s", id));
    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_FOCUSED_CLIENT;
    memset(cbMsg.u.fbWmClients, 0, sizeof(cbMsg.u.fbWmClients));
    if (id != NULL)
    {
        strncpy(cbMsg.u.fbWmClients, id, sizeof(cbMsg.u.fbWmClients) - 1);
        cbMsg.u.fbWmClients[sizeof(cbMsg.u.fbWmClients) - 1] = '\0';

        if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
        {
            RDKWM_TEST_ERROR(("Test firebolt_wm@.callback get_clients ctx@%p message send failed", ctx));
        }
        else
        {
            RDKWM_TEST_INFO(("Test firebolt_wm@.callback get_clients ctx@%p message sent", ctx));
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Test firebolt_wm@.callback get_clients received NULL id"));
    }
    return;
}

/*
 * This function serves as a callback listener for client updates from the Firebolt WM extension.
 * It logs the ID of the clients and sends a message containing the client ID back to the
 * test application context.
 *
 * Input:
 * - void *data: A pointer to the context structure (RdkWmTestAppCtx) which contains information
 *   about the test application.
 * - struct firebolt_wm *firebolt_wm: A pointer to the Firebolt WM instance.
 * - const char *id: The ID of the client.
 *
 * Output:
 * - The function does not return a value, but it sends a message with the client ID
 *   back to the test application context.
 */
static void testFireboltWmGetClientsListener(void *data, struct firebolt_wm *firebolt_wm, const char *id)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;
    RDKWM_TEST_INFO(("Test firebolt_wm@.callback get_clients:%s", id));

    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_GET_CLIENTS;
    memset(cbMsg.u.fbWmClients, 0, sizeof(cbMsg.u.fbWmClients));
    if (id != NULL)
    {
        strncpy(cbMsg.u.fbWmClients, id, sizeof(cbMsg.u.fbWmClients) - 1);
        cbMsg.u.fbWmClients[sizeof(cbMsg.u.fbWmClients) - 1] = '\0';

        if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
        {
            RDKWM_TEST_ERROR(("Test firebolt_wm@.callback get_clients ctx@%p message send failed", ctx));
        }
        else
        {
            RDKWM_TEST_INFO(("Test firebolt_wm@.callback get_clients ctx@%p message sent", ctx));
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Test firebolt_wm@.callback get_clients received NULL id"));
    }
    return;
}

static void testFireboltWmGetClientOwnerIdListener(void *data, struct firebolt_wm *firebolt_wm, const char *id, int32_t ownerId)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    RdkWmTestMessage cbMsg;
    RDKWM_TEST_INFO(("Test firebolt_wm@.callback get_owner client Id:%s", id));
    cbMsg.msgType = RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_OWNER;
    memset(cbMsg.u.fbWmClients, 0, sizeof(cbMsg.u.fbWmClients));
    if (id != NULL)
    {
        cbMsg.u.fbWmClientOwnerId = ownerId;
        if (rdkWmTestSendMessage(ctx, &cbMsg, 0))
        {
            RDKWM_TEST_ERROR(("Test firebolt_wm@.callback get_owner ctx@%p message send failed", ctx));
        }
        else
        {
            RDKWM_TEST_INFO(("Test firebolt_wm@.callback get_owner ctx@%p message sent", ctx));
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Test firebolt_wm@.callback get_owner received NULL id"));
    }
    return;
}

#endif /*RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION*/

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
/*
 * This function creates a firebolt_surface wrapper around wl_surfaces
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltShellGetFireboltSurface(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;
    RdkWmTestMessage getMsg;

    if ((NULL != ctx) && (ctx->fbShell != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSHELL_GET_FB_SURFACE)
        {
            RDKWM_TEST_ERROR(("ConvertWlSurfaceToFireboltSurface :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "ConvertWlSurfaceToFireboltSurface :Unexpected Input param error@%d", __LINE__);
        }
        else
        {
            if (rdkWmShellGetFireboltSurface(ctx, testCase->testInputs.prerequisite.property.surfaceType, testCase->testInputs.prerequisite.property.surfaceId))
            {
                RDKWM_TEST_ERROR(("ConvertWlSurfaceToFireboltSurface :failed to convert fireboltSurface error@%d", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "ConvertWlSurfaceToFireboltSurface :failed to convert fireboltSurface error@%d", __LINE__);
            }
            else
            {
                ret = RDKWM_TEST_RESULT_PASS;
            }
        }
    }

test_fail:
    return ret;
}
#endif /*RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION*/

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
/*
 * This function tests the visibility setting functionality of the Firebolt Surface extension.
 * It retrieves the properties of a surface, toggles its visibility, and verifies the changes.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionToggleVisibility(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VISIBLITY_TOGGLE)
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionSetVisibility :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetVisibility :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbSurfaceInfo surfaceInfo;
            bool                    bExpectedVisiblity;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("testFireboltSurfaceExtensionToggleVisibility : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "testFireboltSurfaceExtensionToggleVisibility : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            /* Get current surface properties and by default the visiblity will be set as 1 */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                                " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                                " crop{x:%f y:%f width:%f height:%f} name:%s",
                                ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                                surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                                surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                                surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetVisibility %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetVisibility %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            /* Toggle new visibility flag */
            bExpectedVisiblity =  surfaceInfo.visible = (surfaceInfo.visible ? RDKWM_TEST_DISABLE_VISIBLITY : RDKWM_TEST_ENABLE_VISIBLITY);
            RDKWM_TEST_INFO(("id:%s set surface_properties ctx@%p ctx->fbWm@%p visibility:%d entry",
                    ctx->display.clientName, ctx, ctx->fbWm, surfaceInfo.visible));

            firebolt_surface_set_visible(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceType, surfaceInfo.visible);
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetVisibility %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetVisibility %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            /* verify the Expected visiblity value matches */
            if (surfaceInfo.visible != bExpectedVisiblity)
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected visibility:%d got visibility:%d entry",
                    ctx->display.clientName, bExpectedVisiblity, surfaceInfo.visible));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetVisibility: failed to get the expected visibility:%d got visibility:%d error@%d",
                    bExpectedVisiblity, surfaceInfo.visible, __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the boundary setting functionality of the Firebolt Surface extension.
 * It retrieves the current properties of the surface, sets new boundary values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetBoundary(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if ((testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS_TOGGLE) &&\
            (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS))
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionSetBoundary :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetBoundary :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbSurfaceInfo surfaceInfo;
            RdkTestFbSurfaceInfo storeDefaultSurfaceInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("testFireboltSurfaceExtensionSetBoundary : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "testFireboltSurfaceExtensionSetBoundary : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS)
            {
                /* Get current surface properties and store boundary values*/
                if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    /* Callback message received */
                    memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                    RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                            " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                            " crop{x:%f y:%f width:%f height:%f} name:%s",
                            ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                            surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                            surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                            surfaceInfo.name));
                    storeDefaultSurfaceInfo = surfaceInfo;
                }
                else
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionSetBounds %d>:Got Unexpected Message", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetBounds %d>:Got Unexpected Message", __LINE__);
                    goto test_fail;
                }
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            /* set the new boundary values */
            firebolt_surface_set_bounds(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                testCase->testInputs.u.surface.x, testCase->testInputs.u.surface.y, testCase->testInputs.u.surface.width, testCase->testInputs.u.surface.height);

            /* Get current surface properties and store boundary values*/
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetBounds %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetBounds %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            /* Compare the received boundary values are not equal to the previous value */
            if ((surfaceInfo.x ==testCase->testInputs.u.surface.x) && (surfaceInfo.y == testCase->testInputs.u.surface.y)\
                && (surfaceInfo.width == testCase->testInputs.u.surface.width) && (surfaceInfo.height == testCase->testInputs.u.surface.height))
             {
                    RDKWM_TEST_INFO(("SurfaceExtensionSetBounds %d>:Got Expected Value", __LINE__));
             }
            else
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected bounds:%d:%d:%d:%d",
                    ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetBounds: failed to get the expected bounds:%d:%d:%d:%d error@%d", 
                    surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height, __LINE__);
                goto test_fail;
            }


            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            if (testCase->testInputs.inputParamType == RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS)
            {
                goto test_pass;
            }

            /* restore the previous boundary values */
            firebolt_surface_set_bounds(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                storeDefaultSurfaceInfo.x, storeDefaultSurfaceInfo.y, storeDefaultSurfaceInfo.width, storeDefaultSurfaceInfo.height);

            /* Get current surface properties */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetBounds %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetBounds %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            /* Compare the received boundary values are not equal to the new values */
            if ((surfaceInfo.x ==storeDefaultSurfaceInfo.x) && (surfaceInfo.y == storeDefaultSurfaceInfo.y)\
                && (surfaceInfo.width == storeDefaultSurfaceInfo.width) && (surfaceInfo.height == storeDefaultSurfaceInfo.height))
             {
                    RDKWM_TEST_INFO(("SurfaceExtensionSetBounds %d>:Got Expected Value", __LINE__));

                    if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                    {
                        RDKWM_TEST_ERROR(("Signal recieved"));
                        ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                        goto test_fail;
                    }
             }
             else
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected bounds:%d:%d:%d:%d",
                    ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetBounds: failed to get the expected bounds:%d:%d:%d:%d error@%d", 
                    surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height, __LINE__);
                goto test_fail;
            }
test_pass:
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This test case retrieves the current properties of a Firebolt surface,
 * checks if the surface name is as expected (initially empty), sets a new
 * name for the surface, and then verifies that the name has been set correctly
 * by retrieving the surface properties again.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context of the RdkWmTest application.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetName(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{

    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        RdkWmTestMessage getMsg;
        RdkTestFbSurfaceInfo surfaceInfo;
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_NAME)
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionSetName :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetName :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionSetCrop : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetCrop : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            firebolt_surface_set_name(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId, testCase->testInputs.u.surface.name);
            /* Get current surface properties to retrive the surface name*/
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetName %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetName %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            /* Ensure the Available name matches with the input surface name */
            if (strcmp(surfaceInfo.name, testCase->testInputs.u.surface.name) != 0)
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected surface name:%s",
                    ctx->display.clientName, surfaceInfo.name));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetName: failed to get the expected surface name:%s error@%d", 
                    surfaceInfo.name, __LINE__);
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the Crop setting functionality of the Firebolt Surface extension.
 * It retrieves the current properties of the surface, sets new crop boundary values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetCrop(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if ((testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP) &&\
            (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP_TOGGLE))
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionSetCrop :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetCrop :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbSurfaceInfo surfaceInfo;
            RdkTestFbSurfaceInfo storeDefaultSurfaceInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionSetCrop : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetCrop : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP)
            {
                /* Get current surface properties and store boundary values*/
                if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    /* Callback message received */
                    memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                    RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                            " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                            " crop{x:%f y:%f width:%f height:%f} name:%s",
                            ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                            surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                            surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                            surfaceInfo.name));
                    storeDefaultSurfaceInfo = surfaceInfo;
                }
                else
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionSetCrop %d>:Got Unexpected Message", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetCrop %d>:Got Unexpected Message", __LINE__);
                    goto test_fail;
                }
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            firebolt_surface_set_crop(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                wl_fixed_from_int(testCase->testInputs.u.surface.x),wl_fixed_from_int(testCase->testInputs.u.surface.y),
                wl_fixed_from_int(testCase->testInputs.u.surface.width), wl_fixed_from_int(testCase->testInputs.u.surface.height));

            /* Get current surface properties and store boundary values*/
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetCrop %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetCrop %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }
            if ((surfaceInfo.cropX== testCase->testInputs.u.surface.x) && (surfaceInfo.cropY== testCase->testInputs.u.surface.y)\
                && (surfaceInfo.cropWidth == testCase->testInputs.u.surface.width) && (surfaceInfo.cropHeight== testCase->testInputs.u.surface.height))
            {
                    RDKWM_TEST_INFO(("SurfaceExtensionSetCrop %d>:Got Expected Value", __LINE__));
            }
            else
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected crop:%f:%f:%f:%f",
                    ctx->display.clientName, surfaceInfo.cropX, surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetCrop: failed to get the expected crop:%f:%f:%f:%f error@%d", 
                    surfaceInfo.cropX, surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight, __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            if (testCase->testInputs.inputParamType == RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP)
            {
                goto test_pass;
            }

            firebolt_surface_set_crop(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                wl_fixed_from_double(storeDefaultSurfaceInfo.cropX),wl_fixed_from_double( storeDefaultSurfaceInfo.cropY),
                wl_fixed_from_double(storeDefaultSurfaceInfo.cropWidth), wl_fixed_from_double(storeDefaultSurfaceInfo.cropHeight));
            /* Get current surface properties and store boundary values*/
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetCrop %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetCrop %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }
            if ((surfaceInfo.cropX == storeDefaultSurfaceInfo.cropX) && (surfaceInfo.cropY == storeDefaultSurfaceInfo.cropY)\
                && (surfaceInfo.cropWidth == storeDefaultSurfaceInfo.cropWidth) && (surfaceInfo.cropHeight== storeDefaultSurfaceInfo.cropHeight))
            {
                    RDKWM_TEST_INFO(("SurfaceExtensionSetCrop %d>:Got Expected Value", __LINE__));
                    if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                    {
                        RDKWM_TEST_ERROR(("Signal recieved"));
                        ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                        goto test_fail;
                    }
            }
            else
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected bounds:%f:%f:%f:%f",
                    ctx->display.clientName, surfaceInfo.cropX, surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetCrop: failed to get the expected crop:%f:%f:%f:%f error@%d", 
                    surfaceInfo.cropX, surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight, __LINE__);
                goto test_fail;
            }
test_pass:
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the Zorder  setting functionality of the Firebolt Surface extension.
 * It retrieves the current properties of the surface, sets new Zorder values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetZorder(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_ZORDER)
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionSetZorder :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetZorder :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbSurfaceInfo surfaceInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionSetZorder : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetZorder : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            firebolt_surface_set_zorder(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,wl_fixed_from_int(testCase->testInputs.u.surface.zOrder));
            /* Get current surface properties */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetZorder %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetZorder %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            /* Compare the Zorder value is as expected */
            if (surfaceInfo.zorder != testCase->testInputs.u.surface.zOrder)
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected zOrder:%d",
                    ctx->display.clientName, surfaceInfo.zorder));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetZorder: failed to get the expected zOrder:%d error@%d", 
                    surfaceInfo.zorder, __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the Opacity setting functionality of the Firebolt Surface extension.
 * Tests the firebolt_surface extension with the user input surface opacity and 
 * ensure the surface opacity is set correctly (values are between 0.0 to 1.0)
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionSetOpacity(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_OPACITY)
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionSetOpacity :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetOpacity :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbSurfaceInfo surfaceInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionSetOpacity : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetOpacity : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            firebolt_surface_set_opacity(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,wl_fixed_from_double(testCase->testInputs.u.surface.opacity));
            /* Get current surface properties */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} name:%s",
                        ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                        surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                        surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                        surfaceInfo.name));
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionSetOpacity %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionSetOpacity %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            /* Compare the opacity value is as expected */
            if (surfaceInfo.opacity != testCase->testInputs.u.surface.opacity)
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected zOrder:%d",
                    ctx->display.clientName, surfaceInfo.zorder));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionSetOpacity: failed to get the expected zOrder:%d error@%d", 
                    surfaceInfo.zorder, __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS ;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the hole punch setting functionality of the Firebolt Surface extension.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt Surface and Shell instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltSurfaceExtensionVideoPinHole(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;

    if ((NULL != ctx) && (ctx->fbSurface != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';
        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VIDEO_HOLE_PUNCH)
        {
            RDKWM_TEST_ERROR(("SurfaceExtensionVideoPinHole :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionVideoPinHole :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbSurfaceInfo surfaceInfo;
            RdkTestFbSurfaceInfo storeDefaultSurfaceInfo;

            if( rdkWmTestVerifyDisplayOutput(6) == false )
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionVideoPinHole : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionVideoPinHole : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }
            if (testCase->testInputs.inputParamType == RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VIDEO_HOLE_PUNCH)
            {
                /* Get current surface properties and store boundary values*/
                if (true == rdkWmGetProperties(ctx, &getMsg, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES, testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    /* Callback message received */
                    memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                    RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                            " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                            " crop{x:%f y:%f width:%f height:%f} name:%s",
                            ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                            surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                            surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                            surfaceInfo.name));
                    storeDefaultSurfaceInfo = surfaceInfo;
                    /* Disable crop */
                    firebolt_surface_set_crop(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId, 0,0,0,0);

                    if((testCase->testInputs.u.surface.width != 0) || (testCase->testInputs.u.surface.height != 0))
                    {
                        /* set the new boundary values */
                        firebolt_surface_set_bounds(ctx->fbSurface, testCase->testInputs.prerequisite.property.surfaceId,
                            testCase->testInputs.u.surface.x, testCase->testInputs.u.surface.y, testCase->testInputs.u.surface.width, testCase->testInputs.u.surface.height);

                        /* Get current surface properties and store boundary values*/
                        if (true == rdkWmGetProperties(ctx, &getMsg, RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES, testCase->testInputs.prerequisite.property.surfaceId,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                        {
                            /* Callback message received */
                            memcpy(&surfaceInfo, &getMsg.u.fbSurfaceInfo, sizeof(RdkTestFbSurfaceInfo));
                            RDKWM_TEST_INFO(("ctx@%p firebolt_surface@.message: surface_properties" \
                                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                                    " crop{x:%f y:%f width:%f height:%f} name:%s",
                                    ctx, ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height,
                                    surfaceInfo.opacity, surfaceInfo.zorder, surfaceInfo.visible, surfaceInfo.cropX,
                                    surfaceInfo.cropY, surfaceInfo.cropWidth, surfaceInfo.cropHeight,
                                    surfaceInfo.name));
                        }
                        else
                        {
                            RDKWM_TEST_ERROR(("SurfaceExtensionVideoPinHole %d>: Got unexpected event message", __LINE__));
                            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionVideoPinHole %d>: Got unexpected event message", __LINE__);
                            goto test_fail;
                        }

                        /* Compare the received boundary values are not equal to the previous value */
                        if ((surfaceInfo.x ==testCase->testInputs.u.surface.x) && (surfaceInfo.y == testCase->testInputs.u.surface.y)\
                            && (surfaceInfo.width == testCase->testInputs.u.surface.width) && (surfaceInfo.height == testCase->testInputs.u.surface.height))
                        {
                            RDKWM_TEST_INFO(("SurfaceExtensionVideoPinHole %d>:Got Expected Value", __LINE__));
                        }
                        else
                        {
                            RDKWM_TEST_ERROR(("id:%s failed to get the expected bounds:%d:%d:%d:%d",
                                ctx->display.clientName, surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height));
                            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"SurfaceExtensionVideoPinHole: failed to get the expected bounds:%d:%d:%d:%d error@%d", surfaceInfo.x, surfaceInfo.y, surfaceInfo.width, surfaceInfo.height, __LINE__);
                            goto test_fail;
                        }
                    }
                }
                else
                {
                    RDKWM_TEST_ERROR(("SurfaceExtensionVideoPinHole %d>: Got Unexpected Message", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionVideoPinHole %d>: Got Unexpected Message", __LINE__);
                    goto test_fail;
                }

                if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                {
                    RDKWM_TEST_ERROR(("Signal recieved"));
                    ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                    goto test_fail;
                }
            }
            else
            {
                RDKWM_TEST_ERROR(("SurfaceExtensionVideoPinHole %d>: Got Unexpected Input Parameter", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "SurfaceExtensionVideoPinHole %d>: Got Unexpected Input Parameter", __LINE__);
                goto test_fail;
            }

            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}


#endif /*RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
/*
 * This function tests the visibility setting functionality of the Firebolt Window Manager (WM) extension.
 * It retrieves the current properties of a client, toggles its visibility, and verifies that the new visibility
 * state is correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instance.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltWmExtensionToggleVisibility(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_VISIBLITY_TOGGLE)
        {
            RDKWM_TEST_ERROR(("WMExtensionSetVisibility :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetVisibility :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbWmClientInfo clientInfo;
            bool             bExpectedVisiblity;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WMExtensionSetVisibility : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetVisibility : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            /* Get client properties  and by default the visiblity will be set as 1*/
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetVisibility %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetVisibility %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if( rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) ==  false )
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            /* Toggle visibility flag */
            bExpectedVisiblity = clientInfo.visible = (clientInfo.visible ? RDKWM_TEST_DISABLE_VISIBLITY : RDKWM_TEST_ENABLE_VISIBLITY);
            RDKWM_TEST_INFO(("id:%s set client_properties ctx@%p ctx->fbWm@%p visibility:%d entry",
                    ctx->display.clientName, ctx, ctx->fbWm, clientInfo.visible));
            /* Set client properties */
            firebolt_wm_set_properties(ctx->fbWm, (const char*)ctx->display.clientName, clientInfo.x, clientInfo.y,
                        clientInfo.width, clientInfo.height, ctx->display.virtualWidth, ctx->display.virtualHeight, wl_fixed_from_double(clientInfo.opacity),
                        clientInfo.zorder, clientInfo.visible, wl_fixed_from_double(clientInfo.cropX), wl_fixed_from_double(clientInfo.cropY),
                        wl_fixed_from_double(clientInfo.cropWidth), wl_fixed_from_double(clientInfo.cropHeight));

            /* Get client properties */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));

                if (clientInfo.visible != bExpectedVisiblity)
                {
                    RDKWM_TEST_ERROR(("id:%s failed to get the expected visibility:%d got visibility:%d entry",
                        ctx->display.clientName, RDKWM_TEST_ENABLE_VISIBLITY, clientInfo.visible));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionSetVisibility: failed to get the expected visibility:%d got visibility:%d error@%d",RDKWM_TEST_ENABLE_VISIBLITY, clientInfo.visible, __LINE__);
                    goto test_fail;
                }
                
                if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                {
                    RDKWM_TEST_ERROR(("Signal recieved"));
                    ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                    goto test_fail;
                }
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetVisibility %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetVisibility %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            ret = RDKWM_TEST_RESULT_PASS ;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests setting and getting client bounds using the Firebolt WM extension.
 * It sets new boundaries for a client and verifies if the boundaries were set correctly.
 * The function also restores the client's original boundaries after the test.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the context structure which contains information
 *   about the test application and the Firebolt WM extension.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltWmExtensionSetBounds(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if ((testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS_TOGGLE) &&\
            (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS))
        {
            RDKWM_TEST_ERROR(("WMExtensionGetSetBounds :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionGetSetBounds :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestWlOutputModeInfo  outputInfo;
            RdkTestFbWmClientInfo clientInfo;
            RdkTestFbWmClientInfo storeDefaultclientInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WMExtensionSetBounds : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetBounds : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }
            if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS)
            {
                /* Get client properties to toggle */
                if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    /* Callback message received */
                    memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));
                    storeDefaultclientInfo = clientInfo;

                    RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} texture:%d",
                        ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                        clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                        clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                        clientInfo.texture));
                }
                else
                {
                    RDKWM_TEST_ERROR(("WMExtensionGetSetBounds %d>:Got Unexpected Message", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionGetSetBounds %d>:Got Unexpected Message", __LINE__);
                    goto test_fail;
                }
            }
            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }

            /* Enable output mode event notification for validating expected vs. actual values in setBounds test */
            ctx->display.bNotifyOutputModeEvent = true;

            /* Set the new Bounds values */
            firebolt_wm_set_client_bounds(ctx->fbWm, (const char*)ctx->display.clientName, testCase->testInputs.u.wmProperties.x, testCase->testInputs.u.wmProperties.y,
                       testCase->testInputs.u.wmProperties.width, testCase->testInputs.u.wmProperties.height);
            if(ctx->display.virtualDisplay != true)
            {
                if (rdkWmTestReceiveMessage(ctx, &getMsg, RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :MessageQueue timeout", __LINE__));
                    goto test_fail;
                }
                else if (getMsg.msgType == RDKWM_TEST_MESSAGE_TYPE_WL_CB_OUTPUT_MODE)
                {
                    memcpy(&outputInfo, &getMsg.u.wlOutputInfo, sizeof(RdkTestWlOutputModeInfo));
                    RDKWM_TEST_INFO(("rdkWmTestValidateMessage %d> :Message received output width %d height %d ", __LINE__,outputInfo.width,outputInfo.height));
                    if (outputInfo.width ==  testCase->testInputs.u.wmProperties.width && outputInfo.height ==  testCase->testInputs.u.wmProperties.height)
                    {
                         RDKWM_TEST_INFO(("WMExtensionGetSetBounds %d>:Got Expected Bounds Value", __LINE__));
                    }
                }
                else
                {
                    RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :Invalid Message Type", __LINE__));
                    goto test_fail;
                }
            }
            ctx->display.bNotifyOutputModeEvent = false;
            /* Get client properties */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));

                /* compare the new Bounds values */
                if (clientInfo.x ==  testCase->testInputs.u.wmProperties.x && clientInfo.y ==  testCase->testInputs.u.wmProperties.y && clientInfo.width ==  testCase->testInputs.u.wmProperties.width &&\
                    clientInfo.height ==  testCase->testInputs.u.wmProperties.height)
                {
                     RDKWM_TEST_INFO(("WMExtensionGetSetBounds %d>:Got Expected Bounds Value", __LINE__));
                }
                else
                {
                    RDKWM_TEST_ERROR(("id:%s Failed to get the expected bounds position x:%d  y: %d w:%u h:%u" \
                            "got bounds position x:%d y:%d w:%u h:%u",
                            ctx->display.clientName, storeDefaultclientInfo.x, storeDefaultclientInfo.y, storeDefaultclientInfo.width,
                            storeDefaultclientInfo.height, clientInfo.x,clientInfo.y,clientInfo.width, clientInfo.height ));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionGetSetBounds: Failed to get the expected bounds position x:%d  y: %d w:%u h:%u got bounds position x:%d y:%d w:%u h:%u error@%d",
                    storeDefaultclientInfo.x, storeDefaultclientInfo.y, storeDefaultclientInfo.width, storeDefaultclientInfo.height, clientInfo.x,clientInfo.y,clientInfo.width, clientInfo.height, __LINE__);
                    goto test_fail;
                }
                if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                {
                    RDKWM_TEST_ERROR(("Signal recieved"));
                    ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                    goto test_fail;
                }
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionGetSetBounds %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionGetSetBounds %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (testCase->testInputs.inputParamType == RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS)
            {
                goto test_pass;
            }
            ctx->display.bNotifyOutputModeEvent = true;
            /* Set the default Bounds values */
            firebolt_wm_set_client_bounds(ctx->fbWm, (const char*)ctx->display.clientName, storeDefaultclientInfo.x, storeDefaultclientInfo.y,
                   storeDefaultclientInfo.width, storeDefaultclientInfo.height);
            if(ctx->display.virtualDisplay != true)
            {
                if (rdkWmTestReceiveMessage(ctx, &getMsg, RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :MessageQueue timeout", __LINE__));
                    goto test_fail;
                }
                else if (getMsg.msgType == RDKWM_TEST_MESSAGE_TYPE_WL_CB_OUTPUT_MODE)
                {
                    memcpy(&outputInfo, &getMsg.u.wlOutputInfo, sizeof(RdkTestWlOutputModeInfo));
                    RDKWM_TEST_INFO(("rdkWmTestValidateMessage %d> :Message received output width %d height %d ", __LINE__,outputInfo.width,outputInfo.height));
                    if (outputInfo.width ==  testCase->testInputs.u.wmProperties.width && outputInfo.height ==  testCase->testInputs.u.wmProperties.height)
                    {
                         RDKWM_TEST_INFO(("WMExtensionGetSetBounds %d>:Got Expected Bounds Value", __LINE__));
                    }
                }
                else
                {
                    RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :Invalid Message Type", __LINE__));
                    goto test_fail;
                }
            }
            ctx->display.bNotifyOutputModeEvent = false;

            /* Get client properties */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));
                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));

                    if (clientInfo.x != storeDefaultclientInfo.x && clientInfo.y != storeDefaultclientInfo.y && clientInfo.width != storeDefaultclientInfo.width \
                        && clientInfo.height != storeDefaultclientInfo.height)
                    {
                        RDKWM_TEST_ERROR(("id:%s Failed to get the expected bounds position x:%d  y: %d w:%u h:%u" \
                                "got bounds position x:%d y:%d w:%u h:%u",
                                ctx->display.clientName, storeDefaultclientInfo.x, storeDefaultclientInfo.y, storeDefaultclientInfo.width,
                                storeDefaultclientInfo.height, clientInfo.x,clientInfo.y,clientInfo.width, clientInfo.height ));
                        snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionGetSetBounds: Failed to get the expected bounds position x:%d  y: %d w:%u h:%u got bounds position x:%d y:%d w:%u h:%u error@%d",
                        storeDefaultclientInfo.x, storeDefaultclientInfo.y, storeDefaultclientInfo.width, storeDefaultclientInfo.height, clientInfo.x,clientInfo.y,clientInfo.width, clientInfo.height, __LINE__);
                        goto test_fail;
                    }
                    if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                    {
                        RDKWM_TEST_ERROR(("Signal recieved"));
                        ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                        goto test_fail;
                    }
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionGetSetBounds %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionGetSetBounds %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }
test_pass:
            ret = RDKWM_TEST_RESULT_PASS ;
        }

test_fail:
        if (ctx->display.bNotifyOutputModeEvent)
        {
            ctx->display.bNotifyOutputModeEvent = false;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

    return ret;
}

/*
 * This function tests the Firebolt Window Manager (WM) extension by requesting a list of clients
 * associated with the given context. It sends a request to the Firebolt WM and waits for a
 * callback message containing the list of clients. The function checks if the expected client (i.e.,
 * the client identified by ctx->display.clientName) is present in the response.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instance
 *   and the display name of the client to verify.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltWmExtensionGetClients(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        RdkWmTestMessage getMsg;
        ctx->logMessage[0] = '\0';

        /* Get clients */
        if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_GET_CLIENTS,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
        {
            /* Callback message received */
            RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: clients" \
                    " clients id:%s , clients %s ",
                    ctx, ctx->display.clientName,getMsg.u.fbWmClients));

           if(strstr(getMsg.u.fbWmClients, ctx->display.clientName)==nullptr)
           {
                RDKWM_TEST_ERROR(("id: Failed to get the expected clients:%s got clients:%s ",
                    ctx->display.clientName ,getMsg.u.fbWmClients ));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionGetClients: Failed to get the expected clients:%s got clients:%s error @%d",ctx->display.clientName, getMsg.u.fbWmClients, __LINE__);
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS ;

            /* Providing 30secs time to wait for fetching the clients */
            if( rdkWmTestVerifyDisplayOutput(30) == false )
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
        }
        else
        {
            RDKWM_TEST_ERROR(("WMExtensionGetClients %d>:Got Unexpected Message ", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionGetClients %d>:Got Unexpected Message", __LINE__);
            goto test_fail;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests getting the currently focused client using the Firebolt WM extension.
 * It sets the focused client and verifies if the focused client is correctly retrieved.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the context structure which contains information
 *   about the test application and the Firebolt WM extension.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */

RdkWmTestReturnStatus testFireboltWmExtensionFocusedClient(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if ((testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_GET_FOCUSED_CLIENT) &&\
            (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_GET_FOCUSED_CLIENT))
        {
            RDKWM_TEST_ERROR(("WMExtensionFocusedClient :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionFocusedClient :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WMExtensionFocusedClient : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionFocusedClient : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            if (testCase->testInputs.inputParamType == RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_GET_FOCUSED_CLIENT)
            {
                /* Set Focused Client */
                firebolt_wm_set_client_focus(ctx->fbWm, ctx->display.clientName);
            }
            /* Get Focused Client */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_FOCUSED_CLIENT,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: clients" \
                    " clients id:%s , clients %s ",
                    ctx, ctx->display.clientName,getMsg.u.fbWmClients));

                if(strlen(getMsg.u.fbWmClients) == 0)
                {
                    RDKWM_TEST_ERROR(("id:%s Failed to get focused client:%s got client:%s ",
                       ctx->display.clientName,ctx->display.clientName ,getMsg.u.fbWmClients ));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionGetFocusedClient: Failed to get the expected focused client:%s got clients:%s error @%d",ctx->display.clientName, getMsg.u.fbWmClients, __LINE__);
                    goto test_fail;
                }
                if(testCase->testInputs.inputParamType == RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_GET_FOCUSED_CLIENT)
                {
                    if(strcmp(getMsg.u.fbWmClients, ctx->display.clientName)!= 0)
                    {
                        RDKWM_TEST_ERROR(("id:%s Failed to get expected focused client:%s got client:%s ",
                           ctx->display.clientName,ctx->display.clientName ,getMsg.u.fbWmClients ));
                        snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionGetFocusedClient: Failed to get the expected focused client:%s got clients:%s error @%d",ctx->display.clientName, getMsg.u.fbWmClients, __LINE__);
                        goto test_fail;
                    }
                }
                if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                {
                    RDKWM_TEST_ERROR(("Signal recieved"));
                    ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                    goto test_fail;
                }
                ret = RDKWM_TEST_RESULT_PASS;
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionGetFocusedClient %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionGetFocusedClient %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }
test_fail:
    return ret;
}

/*
 * This function tests the Zorder setting functionality of the Firebolt WindowManager extension.
 * It retrieves the current properties of the client, sets new Zorder values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltWmExtensionSetZorder(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_ZORDER)
        {
            RDKWM_TEST_ERROR(("WMExtensionSetZOrder :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetZOrder :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbWmClientInfo clientInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WMExtensionSetZorder : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetZorder : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            /* Get client properties to get the current zorder */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetZOrder %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetZOrder %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            /* Set the new zorder value */
            firebolt_wm_set_properties(ctx->fbWm, (const char*)ctx->display.clientName, clientInfo.x, clientInfo.y,
                        clientInfo.width, clientInfo.height, ctx->display.virtualWidth, ctx->display.virtualHeight, wl_fixed_from_double(clientInfo.opacity),
                        testCase->testInputs.u.zOrder, clientInfo.visible, wl_fixed_from_double(clientInfo.cropX), wl_fixed_from_double(clientInfo.cropY),
                        wl_fixed_from_double(clientInfo.cropWidth), wl_fixed_from_double(clientInfo.cropHeight));

            /* Get client properties to get the new zorder */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetZOrder %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetZOrder %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (clientInfo.zorder != testCase->testInputs.u.zOrder)
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected zOrder:%d",
                    ctx->display.clientName, clientInfo.zorder));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionSetZOrder: failed to get the expected zOrder:%d error@%d", 
                    clientInfo.zorder, __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                /* Interrupt recieved while waiting */
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the Opacity setting functionality of the Firebolt WindowManager extension.
 * It retrieves the current properties of the client, sets new opacity values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltWmExtensionSetOpacity(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_OPACITY)
        {
            RDKWM_TEST_ERROR(("WMExtensionSetOpacity :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetOpacity :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbWmClientInfo clientInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WMExtensionSetOpacity : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetOpacity : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            /* Get client properties to get the current Opacity */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetOpacity %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetOpacity %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            for(int i=0; (i < testCase->testInputs.u.opacity.numEntries) && (i < RDK_TEST_NUM_ENTRIES_MAXSIZE); i++)
            {
                /* Set the new opacity value */
                firebolt_wm_set_properties(ctx->fbWm, (const char*)ctx->display.clientName, clientInfo.x, clientInfo.y,
                            clientInfo.width, clientInfo.height, ctx->display.virtualWidth, ctx->display.virtualHeight, wl_fixed_from_double(testCase->testInputs.u.opacity.values[i]),
                            clientInfo.zorder, clientInfo.visible, wl_fixed_from_double(clientInfo.cropX), wl_fixed_from_double(clientInfo.cropY),
                            wl_fixed_from_double(clientInfo.cropWidth), wl_fixed_from_double(clientInfo.cropHeight));

                /* Get client properties to get the new zorder */
                if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    /* Callback message received */
                    memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                    RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} texture:%d",
                        ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                        clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                        clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                        clientInfo.texture));
                }
                else
                {
                    RDKWM_TEST_ERROR(("WMExtensionSetOpacity %d>:Got Unexpected Message", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetOpacity %d>:Got Unexpected Message", __LINE__);
                    goto test_fail;
                }

                if (clientInfo.opacity != testCase->testInputs.u.opacity.values[i])
                {
                    RDKWM_TEST_ERROR(("id:%s failed to get the expected Opacity:%f",
                        ctx->display.clientName, clientInfo.opacity));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionSetOpacity: failed to get the expected Opacity:%f error@%d",
                        clientInfo.opacity, __LINE__);
                    goto test_fail;
                }

                if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
                {
                    /* Interrupt recieved while waiting */
                    RDKWM_TEST_ERROR(("Signal recieved"));
                    ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                    goto test_fail;
                }
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the Crop setting functionality of the Firebolt WindowManager extension.
 * It retrieves the current properties of the client, sets new crop values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */
RdkWmTestReturnStatus testFireboltWmExtensionSetCrop(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CROP)
        {
            RDKWM_TEST_ERROR(("WMExtensionSetCrop :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetCrop :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbWmClientInfo clientInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WMExtensionSetCrop : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetCrop : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            /* Get client properties to get the current zorder */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetCrop %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetCrop %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            /* Set the new crop values */
            firebolt_wm_set_properties(ctx->fbWm, (const char*)ctx->display.clientName, clientInfo.x, clientInfo.y,
                        clientInfo.width, clientInfo.height, ctx->display.virtualWidth, ctx->display.virtualHeight, wl_fixed_from_double(clientInfo.opacity),
                        clientInfo.zorder, clientInfo.visible, wl_fixed_from_double(testCase->testInputs.u.wmProperties.x), wl_fixed_from_double(testCase->testInputs.u.wmProperties.y),
                        wl_fixed_from_double(testCase->testInputs.u.wmProperties.width), wl_fixed_from_double(testCase->testInputs.u.wmProperties.height));

            /* Get client properties to get the new crop values */
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetCrop %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetCrop %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            if (clientInfo.cropX != testCase->testInputs.u.wmProperties.x && clientInfo.cropY != testCase->testInputs.u.wmProperties.y && \
                clientInfo.cropWidth !=testCase->testInputs.u.wmProperties.width && clientInfo.cropHeight != testCase->testInputs.u.wmProperties.height)
            {
                RDKWM_TEST_ERROR(("id:%s failed to get the expected cropX:%f, cropY:%f, cropWidth:%f, cropHeight:%f",
                    ctx->display.clientName, clientInfo.cropX, clientInfo.cropY, clientInfo.cropWidth, clientInfo.cropHeight));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionSetCrop: failed to get the expected zOrder:%d error@%d",
                    clientInfo.zorder, __LINE__);
                goto test_fail;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                /* Interrupt recieved while waiting */
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

/*
 * This function tests the DisplayBounds setting functionality of the Firebolt WindowManager extension.
 * It waits for few seconds to check the display on the screen and  sets new display bounds values,
 * and verifies that the new values are correctly applied.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the Firebolt WM instances.
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure.,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */

RdkWmTestReturnStatus testFireboltWmExtensionSetClientDisplayBounds(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CLIENT_DISPLAY_BOUNDS)
        {
            RDKWM_TEST_ERROR(("WMExtensionSetClientDisplayBounds :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetClientDisplayBounds :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                 /* Interrupt recieved while waiting */
                 RDKWM_TEST_ERROR(("Signal recieved"));
                 ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                 goto test_fail;
            }

            if(testCase->testInputs.u.wmProperties.width > 0 || testCase->testInputs.u.wmProperties.height > 0)
            {
                isVirtualModeEnabled = true;
                firebolt_wm_set_client_display_bounds(ctx->fbWm, (const char*)ctx->display.clientName, testCase->testInputs.u.wmProperties.width, testCase->testInputs.u.wmProperties.height);
                ret = RDKWM_TEST_RESULT_PASS;
            }

            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                /* Interrupt recieved while waiting */
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            /* Disable virtual display to ensure not active for other test cases */
            firebolt_wm_set_client_display_bounds(ctx->fbWm, (const char*)ctx->display.clientName, 0, 0);
            isVirtualModeEnabled= false;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }
test_fail:
    return ret;
}

RdkWmTestReturnStatus testFireboltWmExtensionFullOpaqueMode(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
       RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;

    if ((NULL != ctx) && (ctx->fbWm != NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_OPACITY)
        {
            RDKWM_TEST_ERROR(("WmExtensionFullOpaqueMode:Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WmExtensionFullOpaqueMode :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbWmClientInfo clientInfo;

            if (testCase->testInputs.prerequisite.condition != RDKWM_TEST_CONDITIONS_NONE)
            {
                if(RDKWM_TEST_RESULT_FAIL == rdkWmTestPerformPreCondition(ctx, testCase, RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES))
                {
                    RDKWM_TEST_ERROR(("WmExtensionFullOpaqueMode : Error in perform precondition operation error@%d", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WmExtensionFullOpaqueMode : Error in performing precondition operation error@%d", __LINE__);
                    goto test_fail;
                }
            }

            /* Get client properties to get the current Opacity */
            if ( true == rdkWmGetProperties (ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                    " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                    " crop{x:%f y:%f width:%f height:%f} texture:%d",
                    ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                    clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                    clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                    clientInfo.texture));
            }
            else
            {
                RDKWM_TEST_ERROR(("WmExtensionFullOpaqueMode %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WmExtensionFullOpaqueMode %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }

            for(int i=0; (i < testCase->testInputs.u.opacity.numEntries) && (i < RDK_TEST_NUM_ENTRIES_MAXSIZE); i++)
            {
                /* Set the new opacity value */
                firebolt_wm_set_properties(ctx->fbWm, (const char*)ctx->display.clientName, clientInfo.x, clientInfo.y,
                            clientInfo.width, clientInfo.height, ctx->display.virtualWidth, ctx->display.virtualHeight, wl_fixed_from_double(testCase->testInputs.u.opacity.values[i]),
                            clientInfo.zorder, clientInfo.visible, wl_fixed_from_double(clientInfo.cropX), wl_fixed_from_double(clientInfo.cropY),
                            wl_fixed_from_double(clientInfo.cropWidth), wl_fixed_from_double(clientInfo.cropHeight));

                /* Get client properties to get the new zorder */
                if ( true == rdkWmGetProperties (ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
                {
                    /* Callback message received */
                    memcpy(&clientInfo, &getMsg.u.fbWmClientInfo, sizeof(RdkTestFbWmClientInfo));

                    RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_properties" \
                        " {id:%s x:%d y:%d width:%u height:%u opacity:%f zorder:%d visible:%u}" \
                        " crop{x:%f y:%f width:%f height:%f} texture:%d",
                        ctx, ctx->display.clientName, clientInfo.x, clientInfo.y, clientInfo.width, clientInfo.height,
                        clientInfo.opacity, clientInfo.zorder, clientInfo.visible,clientInfo.cropX,
                        clientInfo.cropY, clientInfo.cropWidth,clientInfo.cropHeight,
                        clientInfo.texture));
                }
                else
                {
                    RDKWM_TEST_ERROR(("WmExtensionFullOpaqueMode %d>:Got Unexpected Message", __LINE__));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WmExtensionFullOpaqueMode %d>:Got Unexpected Message", __LINE__);
                    goto test_fail;
                }

                if (clientInfo.opacity != testCase->testInputs.u.opacity.values[i])
                {
                    RDKWM_TEST_ERROR(("id:%s failed to get the expected Opacity:%f",
                        ctx->display.clientName, clientInfo.opacity));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WmExtensionFullOpaqueMode: failed to get the expected Opacity:%f error@%d",
                        clientInfo.opacity, __LINE__);
                    goto test_fail;
                }

                if( rdkWmTestVerifyDisplayOutput(60) == false )
                {
                    /* Interrupt recieved while waiting */
                    RDKWM_TEST_ERROR(("Signal recieved"));
                    ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                    goto test_fail;
                }
            }
            ret = RDKWM_TEST_RESULT_PASS;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }
test_fail:
    return ret;
}

RdkWmTestReturnStatus testFireboltWmExtensionSetGetClientOwnerId(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL ;
    if((NULL !=ctx) && (ctx->fbWm !=NULL) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';
        if (testCase->testInputs.inputParamType != RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_CLIENT_OWNERID)
        {
            RDKWM_TEST_ERROR(("WMExtensionSetGetClientOwnerId  :Unexpected Input param error@%d", __LINE__));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetGetClientOwnerId  :Unexpected Input param error@%d", __LINE__);
            goto test_fail;
        }
        else
        {
            RdkWmTestMessage getMsg;
            RdkTestFbWmClientInfo clientInfo;
            if (rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            firebolt_wm_set_owner(ctx->fbWm, (const char*)ctx->display.clientName, testCase->testInputs.u.ownerId);
            if(rdkWmTestVerifyDisplayOutput(RDKWM_TEST_DEFAULT_WAITTIME) == false)
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
            if (true == rdkWmGetProperties(ctx,&getMsg,RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_OWNER,0,RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                /* Callback message received */
                int32_t ownerId = getMsg.u.fbWmClientOwnerId;
                RDKWM_TEST_INFO(("ctx@%p firebolt_wm@.message: client_owner" \
                    "{id:%s ownerId:%d",ctx, ctx->display.clientName, ownerId));
                if (ownerId != testCase->testInputs.u.ownerId)
                {
                    RDKWM_TEST_ERROR(("id:%s failed to get the expected ownerId:%d got ownerId:%d entry",
                        ctx->display.clientName, testCase->testInputs.u.ownerId, ownerId));
                    snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WMExtensionSetGetClientOwnerId: failed to get the expected ownerId:%d got ownerId:%d error@%d", testCase->testInputs.u.ownerId, ownerId, __LINE__);
                    goto test_fail;
                }
            }
            else
            {
                RDKWM_TEST_ERROR(("WMExtensionSetGetClientOwnerId %d>:Got Unexpected Message", __LINE__));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WMExtensionSetGetClientOwnerId %d>:Got Unexpected Message", __LINE__);
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS ;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
        goto test_fail;
    }

test_fail:
    return ret;
}
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

/*
 * This function tests the RDK Window Manager (WM) getclients thunder plugin by requesting a list of clients
 * through curl Apis. The function checks if the expected client (i.e.,
 * the client identified by ctx->display.clientName) is present in the response.
 *
 * Input:
 * - RdkWmTestAppCtx *ctx: A pointer to the application context containing the display
 * - RdkWmTestcase *testCase Test case details
 *
 * Output:
 * - Returns 
 * - RDKWM_TEST_RESULT_PASS on success ,
 * - RDKWM_TEST_RESULT_FAIL on failure ,
 * - RDKWM_TEST_RESULT_FORCE_STOP on signal interrupt/sem failure
 */

RdkWmTestReturnStatus testWmThunderPluginGetApps(RdkWmTestAppCtx *ctx,RdkWmTestcase *testCase)
{
    RdkWmTestReturnStatus ret = RDKWM_TEST_RESULT_FAIL;
    bool                  curlRequestResult;
    std::string           getAppsList;

    if ((NULL != ctx) && (testCase != NULL))
    {
        ctx->logMessage[0] = '\0';

        /* cURL to call getApps */
        if (!(curlRequestResult = rdkWmTestCurlRequest(
            RDKWM_TEST_GETAPPS,
            "",
            RDKWM_TEST_PARAM_TYPE_NONE,
            nullptr,
            RDKWM_TEST_GETAPPS_REQUEST_ID,
            getAppsList)))
        {
            RDKWM_TEST_ERROR(("HTTP POST request to getClients failed: %s", getAppsList.c_str()));
            snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE, "WmThunderPluginGetApps %d>:failed to getApps list", __LINE__);
            goto test_fail;
        }
        else
        {
            RDKWM_TEST_INFO(("ctx@%p" \
                    " clients id:%s , Apps %s ",
                    ctx, ctx->display.clientName,getAppsList.c_str()));

           if(strstr(getAppsList.c_str(), ctx->display.clientName)==nullptr)
           {
                RDKWM_TEST_ERROR(("id: Failed to get the expected clients:%s got clients:%s ",
                    ctx->display.clientName ,getAppsList.c_str() ));
                snprintf(ctx->logMessage,RDKWM_TEST_LOG_MESSAGE_MAXSIZE,"WmThunderPluginGetApps: Failed to get the expected clients:%s got clients:%s error @%d",ctx->display.clientName, getAppsList.c_str(), __LINE__);
                goto test_fail;
            }
            ret = RDKWM_TEST_RESULT_PASS ;
            /* Providing 30secs time to wait for fetching the clients */
            if( rdkWmTestVerifyDisplayOutput(30) == false )
            {
                RDKWM_TEST_ERROR(("Signal recieved"));
                ret = RDKWM_TEST_RESULT_FORCE_STOP ;
                goto test_fail;
            }
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("Invalid context!"));
    }

test_fail:
    return ret;
}

static bool rdkWmTestReport(RdkWmTestAppCtx *ctx, RdkWmTestReportFileType fileType)
{
    char path[RDKWM_TEST_REPORT_FILE_SIZE];
    bool ret = false;
    if (RDKWM_TEST_REPORT_FORMAT_PLAIN_TEXT == fileType)
    {
        struct stat sb;
        int fd = -1;
        if (mkdir(RDKWM_TESTAPP_REPORT_PATH, 0644) != 0)
        {    
            /* Check if the error is due to the directory already existing */
            if (errno != EEXIST)
            {
                RDKWM_TEST_ERROR(("failed to create report directory - %s", RDKWM_TESTAPP_REPORT_PATH));
                goto report_fail;
            }
        }
        sprintf(path, "%s/%s_%s_%u.txt",RDKWM_TESTAPP_REPORT_PATH, RDKWM_TESTAPP_NAME,ctx->display.clientName, getpid());
        FILE* fp = fopen(path, "w");

        if (NULL == fp)
        {
            RDKWM_TEST_ERROR(("unable to open file - %s", path ));
        }
        else
        {
            fprintf(fp, "Selected test count:%d; passed count = %d; failed count = %d skipped count = %d\n", ctx->testList.size(),
                ctx->passCount, ctx->failCount, (ctx->testList.size() - (ctx->failCount+ctx->passCount)));

            RDKWM_TEST_INFO(("-----------------------------------------------------------------------------"));
            RDKWM_TEST_INFO(("Total test Count: %d", ctx->testList.size()));
            RDKWM_TEST_INFO(("Passed test Count: %d", ctx->passCount));
            RDKWM_TEST_INFO(("Failure test Count: %d", ctx->failCount));
            RDKWM_TEST_INFO(("Skipped test Count: %d", (ctx->testList.size() - (ctx->failCount + ctx->passCount))));

            for (auto& testCase : ctx->testList)
            {
                if (testCase.second.runStatus.testResult != RDKWM_TEST_RESULT_UNKNOWN)
                {
                    char start[RDKWM_TEST_REPORT_STRING_MAXSIZE];
                    char end[RDKWM_TEST_REPORT_STRING_MAXSIZE];
                    char result[RDKWM_TEST_REPORT_STRING_MAXSIZE];
                    char time[RDKWM_TEST_REPORT_STRING_MAXSIZE];
                    long timeTakenSecs = (long)testCase.second.runStatus.testEnd.tv_sec - (long)testCase.second.runStatus.testStart.tv_sec;

                    strftime(start, sizeof(start), "%D %T", gmtime(&testCase.second.runStatus.testStart.tv_sec));
                    strftime(end, sizeof(end), "%D %T", gmtime(&testCase.second.runStatus.testEnd.tv_sec));
                    strftime(time, sizeof(time), "%T", gmtime((const time_t*)&timeTakenSecs));
                    fprintf(fp, "\tTestName case name:%s Result:%s Start:%s - End:%s - TimeTaken:%s\n", testCase.first,
                                 gRdkWmTestResult[testCase.second.runStatus.testResult], start, end, time);
                    if (!testCase.second.runStatus.message.empty())
                    {
                        RDKWM_TEST_ERROR(("Failure test Name %s", testCase.first));

                        fprintf(fp, "\tLog Message:\n");
                        for (const auto &message : testCase.second.runStatus.message)
                        {
                            fprintf(fp, "\t\t%s \n", message.c_str());
                            RDKWM_TEST_ERROR(("Failure Message %s", message.c_str()));
                        }
                        testCase.second.runStatus.message.clear();
                    }
                    #ifdef RDK_WINDOW_MANAGER_LOGGER
                    if (!testCase.second.runStatus.wmLogMessage.empty())
                    {
                        RDKWM_TEST_ERROR(("Failure test Name %s", testCase.first));

                        for (const auto &message : testCase.second.runStatus.wmLogMessage)
                        {
                            fprintf(fp, "\t\t%s \n", message.c_str());
                            RDKWM_TEST_ERROR(("Failure Message %s", message.c_str()));
                        }
                        testCase.second.runStatus.wmLogMessage.clear();
                    }
                    #endif
                }
            }
            RDKWM_TEST_INFO(("-----------------------------------------------------------------------------"));
            fflush(fp);
            fd = fileno(fp);
            if (fd != -1)
            {
                fsync(fileno(fp));
            }
            fclose(fp);
            ret = true;
        }
    }
    else
    {
        RDKWM_TEST_ERROR(("File Type Not Supported"));
    }
report_fail:
    return ret;
}

static void signalHandler(int signum)
{
    RDKWM_TEST_INFO(("signalHandler: signum %d", signum));
    gTestAppRunning = false ;

    RDKWM_TEST_INFO(("semwait_counter:%d",semwait_counter));
    if( semwait_counter > 0 )
    {
        RDKWM_TEST_INFO(("Signal handler is triggered:%d",signum));
        if ( sem_post(&sem) == -1 ) 
        {
            RDKWM_TEST_ERROR(("Failure in sempost"));
        }
    }
    else
    {
        /* When there is no semaphore wait */
        longjmp(gTestEnv, signum);
    }
}

static long long currentTimeMillis()
{
    long long timeMillis;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timeMillis = (long long)tv.tv_sec * 1000LL + ((long long)tv.tv_usec / 1000LL);
    return timeMillis;
}

static void listAllTests()
{
    uint32_t loop=0;
    RDKWM_TEST_INFO(("List of the test cases"));
    RDKWM_TEST_INFO(("----------------------"));
    for(loop = 0; loop < ((sizeof(gRdkWmTests)/sizeof(RdkWmTestcase))-1); loop++)
    {
        RDKWM_TEST_INFO(("TestCase #%d:%s", loop+1, (char *)gRdkWmTests[loop].name));
        RDKWM_TEST_INFO(("%s", (char *)gRdkWmTests[loop].desc));
    }
}

static void showUsage()
{
    printf("usage:\n");
    printf(" %s [options]\n", RDKWM_TESTAPP_NAME);
    printf("where [options] are:\n");
    printf("  --display <name> : (in lower case)wayland display to connect to\n");
    printf("  --resolution <width> <height> : current display resolution, if --resolution option not specified, 1920x1080 used\n");
    printf("  --client : <client name> : (in lower case) that will be controlled, if --client option not specified YouTube is used\n");
    printf("  --virtualdisplay <virtualwidth> <virtualheight>: if --virtualdisplay option not specified virtualdisplay, virtualwidth, virtualheight would be set to 0\n");
    printf("  --topmost <bool topmost optional param>: if --topmost option not specified topmost would be set to 0\n");
    printf("  --focus <bool focus optional param>: if --focus option not specified focus would be set to 0\n");
    printf("  --surface <surfacetype(string: (standard popup notification video))>. : This will option create the number of wl_surfaces based on string type max surface =16 \n");
    printf("  -? : show usage\n");
    printf("  -l : list All Tests\n");
    printf("  --test testXX : Test case/Test cases specified to be run\n");
    printf("  --test all : All test cases to be run,\n");
    printf("\n");
}

static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    RdkWmTestAppCtx *ctx = (RdkWmTestAppCtx*)data;
    wl_callback_destroy(callback);
    ctx->needRedraw = true;
}

static struct wl_callback_listener frameListener = {
                    redraw
                };

static void drawFrame(RdkWmTestAppCtx *ctx)
{

    RdkWmtestRenderGraphics(ctx);
    ctx->frameCallback = wl_surface_frame(ctx->wlSurface);
    wl_callback_add_listener(ctx->frameCallback, &frameListener, ctx);
    if (ctx->isOpengl)
    {
        eglSwapBuffers(ctx->eglDisplay, ctx->eglSurfaceWindow);
    }
    else
    {
        wl_surface_attach(ctx->wlSurface,*ctx->buffers, 0, 0);
        wl_surface_damage(ctx->wlSurface, 0, 0, ctx->display.displayWidth, ctx->display.displayHeight);
        wl_surface_commit(ctx->wlSurface);
    }
}

static void fillAllTestDetails(RdkWmTestAppCtx *ctx,bool oneArgFlag)
{
    uint32_t    loop = 0;
    for (loop = 0; loop < (sizeof(gRdkWmTests)/sizeof(RdkWmTestcase)); loop++)
    {
        if (gRdkWmTests[loop].name)
        {
            if (gRdkWmTests[loop].func)
            {
                ctx->testList[(char *)gRdkWmTests[loop].name] = gRdkWmTests[loop];
#ifdef RDKWM_TEST_DEBUG_TEST_SELECTION
                RDKWM_TEST_INFO(("Test case inserted:%s",(char *)gRdkWmTests[loop].name));
#endif
            }
            else
            {
#ifdef RDKWM_TEST_DEBUG_TEST_SELECTION
                RDKWM_TEST_ERROR(("Invalid test func,hence skipping:%s",gRdkWmTests[loop].name));
#endif
            }
        }
        else
        {
#ifdef RDKWM_TEST_DEBUG_TEST_SELECTION
            RDKWM_TEST_ERROR(("Mismatch,hence skip & continue"));
#endif
        }
    }
    if((ctx->testList.empty()) && !oneArgFlag)
    {
        RDKWM_TEST_INFO(("Testcases are invalid"));
        showUsage();
    }
}

static bool rdkWmTestVerifyDisplayOutput(uint32_t waitTimeInSecs)
{
    int semReturnValue ;
    RDKWM_TEST_INFO(("rdkWmTestVerifyDisplayOutput entry"));

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
        handle_error("clock_gettime");

    semwait_counter++;
    ts.tv_sec += waitTimeInSecs;

    while ((semReturnValue = sem_timedwait(&sem, &ts)) == -1 && (errno == EINTR))
    {
        RDKWM_TEST_INFO(("errno is EINTR\n"));
        continue;
    }

    if (semReturnValue == -1)
    {
        if (errno == ETIMEDOUT)
        {
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
                handle_error("clock_gettime");

            /* Wait for waitTimeInSecs and so if semaphore not posted then timeout & continue with other tests */
            RDKWM_TEST_INFO(("sem_timedwait() timed out:%ld\n",ts.tv_sec));
            semwait_counter--;
            return true;
        }
        else
        {
            /* sem_timedwait,failed & exit */
            RDKWM_TEST_ERROR(("sem_timedwait"));
            semwait_counter--;
            return false;
        }
    }
    else
    {
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            handle_error("clock_gettime");

        /* Wait for waitTimeInSecs and so if semaphore posted,and exit */
        RDKWM_TEST_INFO(("sem_timedwait() succeeded:%ld",ts.tv_sec));
        semwait_counter--;
        return false ;
    }
}

int32_t main(int argc, char** argv)
{
    RdkWmTestAppCtx *ctx = NULL;
    struct sigaction sigint;
    int32_t     ret = -1;
    uint32_t    loop = 0;
    uint32_t    iterator = 0;
    bool        bExecutorActive = false;
    char        *testCase = NULL;
    bool        curlRequestResult;
    std::string activateResponseString;
    std::string createdisplayResponseString;

    /* Rdk WM Test context allocation */
    ctx = new RdkWmTestAppCtx;
    if (NULL == ctx)
    {
        RDKWM_TEST_ERROR(("No memory for Rdk WM testApp context!"));
        goto test_exit;
    }

    wmTestInitialiseContext(ctx);

    rdkWmTestDefaultDisplay(&ctx->display);

    if (sem_init(&sem, 0, 0) == -1)
        handle_error("sem_init");

    RDKWM_TEST_INFO(("%s starting", RDKWM_TESTAPP_NAME));

    if(argc == 1)
    {
        RDKWM_TEST_INFO(("Add all tests"));
        fillAllTestDetails(ctx,true);
    }

    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp((const char*)argv[i], "--display"))
        {
            if (i+1 < argc)
            {
                ++i;
                ctx->display.displayName = argv[i];
            }
        }
        else if (!strcmp((const char*)argv[i], "--resolution"))
        {
            if (i+2 < argc)
            {
                ++i;
                ctx->display.displayWidth = atoi(argv[i]);
                ++i;
                ctx->display.displayHeight = atoi(argv[i]);
            }
        }
        else if (!strcmp((const char*)argv[i], "--client"))
        {
            if (i+1 < argc)
            {
                ++i;
                /* Client Name */
                strncpy(ctx->display.clientName, argv[i], sizeof(ctx->display.clientName)-1);
            }
        }
        else if (!strcmp((const char*)argv[i], "--virtualdisplay"))
        {
            if (i+2 < argc)
            {
                ctx->display.virtualDisplay = RDKWM_TEST_ENABLE_VIRTUAL_DISPLAY;
                ++i;
                ctx->display.virtualWidth= atoi(argv[i]);
                ++i;
                ctx->display.virtualHeight= atoi(argv[i]);
            }
        }
        else if (!strcmp((const char*)argv[i], "--topmost"))
        {
            if (i+1 < argc)
            {
                ++i;
                ctx->display.topmost= atoi(argv[i]);
            }
        }
        else if (!strcmp((const char*)argv[i], "--focus"))
        {
            if (i+1 < argc)
            {
                ++i;
                ctx->display.focus= atoi(argv[i]);
            }
        }
        else if (!strcmp((const char*)argv[i], "-?"))
        {
            showUsage();
            ret = 0;
            goto test_help;
        }
        else if (!strcmp((const char*)argv[i], "-l"))
        {
            listAllTests();
            ret = 0;
            goto test_ctxcleanup;
        }
        else if (!strcmp((const char*)argv[i], "--test"))
        {
            if (i+1 < argc)
            {
                if (!strncmp((const char*)argv[i+1], "all",strlen("all")))
                {
                    /* If all option provided execute all the tests*/
                    RDKWM_TEST_INFO(("Insert all the tests"));
                    fillAllTestDetails(ctx,false);
                    ++i;
                }
                else
                {
                    while ((i+1<argc) && (argv[i+1][0] != '-'))
                    {
                        /* If testcases mentioned[Testcase should have prefix:testFirebolt],add it to the list*/
                        RDKWM_TEST_INFO(("Insert the specific test case %s", argv[i+1]));
                        testCase= argv[i+1];
                        for( iterator = 0 ;iterator < (sizeof(gRdkWmTests)/sizeof(RdkWmTestcase)) ; iterator++ )
                        {
                            if (!strncmp(testCase,gRdkWmTests[iterator].name,RDKWM_TEST_API_NAME_MAXSIZE))
                            {
                                if( gRdkWmTests[iterator].func )
                                {
                                    ctx->testList[(char *)gRdkWmTests[iterator].name] = gRdkWmTests[iterator];
#ifdef RDKWM_TEST_DEBUG_TEST_SELECTION
                                    RDKWM_TEST_INFO(("Test case inserted:%s :%p",gRdkWmTests[iterator].name,gRdkWmTests[iterator].testInputs));
#endif
                                }
                                else
                                {
                                    RDKWM_TEST_WARN(("Given Test %s test API not associated",gRdkWmTests[iterator].name));
                                }
                            }
                        }
                        i++;
                    }
                    if( ctx->testList.empty() )
                    {
                        RDKWM_TEST_WARN(("Testcase not found"));
                        showUsage();
                        goto test_help;
                    }
                }
            }
        }
    }

    if (ctx->testList.empty())
    {
        /* By default insert all the tests*/
        RDKWM_TEST_INFO(("Selecting all the tests by default"));
        fillAllTestDetails(ctx,false);
        if( ctx->testList.empty() )
        {
            goto test_ctxcleanup;
        }
    }

    RDKWM_TEST_INFO(("Display width=%d, height=%d Virtual Display width=%d, height=%d Topmost=%d Focus=%d", 
        ctx->display.displayWidth, ctx->display.displayHeight, ctx->display.virtualWidth, ctx->display.virtualHeight, ctx->display.topmost, ctx->display.focus));

    ctx->display.outputDisplayWidth = ctx->display.displayWidth;
    ctx->display.outputDisplayHeight = ctx->display.displayHeight;

    if(ctx->display.virtualWidth > 0 || ctx->display.virtualHeight > 0)
    {
        isVirtualModeEnabled = true;
    }

    /* cURL to activate RDKWindowManager plugin */
    if (!(curlRequestResult = rdkWmTestCurlRequest(
        RDKWM_TEST_ACTIVATE_METHOD,
        RDKWM_TEST_ACTIVATE_CALLSIGN,
        RDKWM_TEST_PARAM_TYPE_NONE,
        nullptr,
        RDKWM_TEST_ACTIVATE_REQUEST_ID,
        activateResponseString))) {
        RDKWM_TEST_ERROR(("HTTP POST request to activate RDKWindowManager failed: %s", activateResponseString.c_str()));
        goto test_fail;
    }

    if(ctx->display.clientName[0]== '\0')
    {
        snprintf(ctx->display.clientName, RDKWM_TEST_NAME_MAXSIZE -1, "%s_%d", RDKWM_TESTAPP_NAME, getpid());
        ctx->display.clientName[sizeof(ctx->display.clientName)-1] = '\0';
    }

    /* cURL to call createDisplay */
    if (!(curlRequestResult = rdkWmTestCurlRequest(
                                RDKWM_TEST_CREATEDISPLAY_METHOD,
                                RDKWM_TEST_CREATEDISPLAY_CALLSIGN,
                                RDKWM_TEST_PARAM_TYPE_CREATEDISPLAY,
                                &ctx->display,
                                RDKWM_TEST_CREATEDISPLAY_REQUEST_ID,
                                createdisplayResponseString)))
    {
        RDKWM_TEST_ERROR(("HTTP POST request to createdisplay failed: %s", createdisplayResponseString.c_str()));
        goto test_fail;
    }

    /* For the Testapp to work, we are assigning clientName to displayName for wl_display_connect to work */
    if (ctx->display.displayName == NULL || strlen(ctx->display.displayName) == 0)
    {
        ctx->display.displayName = ctx->display.clientName;
    }

    ctx->wlDisplay = wl_display_connect(ctx->display.displayName);
    if (!ctx->wlDisplay)
    {
        RDKWM_TEST_ERROR(("wl_display_connect(%s) failed", ctx->display.displayName));
        goto test_ctxcleanup;
    }
    else
    {
        RDKWM_TEST_INFO(("wl_display_connect(%s) display@%p connected", ctx->display.displayName, ctx->wlDisplay));

        RDKWM_TEST_INFO(("Calling wl_display_get_registry(%p)", ctx->wlDisplay));
        ctx->wlRegistry = wl_display_get_registry(ctx->wlDisplay);
        if (!ctx->wlRegistry)
        {
            RDKWM_TEST_ERROR(("wl_display_get_registry(%p) failed", ctx->wlDisplay));
            goto test_fail;
        }
        RDKWM_TEST_INFO(("wl_display_get_registry(%p) registry@%p success", ctx->wlDisplay, ctx->wlRegistry));

        RDKWM_TEST_INFO(("Calling wl_registry_add_listener registry@%p", ctx->wlRegistry));
        ret = wl_registry_add_listener(ctx->wlRegistry, &registryListener, ctx);
        if (ret < 0)
        {
            RDKWM_TEST_ERROR(("wl_registry_add_listener registry:%p failed", ctx->wlRegistry));
            goto test_fail;
        }
        RDKWM_TEST_INFO(("wl_registry_add_listener registry@%p success", ctx->wlRegistry));

        RDKWM_TEST_INFO(("Calling wl_display_roundtrip(%p)", ctx->wlDisplay));
        ret = wl_display_roundtrip(ctx->wlDisplay);
        if (ret < 0)
        {
            RDKWM_TEST_ERROR(("wl_display_roundtrip(%p) failed", ctx->wlDisplay));
            goto test_fail;
        }
        RDKWM_TEST_INFO(("wl_display_roundtrip(%p) success", ctx->wlDisplay));

        loop = 0;
        while (NULL == ctx->wlCompositor)
        {
            RDKWM_TEST_WARN(("wl_registry_bind with wl_compositor not yet done"));
            usleep(100);
            if(++loop > 10)
            {
                RDKWM_TEST_ERROR(("wl_registry_bind with wl_compositor not ready existing!"));
                goto test_fail;
            }
        }
        /* Setup EGL and GL surface create */
        if (!RDKWmtestSetupGraphics(ctx))
        {
                RDKWM_TEST_ERROR(("Failed to setup EGL and GL surface"));
                goto test_fail;
        }
        /* Draw Frame */
        drawFrame(ctx);

        /* Signal handler attachment */
        sigint.sa_handler = signalHandler;
        sigemptyset(&sigint.sa_mask);
        sigint.sa_flags = SA_RESETHAND;
        sigaction(SIGINT, &sigint, NULL);
        sigaction(SIGABRT, &sigint, NULL);
        sigaction(SIGSEGV, &sigint, NULL);
        sigaction(SIGFPE, &sigint, NULL);
        sigaction(SIGILL, &sigint, NULL);
        sigaction(SIGBUS, &sigint, NULL);

        wl_display_flush(ctx->wlDisplay);

        snprintf(ctx->msgQueueName, RDKWM_TEST_NAME_MAXSIZE -1, "%s_%d",RDKWM_TEST_MESSAGEQUEUE_NAME, getppid());

        gTestAppRunning = true;
        /* RDK WM Test message queue setup */
        if (rdkWmTestSetupMessageQueue(ctx, ctx->msgQueueName))
        {
            RDKWM_TEST_ERROR(("Test message queue setup - failed!"));
            goto test_fail;
        }
        #ifdef RDK_WINDOW_MANAGER_LOGGER
        RdkTestLogMonitorConfig monitorCfg = {RDK_WINDOW_MANAGER_LOGFILE};
        if(rdkTestLogMonitorInitialize(std::move(monitorCfg)) == -1)
        {
            RDKWM_TEST_ERROR(("rdkTestLogMonitorInitialize failed"));
        }
        else
        {
            RDKWM_TEST_INFO(("rdkTestLogMonitorInitialize started"));
        }
        #endif
        /* Create RDK WM Test executor thread */
        if (rdkWmTestCreateExecutorThread(ctx))
        {
            RDKWM_TEST_ERROR(("Test executor thread create - failed!"));
            goto test_fail;
        }
        else
        {
            RDKWM_TEST_INFO(("Test executor thread create - success"));
            bExecutorActive = true;
        }

        /* Re-drawing loop */
        loop = 0;
        while (gTestAppRunning)
        {
            if (wl_display_dispatch(ctx->wlDisplay) == -1)
            {
                break;
            }
            if (ctx->needRedraw)
            {
                ctx->needRedraw = false;
                drawFrame(ctx);
            }
        }
        /* Successful */
        ret = 0;
    }

test_fail:
    /* forced stop here, may needed for test_fail case */
    gTestAppRunning = false;
    wmTestDestroyContext(ctx);

    if (sem_destroy(&sem) != 0)
    {
        RDKWM_TEST_ERROR(("Sem_destroy failed"));
    }
    else
    {
        RDKWM_TEST_INFO(("Semaphore destroyed successfully"));
    }

test_ctxcleanup:
test_help:
    /* RDK WM Test context delete */
    if (NULL != ctx)
    {
        delete ctx;
        ctx = NULL;
    }

test_exit:
    RDKWM_TEST_INFO(("%s exit:%d", RDKWM_TESTAPP_NAME, ret));
    return ret;
}

/* Initialize the display structure with default values */
static void rdkWmTestDefaultDisplay(RdkWmTestWmDisplay *params)
{

    if (params == NULL)
    {
        RDKWM_TEST_ERROR(("Invalid context or display!"));
        return;
    }

    memset(params->clientName, 0, sizeof(params->clientName));
    params->displayName = "" ;
    params->displayWidth = RDKWM_TEST_RESOLUTION_DEFAULT_DISPLAY_WIDTH;
    params->displayHeight = RDKWM_TEST_RESOLUTION_DEFAULT_DISPLAY_HEIGHT;
    params->virtualDisplay = false;
    params->virtualWidth = RDKWM_TEST_DISPLAY_DEFAULT_VALUE;
    params->virtualHeight = RDKWM_TEST_DISPLAY_DEFAULT_VALUE;
    params->topmost = false;
    params->focus = false;

    params->isHidden = false;
    params->isTransparent = false;
    params->isCropMode = false;
    params->isBoundMode = false;

    params->surface.isHidden = false;
    params->surface.isTransparent = false;
    params->surface.isCropMode = false;
    params->surface.isBoundMode = false;
    params->surface.fbSurfaceType = (firebolt_shell_firebolt_surface_type)0;

}

/* Function to initialize cURL */
static CURL* rdkWmTestInitializeCurl()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    return curl_easy_init();
}

/* Function to clean up cURL */
static void rdkWmTestCleanupCurl(CURL* curl, struct curl_slist* headers)
{
    if (curl)
    {
        curl_easy_cleanup(curl);
    }
    if (headers)
    {
        curl_slist_free_all(headers);
    }
    curl_global_cleanup();
}

/* Function to fetch security token */
static bool rdkWmTestFetchSecurityToken(std::string &token)
{
    char authorizationBearer[RDKWM_TEST_SECURITY_TOKEN_SIZE];
    char securityToken[RDKWM_TEST_SECURITY_TOKEN_SIZE];
    FILE* fp;

    snprintf(authorizationBearer, sizeof(authorizationBearer), "WPEFrameworkSecurityUtility | cut -d '\"' -f 4");
    fp = popen(authorizationBearer, "r");
    if (fp == NULL) {
        RDKWM_TEST_ERROR(("popen failed"));
        return false;
    }

    while (fgets(securityToken, sizeof(securityToken) - 1, fp) != NULL) {
        securityToken[strcspn(securityToken, "\n")] = 0;
        token = securityToken;
    }

    if (pclose(fp) != 0) {
        RDKWM_TEST_ERROR(("Authorization bearer not found or exited with error status"));
        return false;
    }

    return true;
}

static void rdkWmTestSetupCurlOptions(CURL* curl, const std::string& url, const std::string& jsonData, const std::string& security_token, struct curl_slist** headers, std::string& response)
{
    char auth_header[RDKWM_TEST_AUTHORIZATION_HEADER_MAX_SIZE];

    if(headers == nullptr)
    {
        RDKWM_TEST_ERROR(("Headers is still null"));
        goto curl_exit;
    }
#if 0
    *headers = curl_slist_append(*headers, "Content-Type: application/json");

    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", security_token.c_str());
    if (auth_header[0] == '\0' || security_token.empty())
    {
        RDKWM_TEST_ERROR(("Failed to set authorization header"));
        goto curl_exit;
    }
    *headers = curl_slist_append(*headers, auth_header);
#endif /* #if 0 */
    (void)curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    (void)curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    (void)curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
    (void)curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rdkWmTestWriteCallback);
    (void)curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);     /* Set connection timeout to 5 seconds */

    (void)curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);    /* Set the total timeout to 10 seconds */
    (void)curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

curl_exit:
    return;
}

/* Function to remove whitespace/new line/tab space */
static const char* rdkWmTrimWhitespaceAndSkip(const char* start) {
    while (*start == ' ' || *start == '\n' || *start == '\t') {
        ++start;
    }
    return start;
}

static std::string rdkWmTestExtractValueFromCurlResponse(const std::string& response, const char* key, const char delimiter = '}')
{
    const char* start = strstr(response.c_str(), key);
    const char* end;
    std::string result;

    if (start)
    {
        start += strlen(key);
        start = rdkWmTrimWhitespaceAndSkip(start);
        end = strchr(start, delimiter);
        result = std::string(start, end ? end - start : strlen(start));
    }
curl_exit:
    return result;
}

static int rdkWmTestExtractIdFromCurlResponse(const std::string& response, const char* key)
{
    std::string idStr = rdkWmTestExtractValueFromCurlResponse(response, key, ',');
    return !idStr.empty() ? atoi(idStr.c_str()) : 0;
}

RdkWmTestCurlMethodEnum rdkWmTestGetMethodEnum(const std::string& method)
{
    if (method == RDKWM_TEST_GETAPPS) {
        return RDKWM_TEST_GETCLIENTS_ENUM;
    } else if (method == RDKWM_TEST_ACTIVATE_METHOD) {
        return RDKWM_TEST_ACTIVATE_METHOD_ENUM;
    } else if (method == RDKWM_TEST_CREATEDISPLAY_METHOD) {
        return RDKWM_TEST_CREATEDISPLAY_METHOD_ENUM;
    } else {
        return RDKWM_TEST_UNKNOWN;
    }
}


static std::string rdkWmTestHandleCurlResponse(const std::string& method, CURL* curl, CURLcode res, const std::string& response)
{
    long        httpCode = 0;
    std::string resultBuffer;
    int         resultId = 0;
    int         errorId = 0;
    int         errorCode = 0;

    res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (res != CURLE_OK) {
        RDKWM_TEST_ERROR(("curl_easy_getinfo() failed: %s", curl_easy_strerror(res)));
    }

    if ((httpCode >= 200 && httpCode < 300) && !response.empty())
    {
        RDKWM_TEST_INFO(("Response data: %s", response.c_str()));

        std::string result = rdkWmTestExtractValueFromCurlResponse(response, "\"result\":");
        resultId = rdkWmTestExtractIdFromCurlResponse(response, "\"id\":");
        errorCode = rdkWmTestExtractIdFromCurlResponse(response, "\"code\":");
        errorId = rdkWmTestExtractIdFromCurlResponse(response, "\"error\":{\"id\":");

        RdkWmTestCurlMethodEnum methodEnum = rdkWmTestGetMethodEnum(method);

        switch (methodEnum) {
            case RDKWM_TEST_GETCLIENTS_ENUM:
            {
                if (!result.empty()) {
                    resultBuffer = result.c_str();
                } else {
                    RDKWM_TEST_INFO(("No clients found"));
                    resultBuffer = "[Curl response error] No clients found";
                }
                break;
            }
            default:
            {
                if (!result.empty()) {
                    resultBuffer = "Result: " + result + " id: " + std::to_string(resultId);
                } else if (errorCode > 0) {
                    resultBuffer = "[Curl response error] Error code: " + std::to_string(errorCode) + " id: " + std::to_string(errorId);
                } else if (httpCode != 200) {
                    RDKWM_TEST_ERROR(("Non-OK HTTP Status Code: %ld", httpCode));
                    resultBuffer = "[Curl response error] HTTP Error Code: " + std::to_string(httpCode);
                } else {
                    RDKWM_TEST_INFO(("No error or result in response"));
                    resultBuffer = "[Curl response error] No error or result in response";
                }
                break;
            }
        }
    } else {
        resultBuffer = "[Curl response error] Response empty or httpcode not between range 200-299";
    }

    return resultBuffer;
}

/* Write callback function for cURL */
static size_t rdkWmTestWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/* Function to send cURL request */
static std::string rdkWmTestSendCurlRequest(const std::string& method, const std::string& jsonData)
{
    CURLcode           res;
    int                attempts = 0;
    std::string        securityToken = "";
    std::string        result;
    std::string        response;
    struct curl_slist* headers = nullptr;

    CURL* curl = rdkWmTestInitializeCurl();
    if (!curl) {
        return "[Curl response error] Curl initialization failed";
    }

#if 0
    if (!rdkWmTestFetchSecurityToken(securityToken)) {
        rdkWmTestCleanupCurl(curl, headers);
        result = "[Curl response error] Failed to fetch security token";
        goto curl_exit;
    }
#endif /* #if 0 */

    rdkWmTestSetupCurlOptions(curl, RDKWM_TEST_JSON_RPC_URL, jsonData, securityToken, &headers, response);
    do {
        res = curl_easy_perform(curl);
        attempts++;

        if (res == CURLE_OK) {
            result = rdkWmTestHandleCurlResponse(method, curl, res, response);
            break;
        }

        if (res == CURLE_OPERATION_TIMEDOUT) {
            RDKWM_TEST_ERROR(("curl_easy_perform() timeout: %s", curl_easy_strerror(res)));
            result = "[Curl response error] Curl timedout";
        } else {
            RDKWM_TEST_ERROR(("curl_easy_perform() failed: %s", curl_easy_strerror(res)));
            result = "[Curl response error] Curl failed";
        }
    } while (attempts < RDKWM_TEST_MAX_CURL_RETRIES);

    rdkWmTestCleanupCurl(curl, headers);

curl_exit:
    return result;
}

/*
 * This function handles the curl requests and response.
 * The return type is bool(pass/fail)
 * Gets error string/results string in responseString
 *
 * Input:
 * - const std::string& method: the method string used for the curl request
 * - const std::string& callsign: the callsign string used for the curl request
 * - RdkWmTestCurlThunderPluginEnum thunderPlugin: enum type of thunderPlugin(none or createdisplay)
 * - void* param_value: thunderPluginParams structure in case of createdisplay curl request
 * - requestId: Request id of the request
 * - std::string &responseString: A string that will be used to store the error/result string from the response.
 *
 * Output:
 * - Returns true on success and false on failure.
 */
bool rdkWmTestCurlRequest(const std::string& method, const std::string& callsign, RdkWmTestCurlThunderPluginEnum thunderPlugin, void* thunderPluginParams, int requestId, std::string &responseString)
 {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    writer.Key("jsonrpc");
    writer.String("2.0");
    writer.Key("id");
    writer.Int(requestId);

    if (!method.empty()) {
    writer.Key("method");
    writer.String(method.c_str());
    }
    writer.Key("params");
    writer.StartObject();

    /* Handle param_value based on param_type */
    switch (thunderPlugin) {
        case RDKWM_TEST_PARAM_TYPE_NONE: {
            if (!callsign.empty()) {
            writer.Key("callsign");
            writer.String(callsign.c_str());
            }
            RDKWM_TEST_ERROR(("No additional parameters to add"));
            break;
         }

        case RDKWM_TEST_PARAM_TYPE_CREATEDISPLAY: {
            RDKWM_TEST_ERROR(("Fill additional params createDisplay"));

            /* Construct displayParams as a JSON string */
            rapidjson::StringBuffer nestedBuffer;
            rapidjson::Writer<rapidjson::StringBuffer> nestedWriter(nestedBuffer);

            nestedWriter.StartObject();
            RdkWmTestWmDisplay* displayProperties = static_cast<RdkWmTestWmDisplay*>(thunderPluginParams);

            nestedWriter.Key("client");
            nestedWriter.String(displayProperties->clientName);

            if (!callsign.empty()) {
            nestedWriter.Key("callsign");
            nestedWriter.String(callsign.c_str());
            }

            nestedWriter.Key("displayName");
            nestedWriter.String(displayProperties->displayName);

            nestedWriter.Key("displayWidth");
            nestedWriter.Uint(displayProperties->displayWidth);

            nestedWriter.Key("displayHeight");
            nestedWriter.Uint(displayProperties->displayHeight);

            nestedWriter.Key("virtualDisplay");
            nestedWriter.Bool(displayProperties->virtualDisplay);

            nestedWriter.Key("virtualWidth");
            nestedWriter.Uint(displayProperties->virtualWidth);

            nestedWriter.Key("virtualHeight");
            nestedWriter.Uint(displayProperties->virtualHeight);

            nestedWriter.Key("topmost");
            nestedWriter.Bool(displayProperties->topmost);

            nestedWriter.Key("focus");
            nestedWriter.Bool(displayProperties->focus);
            nestedWriter.EndObject();

            /* Add stringified displayParams */
            writer.Key("displayParams");
            writer.String(nestedBuffer.GetString());
            break;
        }
        default: {
            RDKWM_TEST_ERROR(("Unsupported parameter type"));
            break;
        }
    }

    writer.EndObject();
    writer.EndObject();

    std::string response_activate = rdkWmTestSendCurlRequest (method, buffer.GetString());
    responseString = response_activate.c_str();
    if (response_activate.find("[Curl response error]") != std::string::npos)
    {
        return false;
    }
    return true;
}

static void wmTestInitialiseContext(RdkWmTestAppCtx *context)
{
#ifdef RDK_WINDOW_MANAGER_BUILD_TEST_APP_WITH_OPENGL
    context->isOpengl = true;
#else
    context->isOpengl = false;
#endif
    context->wlShm = NULL;
    context->wlShell = NULL;
    context->wlDisplay = NULL;
    context->wlRegistry = NULL;
    context->wlCompositor = NULL;
    context->wlSurface = NULL;
    context->frameCallback = NULL;
    context->drawShape = RDKWM_TEST_DRAW_RECTANGLE;
    context->startTime = 0;
    context->currTime = 0;
    context->needRedraw = false;
    context->msgQueueFds[RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_WRITE] = (mqd_t)-1;
    context->msgQueueFds[RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_READ] = (mqd_t)-1;
    context->msgId = 0;
    context->executorPthread = (pthread_t)NULL;
    context->returnStatus = NULL;
    context->bExecutorActive = false;
    context->eglDisplay = EGL_NO_DISPLAY;
    context->eglConfig = NULL;
    context->eglSurfaceWindow = EGL_NO_SURFACE;
    context->eglContext = EGL_NO_CONTEXT;
    context->native = NULL;
    context->gl.rotation_uniform = 0;
    context->gl.pos = 0;
    context->gl.col = 0;
#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
    context->fbSurface = NULL;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

    memset(context->msgQueueName,0,RDKWM_TEST_NAME_MAXSIZE);
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
    context->fbShell = NULL;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
    context->fbWm = NULL;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */
    context->testList.clear();
    context->failCount = 0;
    context->passCount= 0;
    memset(context->logMessage, 0, RDKWM_TEST_LOG_MESSAGE_MAXSIZE);
    memset(&context->display, 0, sizeof(RdkWmTestWmDisplay));
}

static void wmTestDestroyContext(RdkWmTestAppCtx *ctx)
{
    if(NULL != ctx)
    {
        if(!ctx->isOpengl)
        {
            if (ctx->buffers)
            {
                if(*ctx->buffers != NULL)
                {
                     wl_buffer_destroy(*ctx->buffers);
                    *ctx->buffers =NULL;
                }
                free(ctx->buffers);
                ctx->buffers = NULL;
            }
        }
        if (NULL != ctx->wlCompositor)
        {
            wl_compositor_destroy(ctx->wlCompositor);
            RDKWM_TEST_INFO(("wl_compositor_destroy(%p)", ctx->wlCompositor));
            ctx->wlCompositor = NULL;
        }

        if (NULL != ctx->wlShell)
        {
            wl_shell_destroy(ctx->wlShell);
            RDKWM_TEST_INFO(("wl_simple_shell_destroy(%p)", ctx->wlShell));
            ctx->wlShell = NULL;
        }
        if (NULL != ctx->wlShm)
        {
            wl_shm_destroy(ctx->wlShm);
            RDKWM_TEST_INFO(("wl_simple_shm_destroy(%p)", ctx->wlShm));
            ctx->wlShm = NULL;
        }

        if (RDKWmtestTermGraphics(ctx) < 0)
        {
            RDKWM_TEST_ERROR(("Failed to terminate graphics context"));
        }

        if (NULL != ctx->wlRegistry)
        {
            wl_registry_destroy(ctx->wlRegistry);
            RDKWM_TEST_INFO(("wl_registry_destroy(%p)", ctx->wlRegistry));
            ctx->wlRegistry = NULL;
        }

        if (NULL != ctx->wlDisplay)
        {
            wl_display_disconnect(ctx->wlDisplay);
            RDKWM_TEST_INFO(("wl_display_disconnect(%p) display:%s disconnected", ctx->wlDisplay, ctx->display.displayName));
            ctx->wlDisplay = NULL;
        }

        /* Destory message queue */
        if (rdkWmTestDestroyMessageQueue(ctx,ctx->msgQueueName))
        {
            RDKWM_TEST_ERROR(("Test message queue destroy - failed !"));
        }

        /* Wait for executor thread to finish off */
        if (ctx->bExecutorActive && rdkWmTestDestoryExecutorThread(ctx))
        {
            RDKWM_TEST_ERROR(("Test executor thread pthread_join - failed!"));
        }
    }
}

static void *rdkWmTestExecutorThreadRoutine(void* param)
{
    RdkWmTestAppCtx  *ctx;
    uint32_t loop = 0;
    int32_t num;
    void* handle=NULL;
    RdkWmTestReturnStatus testResult = RDKWM_TEST_RESULT_PASS ;
    RdkWmTestList testCase;

    RDKWM_TEST_INFO(("Test Executor Routine - started"));
    ctx = (RdkWmTestAppCtx *)param;
    if (NULL != ctx)
    {
        ctx->failCount = 0;
        ctx->passCount = 0;

        for (auto& testCase : ctx->testList)
        {
            if ((testCase.first) && (testCase.second.func))
            {
                RDKWM_TEST_INFO(("-----------------------------------------------------------------------------"));
                RDKWM_TEST_INFO(("Running loop:%d test{name:%s func@%p}", loop+1, testCase.first, testCase.second.func));
                num = setjmp(gTestEnv);
                if (num == 0)
                {
                    clock_gettime(CLOCK_REALTIME, &testCase.second.runStatus.testStart);
                    testCase.second.runStatus.testResult = RDKWM_TEST_RESULT_UNKNOWN;

                    #ifdef RDK_WINDOW_MANAGER_LOGGER
                    handle = rdkWMTestSubscribeLogmonitor(ctx,(&testCase.second));
                    if (NULL == handle)
                    {
                        /* error - add log fail log message */
                        RDKWM_TEST_ERROR(("Log monitoring subscribe failed for Testcase #%d test{name:%s func@%p}",
                                            loop+1, testCase.first, testCase.second.func));
                    }
                    #endif /* RDK_WINDOW_MANAGER_LOGGER  */

                    testResult = testCase.second.func(ctx,(&testCase.second));

                    #ifdef RDK_WINDOW_MANAGER_LOGGER
                    if (NULL != handle)
                    {
                        rdkTestLogMonitorUnsubscribe(handle);
                    }
                    #endif /* RDK_WINDOW_MANAGER_LOGGER  */

                    RDKWM_TEST_INFO(("Result Test case #%d test{name:%s func@%p} - %s",
                                        loop+1, testCase.first, testCase.second.func ,((testResult != RDKWM_TEST_RESULT_PASS )? "Fail" : "Pass")));

                    clock_gettime(CLOCK_REALTIME, &testCase.second.runStatus.testEnd);
                    testCase.second.runStatus.testResult = testResult;

                    if (ctx->logMessage[0] != '\0')
                    {
                         testCase.second.runStatus.message.push_back(ctx->logMessage);
                    }

                    if (testResult == RDKWM_TEST_RESULT_FAIL)
                    {
                        ++ctx->failCount;
                    }
                    else if (testResult == RDKWM_TEST_RESULT_PASS)
                    {
                        ++ctx->passCount;
                    }
                    else if(testResult == RDKWM_TEST_RESULT_FORCE_STOP)
                    {
                        RDKWM_TEST_WARN(("Exiting the test case due to signal interrupt"));
                        goto ret_fail;
                    }
                    loop++;
                }
                else
                {
                    RDKWM_TEST_WARN(("Abort:signum:%d raised!",num));
                    goto ret_fail;
                }
            }
            else
            {
                RDKWM_TEST_WARN(("Invalid inputs"));
            }
            RDKWM_TEST_INFO(("-----------------------------------------------------------------------------"));
        }

ret_fail:
        if (!rdkWmTestReport(ctx, RDKWM_TEST_REPORT_FORMAT_PLAIN_TEXT))
        {
            RDKWM_TEST_ERROR(("Failure to generate the report file"));
        }

        RDKWM_TEST_INFO(("Test Executor Routine - exit"));
    }
    else
    {
        RDKWM_TEST_ERROR(("Test Executor Routine - context invalid!"));
    }

    gTestAppRunning = false;
    pthread_exit(((NULL != ctx) ? (void *)ctx->returnStatus : (void *)NULL));
    return NULL;
}

static bool rdkWmTestCreateExecutorThread(RdkWmTestAppCtx *ctx)
{
    bool status = true;

    if (!ctx)
    {
        RDKWM_TEST_ERROR(("Not valid RDK WM Test context!"));
        goto ret_fail;
    }

    /* Thread creation for test executor */
    if (0 != pthread_create(&ctx->executorPthread, NULL, rdkWmTestExecutorThreadRoutine, ctx))
    {
        RDKWM_TEST_ERROR(("Failed to pthread_create for executor errno:%d(%s)", errno, strerror(errno)));
    }
    else
    {
        RDKWM_TEST_INFO(("Test executor thread created: TID[%lu]", ctx->executorPthread));
        status = false;
    }

ret_fail:
    return status;
}

static bool rdkWmTestDestoryExecutorThread(RdkWmTestAppCtx *ctx)
{
    bool status = true;

    if (!ctx)
    {
        RDKWM_TEST_ERROR(("Not valid RDK WM Test context!"));
        goto ret_fail;
    }

    /* Wait for executor thread to finish off */
    if (0 != pthread_join(ctx->executorPthread, (void **)&ctx->returnStatus))
    {
        RDKWM_TEST_ERROR(("Test executor thread pthread_join - failed!"));
    }
    else
    {
        RDKWM_TEST_INFO(("Test executor thread pthread_join - success"));
        status = false;
    }

ret_fail:
    return status;
}

static bool rdkWmTestSetupMessageQueue(RdkWmTestAppCtx *ctx, const char *msgQueue)
{
    bool status = false;
    struct mq_attr mqAttr;
    RdkWmTestMsgQueueFdEnum fdIndex;

    if (!ctx)
    {
        RDKWM_TEST_ERROR(("Not valid RDK WM Test context!"));
        status = true;
        goto ret_fail;
    }

    /* Write message queue file descriptor */
    fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_WRITE;
    if ((mqd_t)-1 == ctx->msgQueueFds[fdIndex])
    {
        /* Message queue attributes */
        memset((char *)&mqAttr, 0, sizeof(struct mq_attr));
        mqAttr.mq_flags   = 0;
        mqAttr.mq_maxmsg  = RDKWM_TEST_MESSAGEQUEUE_MAX_MESSAGES;
        mqAttr.mq_msgsize = sizeof(RdkWmTestMessage);

        /* Opening the message queue for write operation */
        fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_WRITE;
        ctx->msgQueueFds[fdIndex] = mq_open(msgQueue, (O_CREAT | O_WRONLY), 0666, &mqAttr);
        if ((mqd_t)-1 == ctx->msgQueueFds[fdIndex])
        {
            RDKWM_TEST_ERROR(("Failed to mq_open %s for write! errno:%d(%s)", msgQueue, errno, strerror(errno)));
            status = true;
            goto ret_fail;
        }
    }

    /* Read message queue file descriptor */
    fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_READ;
    if ((mqd_t)-1 == ctx->msgQueueFds[fdIndex])
    {
        /* Opening the message queue for read operation */
        ctx->msgQueueFds[fdIndex] = mq_open(msgQueue, O_RDONLY);
        if ((mqd_t)-1 == ctx->msgQueueFds[fdIndex])
        {
            RDKWM_TEST_ERROR(("Failed to mq_open %s for read! errno:%d(%s)", msgQueue, errno, strerror(errno)));
            status = true;
            goto ret_fail;
        }
    }

    if (!status)
    {
        ctx->msgId = 0;
        RDKWM_TEST_INFO(("Test message queue create - success"));
    }

ret_fail:
    return status;
}

static bool rdkWmTestDestroyMessageQueue(RdkWmTestAppCtx *ctx, const char *msgQueue)
{
    bool status = true;
    RdkWmTestMsgQueueFdEnum fdIndex;

    if (!ctx)
    {
        RDKWM_TEST_ERROR(("Not valid RDK WM Test context!"));
        goto ret_fail;
    }

    /* Close write message queue file descriptor */
    fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_WRITE;
    if ((mqd_t)-1 != ctx->msgQueueFds[fdIndex])
    {
        RdkWmTestMessage msg;
        /* Send stop exit message to release the caller, if anyone is waiting for message */
        msg.msgId = 0xFFFFFFFF;
        msg.msgType = RDKWM_TEST_MESSAGE_TYPE_STOP_EXIT;
        if (!rdkWmTestSendMessage(ctx, &msg, 0))
        {
            RDKWM_TEST_INFO(("Test sent stop exit message{id:%d type:%d}", msg.msgId, msg.msgType));
            /* To reach stop message to the caller */
            usleep(500);
        }

        if (-1 == mq_close(ctx->msgQueueFds[fdIndex]))
        {
            RDKWM_TEST_ERROR(("Failed to mq_close %s for write fd[%d]:%d! errno:%d(%s)", msgQueue, fdIndex,
                            ctx->msgQueueFds[fdIndex], errno, strerror(errno)));
        }
        else
        {
            ctx->msgQueueFds[fdIndex] = (mqd_t)-1;
            status = false;
        }
    }

    /* Close read message queue file descriptor */
    fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_READ;
    if ((mqd_t)-1 != ctx->msgQueueFds[fdIndex])
    {
        if (-1 == mq_close(ctx->msgQueueFds[fdIndex]))
        {
            RDKWM_TEST_ERROR(("Failed to mq_close %s for read fd[%d]:%d! errno:%d(%s)", msgQueue, fdIndex,
                            ctx->msgQueueFds[fdIndex], errno, strerror(errno)));
            status = true;
        }
        else
        {
            ctx->msgQueueFds[fdIndex] = (mqd_t)-1;
            status = false;
        }
    }

    /* Unlink the message queue path */
    if (!status && (NULL != msgQueue))
    {
        if (-1 == mq_unlink(msgQueue))
        {
            RDKWM_TEST_ERROR(("Failed to mq_unlink %s errno:%d(%s)", msgQueue, errno, strerror(errno)));
            status = true;
        }
    }

    if (!status)
    {
        RDKWM_TEST_INFO(("Test message queue destory - success"));
    }
ret_fail:
    return status;
}

static bool rdkWmGetProperties(RdkWmTestAppCtx *ctx, RdkWmTestMessage *msg, RdkWmTestMessageTypeEnum message, uint32_t surfaceId, uint32_t timeoutInMilliSecs)
{
    bool ret = false;
    void *property = NULL;

    if ((NULL != ctx)
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
        && (ctx->fbShell != NULL)
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
       && (ctx->fbWm != NULL)
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
        && (ctx->fbSurface != NULL)
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */
        )
    {
        switch (message)
        {
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
            case RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES:
                firebolt_wm_get_properties(ctx->fbWm, (const char*)ctx->display.clientName);
                break;

            case RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_GET_CLIENTS:
                firebolt_wm_get_clients(ctx->fbWm);
                break;

            case RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_FOCUSED_CLIENT:
                firebolt_wm_get_focused_client(ctx->fbWm);
                break;

            case RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_OWNER:
                firebolt_wm_get_owner(ctx->fbWm, (const char*)ctx->display.clientName);
                break;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
            case RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES:
                firebolt_surface_get_properties(ctx->fbSurface, surfaceId);
                break;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

            default:
                 RDKWM_TEST_ERROR(("rdkWmGetProperties %d> :Invalid Message Type", __LINE__));
                 break;
        }

        if (rdkWmTestReceiveMessage(ctx, msg, timeoutInMilliSecs))
        {
            RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :MessageQueue timeout", __LINE__));
        }
        else if (message == msg->msgType)
        {
            RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :Got the Expected Message", __LINE__));

            /* Set the property based on message type */
            if (message == RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES)
            {
                property = &msg->u.fbWmClientInfo;
            }
            else if (message == RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES)
            {
                property = &msg->u.fbSurfaceInfo;
            }
            else
            {
                RDKWM_TEST_ERROR(("Message type handling not required to update properties states"));
            }
            /* Call the below api to maintain the current states of the properties visibiltiy,
                 opacity, crop and bounds(RdkWmTestWmDisplay)at any point of time to reduce
                 unecessary calls to get and set the properties */
            if (property != NULL)
            {
                if (RDKWM_TEST_RESULT_FAIL == rdkWmTestUpdatePropertiesStates(ctx, message, property))
                {
                    RDKWM_TEST_ERROR(("rdkWmTestUpdatePropertiesStates failed for %s",
                                      (message == RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES) ? "WM" : "surface"));
                    return ret;
                }
            }
            ret = true;
        }
        else
        {
            RDKWM_TEST_ERROR(("rdkWmTestValidateMessage %d> :Invalid Message Type", __LINE__));
        }
    }
    return ret;
}
static bool rdkWmTestSendMessage(RdkWmTestAppCtx *ctx, RdkWmTestMessage *msg, uint32_t timeoutInMilliSecs)
{
    bool status = true;
    RdkWmTestMsgQueueFdEnum fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_WRITE;

    if (!ctx)
    {
        RDKWM_TEST_ERROR(("Not valid RDK WM Test context argument!"));
        goto ret_fail;
    }
    else if (!msg)
    {
        RDKWM_TEST_ERROR(("Not valid msg argument!"));
        goto ret_fail;
    }
    else if ((mqd_t)-1 != ctx->msgQueueFds[fdIndex])
    {
        if(RDKWM_TEST_MESSAGE_TYPE_STOP_EXIT != msg->msgType)
        {
            ctx->msgId++;
            msg->msgId = ctx->msgId;
        }
        /* Send a message to a queue */
        if (-1 == mq_send(ctx->msgQueueFds[fdIndex], (const char *)msg, sizeof(RdkWmTestMessage), 0))
        {
            RDKWM_TEST_ERROR(("Failed to mq_send fd[%d]:%d for message{ctx@%p id:%d type:%d}! errno:%d(%s)",
                            fdIndex, ctx->msgQueueFds[fdIndex], ctx, msg->msgId, msg->msgType, errno, strerror(errno)));
            goto ret_fail;
        }

        /* Send a message is successful */
        status = false;
        RDKWM_TEST_INFO(("mq_send fd[%d]:%d sent message{ctx@%p id:%d type:%d}",
                        fdIndex, ctx->msgQueueFds[fdIndex], ctx, msg->msgId, msg->msgType));
    }
    else
    {
        RDKWM_TEST_ERROR(("mq_send fd[%d]:%d not valid!", fdIndex, ctx->msgQueueFds[fdIndex]));
    }

ret_fail:
    return status;
}

static bool rdkWmTestReceiveMessage(RdkWmTestAppCtx *ctx, RdkWmTestMessage *msg, uint32_t timeoutInMilliSecs)
{
    bool status = true;
    ssize_t bytes = -1;
    RdkWmTestMsgQueueFdEnum fdIndex = RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_READ;

    if (!ctx)
    {
        RDKWM_TEST_ERROR(("Not valid RDK WM Test context argument!"));
        goto ret_fail;
    }
    else if (!msg)
    {
        RDKWM_TEST_ERROR(("Not valid msg argument!"));
        goto ret_fail;
    }
    else if ((mqd_t)-1 != ctx->msgQueueFds[fdIndex])
    {
        /* Receive a message from message queue */
        if (timeoutInMilliSecs == 0)
        {
             RDKWM_TEST_INFO(("mq_receive timed for infinate time"));

            /* Receive a message from message queue */
            bytes = mq_receive(ctx->msgQueueFds[fdIndex], (char *)msg, sizeof(RdkWmTestMessage), NULL);
        }
        else
        {
            struct timespec tm;
            struct timespec wait;

            clock_gettime(CLOCK_REALTIME, &tm);
            unsigned long totalMilliSeconds;

            totalMilliSeconds = timeoutInMilliSecs + tm.tv_nsec / 1000;
            wait.tv_sec  = tm.tv_sec + totalMilliSeconds / 1000;
            wait.tv_nsec = (totalMilliSeconds % 1000) * 1000000;

            RDKWM_TEST_INFO(("mq_timedreceive timed wait for %dms",timeoutInMilliSecs));

            /* Receive a message from message queue */
            bytes = mq_timedreceive(ctx->msgQueueFds[fdIndex], (char *)msg, sizeof(RdkWmTestMessage), NULL, &wait);
        }
        if ((ssize_t)-1 == bytes)
        {
            RDKWM_TEST_ERROR(("Failed to mq_receive for fd[%d]:%d! errno:%d(%s)",
                        fdIndex, ctx->msgQueueFds[fdIndex], errno, strerror(errno)));
            goto ret_fail;
        }

        /* Receive a message is successful */
        status = false;
        RDKWM_TEST_INFO(("mq_receive fd[%d]:%d %d bytes received message{id:%d type:%d}",
                        fdIndex, ctx->msgQueueFds[fdIndex], bytes, msg->msgId, msg->msgType));
    }
    else
    {
        RDKWM_TEST_ERROR(("mq_receive fd[%d]:%d not valid!", fdIndex, ctx->msgQueueFds[fdIndex]));
    }

ret_fail:
    return status;
}

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
static bool rdkWmShellGetFireboltSurface(RdkWmTestAppCtx *ctx, firebolt_shell_firebolt_surface_type surfaceType, int32_t surfaceId)
{
    bool ret = true;
    RdkWmTestMessage getMsg;
    if ((NULL != ctx) && (ctx->fbShell != NULL) )
    {
        firebolt_shell_get_firebolt_surface(ctx->fbShell, surfaceId, surfaceType);

        if (surfaceType == FIREBOLT_SHELL_FIREBOLT_SURFACE_TYPE_VIDEO)
        {
            if (rdkWmTestReceiveMessage(ctx, &getMsg, RDKWM_TEST_MESSAGEQUEUE_TIMEOUT_MS))
            {
                RDKWM_TEST_ERROR(("rdkWmShellGetFireboltSurface :MessageQueue timeout error@%d", __LINE__));
            }
            else if (RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_VIDEO_SURFACE_ID != getMsg.msgType)
            {
                RDKWM_TEST_ERROR(("Received id:%s ctx@%p ctx->fbSurface@%p unexpected message type:%d!",
                        ctx->display.displayName, ctx, ctx->fbSurface, getMsg.msgType));
            }
            else
            {
                /*Yet To Implement*/
                RDKWM_TEST_INFO(("rdkWmShellGetFireboltSurface : Received HW Video ID: %s",getMsg.u.videoSufaceID));
                ret = false;
            }
        }
        else
        {
            ret = false;
        }
    }
    return ret;
}
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

static int RDKWmtestTermGraphics(RdkWmTestAppCtx *ctx)
{
    int result = 0;

    if(ctx->wlDisplay)
    {
        if (ctx->isOpengl)
        {
            if (eglMakeCurrent(ctx->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
            {
                RDKWM_TEST_ERROR(("Failed to make EGL context current"));
                result = -1;
                goto exit;
            }

            if (eglTerminate(ctx->eglDisplay) == EGL_FALSE)
            {
                RDKWM_TEST_ERROR(("Failed to terminate EGL display"));
                result = -1;
                goto exit;
            }

            eglReleaseThread();
        }
        if (destroySurface(ctx) < 0)
        {
            RDKWM_TEST_ERROR(("Error: Failed to destroy the surface"));
            result = -1;
            goto exit;
        }
    }
exit:
    return result;
}

static bool createSurface(RdkWmTestAppCtx *ctx)
{
    bool result = false;
    ctx->wlSurface = wl_compositor_create_surface(ctx->wlCompositor);
    RDKWM_TEST_INFO(("surface=%p", ctx->wlSurface));
    if (!ctx->wlSurface)
    {
        RDKWM_TEST_ERROR(("unable to create wayland surface"));
        goto exit;
    }
    if(ctx->isOpengl)
    {
        EGLBoolean b;

        ctx->native = wl_egl_window_create(ctx->wlSurface, ctx->display.displayWidth, ctx->display.displayHeight);
        if (!ctx->native)
        {
            RDKWM_TEST_ERROR(("unable to create wl_egl_window"));
            goto exit;
        }
        RDKWM_TEST_INFO(("wl_egl_window %p", ctx->native));

        /*
        * Create a window surface
        */
        ctx->eglSurfaceWindow = eglCreateWindowSurface(ctx->eglDisplay,
                                                      ctx->eglConfig,
                                                      (EGLNativeWindowType)ctx->native,
                                                      NULL);
        if (ctx->eglSurfaceWindow == EGL_NO_SURFACE)
        {
            RDKWM_TEST_INFO(("eglCreateWindowSurface: A: error %X", eglGetError()));
            ctx->eglSurfaceWindow = eglCreateWindowSurface(ctx->eglDisplay,
                                                             ctx->eglConfig,
                                                             (EGLNativeWindowType)NULL,
                                                             NULL);
            if (ctx->eglSurfaceWindow == EGL_NO_SURFACE)
            {
                RDKWM_TEST_ERROR(("eglCreateWindowSurface: B: error %X", eglGetError()));
                goto exit;
            }
        }
        RDKWM_TEST_INFO(("eglCreateWindowSurface: eglSurfaceWindow %p", ctx->eglSurfaceWindow));
        b = eglMakeCurrent(ctx->eglDisplay, ctx->eglSurfaceWindow, ctx->eglSurfaceWindow, ctx->eglContext);
        if (!b)
        {
            RDKWM_TEST_ERROR(("eglMakeCurrent failed: %X", eglGetError()));
            goto exit;
        }

        eglSwapInterval(ctx->eglDisplay, 1);
    }
    else
    {
        ctx->shell_surface = wl_shell_get_shell_surface(ctx->wlShell, ctx->wlSurface);
        if (!ctx->shell_surface)
        {
            RDKWM_TEST_ERROR(("wl_shell_get_shell_surface failed"));
            goto exit;
        }

        wl_shell_surface_add_listener(ctx->shell_surface, &mShellSurfaceListener, NULL);
        wl_shell_surface_set_toplevel(ctx->shell_surface);
        wl_shell_surface_set_user_data(ctx->shell_surface, ctx->wlSurface);
        wl_surface_set_user_data(ctx->wlSurface, NULL);

        ctx->wlSurface= (struct wl_surface *)wl_shell_surface_get_user_data(ctx->shell_surface);
        if (!ctx->wlSurface)
        {
            RDKWM_TEST_ERROR(("Failed to get Wayland surface for surface"));
            goto exit;
        }
    }
    result = true;
exit:
    if (!result)
    {
        if(ctx->isOpengl)
        {
            if (ctx->native)
            {
                wl_egl_window_destroy(ctx->native);
                ctx->native = NULL;
            }
            if (ctx->eglSurfaceWindow != EGL_NO_SURFACE)
            {
                eglDestroySurface(ctx->eglDisplay, ctx->eglSurfaceWindow);
                ctx->eglSurfaceWindow = EGL_NO_SURFACE;
            }
        }
        else
        {
            if (ctx->shell_surface)
            {
                wl_shell_surface_destroy(ctx->shell_surface);
                ctx->shell_surface = NULL;
            }
        }
    }
    return result;
}


static int destroySurface(RdkWmTestAppCtx *ctx)
{
    int result = 0;

    if(ctx->isOpengl)
    {
        if (ctx->eglSurfaceWindow != EGL_NO_SURFACE)
        {
            eglDestroySurface(ctx->eglDisplay, ctx->eglSurfaceWindow);
            ctx->eglSurfaceWindow = EGL_NO_SURFACE;
        }
        if (ctx->native)
        {
            wl_egl_window_destroy(ctx->native);
            ctx->native = NULL;
        }
        ctx->eglDisplay = EGL_NO_DISPLAY;
    }
    else
    {
        if (ctx->shell_surface)
        {
            wl_shell_surface_destroy(ctx->shell_surface);
            ctx->shell_surface = NULL;
        }
    }
    if (ctx->wlSurface)
    {
        wl_surface_destroy(ctx->wlSurface);
        ctx->wlSurface = NULL;
    }
exit:
    return result;
}

static void resizeSurface(RdkWmTestAppCtx *ctx, int dx, int dy, int width, int height)
{
    if(ctx->isOpengl)
    {
        if (ctx->native)
        {
            wl_egl_window_resize(ctx->native, width, height, dx, dy);
        }
    }
}

static const char *vert_shader_text =
    "uniform mat4 rotation;\n"
    "attribute vec4 pos;\n"
    "attribute vec4 color;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_Position = rotation * pos;\n"
    "  v_color = color;\n"
    "}\n";

static const char *frag_shader_text =
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_FragColor = v_color;\n"
    "}\n";

static GLuint createShader(RdkWmTestAppCtx *ctx, GLenum shaderType, const char *shaderSource)
{
    GLuint shader = 0;
    GLint shaderStatus;
    GLsizei length;
    char logText[1000];

    shader = glCreateShader(shaderType);
    if (shader)
    {
        glShaderSource(shader, 1, (const char **)&shaderSource, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderStatus);
        if (!shaderStatus)
        {
            glGetShaderInfoLog(shader, sizeof(logText), &length, logText);
            RDKWM_TEST_ERROR(("Error compiling %s shader: %*s",
                ((shaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment"),
                length,
                logText));
        }
    }
    return shader;
}

static bool RDKWmtestSetupGraphics(RdkWmTestAppCtx *ctx)
{
    bool result = false;
    EGLConfig *eglConfigs = NULL;

    if(ctx->isOpengl)
    {
        EGLint major, minor;
        EGLBoolean b;
        EGLint configCount;
        EGLint attr[32];
        EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
        EGLint ctxAttrib[3];
        int i;
        /*
         * Get default EGL display
         */
        ctx->eglDisplay = eglGetDisplay((NativeDisplayType)ctx->wlDisplay);
        RDKWM_TEST_INFO(("eglDisplay=%p", ctx->eglDisplay));
        if (ctx->eglDisplay == EGL_NO_DISPLAY)
        {
            RDKWM_TEST_ERROR(("EGL not available"));
            goto exit;
        }

        /*
         * Initialize display
         */
        b = eglInitialize(ctx->eglDisplay, &major, &minor);
        if (!b)
        {
            RDKWM_TEST_ERROR(("Unable to initialize EGL display"));
            goto exit;
        }
        RDKWM_TEST_INFO(("eglInitiialize: major: %d minor: %d", major, minor));

        /*
         * Get number of available configurations
         */
        b = eglGetConfigs(ctx->eglDisplay, NULL, 0, &configCount);
        if (!b)
        {
            RDKWM_TEST_ERROR(("Unable to get count of EGL configurations: %X", eglGetError()));
            goto exit;
        }
        RDKWM_TEST_INFO(("Number of EGL configurations: %d", configCount));

        eglConfigs = (EGLConfig*)malloc(configCount*sizeof(EGLConfig));
        if (!eglConfigs)
        {
            RDKWM_TEST_ERROR(("unable to alloc memory for EGL configurations"));
            goto exit;
        }

        i = 0;
        attr[i++] = EGL_RED_SIZE;
        attr[i++] = RED_SIZE;
        attr[i++] = EGL_GREEN_SIZE;
        attr[i++] = GREEN_SIZE;
        attr[i++] = EGL_BLUE_SIZE;
        attr[i++] = BLUE_SIZE;
        attr[i++] = EGL_DEPTH_SIZE;
        attr[i++] = DEPTH_SIZE;
        attr[i++] = EGL_STENCIL_SIZE;
        attr[i++] = 0;
        attr[i++] = EGL_SURFACE_TYPE;
        attr[i++] = EGL_WINDOW_BIT;
        attr[i++] = EGL_RENDERABLE_TYPE;
        attr[i++] = EGL_OPENGL_ES2_BIT;
        attr[i++] = EGL_NONE;

        /*
         * Get a list of configurations that meet or exceed our requirements
         */
        b = eglChooseConfig(ctx->eglDisplay, attr, eglConfigs, configCount, &configCount);
        if (!b)
        {
            RDKWM_TEST_ERROR(("eglChooseConfig failed: %X", eglGetError()));
            goto exit;
        }
        RDKWM_TEST_INFO(("eglChooseConfig: matching configurations: %d", configCount));

        /*
         * Choose a suitable configuration
         */
        for(i = 0; i < configCount; ++i)
        {
            eglGetConfigAttrib(ctx->eglDisplay, eglConfigs[i], EGL_RED_SIZE, &redSize);
            eglGetConfigAttrib(ctx->eglDisplay, eglConfigs[i], EGL_GREEN_SIZE, &greenSize);
            eglGetConfigAttrib(ctx->eglDisplay, eglConfigs[i], EGL_BLUE_SIZE, &blueSize);
            eglGetConfigAttrib(ctx->eglDisplay, eglConfigs[i], EGL_ALPHA_SIZE, &alphaSize);
            eglGetConfigAttrib(ctx->eglDisplay, eglConfigs[i], EGL_DEPTH_SIZE, &depthSize);
            RDKWM_TEST_INFO(("config %d: red: %d green: %d blue: %d alpha: %d depth: %d",
                  i, redSize, greenSize, blueSize, alphaSize, depthSize));
            if ((redSize == RED_SIZE) &&
               (greenSize == GREEN_SIZE) &&
               (blueSize == BLUE_SIZE) &&
               (alphaSize == ALPHA_SIZE) &&
               (depthSize >= DEPTH_SIZE))
            {
                RDKWM_TEST_INFO(("choosing config %d", i));
                break;
            }
        }
        if (i == configCount)
        {
            RDKWM_TEST_ERROR(("no suitable configuration available"));
            goto exit;
        }
        ctx->eglConfig = eglConfigs[i];
        ctxAttrib[0] = EGL_CONTEXT_CLIENT_VERSION;
        ctxAttrib[1] = 2; // ES2
        ctxAttrib[2] = EGL_NONE;

        /*
         * Create an EGL context
         */
        ctx->eglContext = eglCreateContext(ctx->eglDisplay, ctx->eglConfig, EGL_NO_CONTEXT, ctxAttrib);
        if (ctx->eglContext == EGL_NO_CONTEXT)
        {
            RDKWM_TEST_ERROR(("eglCreateContext failed: %X", eglGetError()));
            goto exit;
        }
        RDKWM_TEST_INFO(("eglCreateContext: eglContext %p", ctx->eglContext));
    }
    else
    {
        struct wl_buffer *buffer = NULL;

        if(ctx->buffers == NULL)
        {
            ctx->buffers = (struct wl_buffer **)malloc(sizeof(struct wl_buffer *));
            if (!ctx->buffers)
           {
                RDKWM_TEST_ERROR(("Failed to allocate memory for buffers"));
                goto exit;
            }
            *ctx->buffers = NULL;
        }

        /* Create a new SHM buffer */
        if (createShmBuffer(ctx, &buffer, WL_SHM_FORMAT_XRGB8888) != 0)
        {
            RDKWM_TEST_ERROR(("Failed to create SHM buffer for surface"));
            goto exit;
        }

        /* Ensure the buffer is valid before proceeding */
        if (!buffer)
        {
            RDKWM_TEST_ERROR(("SHM buffer is NULL for surface"));
            goto exit;
        }
        *ctx->buffers = buffer;
    }
    if (!createSurface(ctx))
    {
            RDKWM_TEST_ERROR(("Failed to create the surface"));
            goto exit;
    }
    else
    {
            result = true;
    }
    if(ctx->isOpengl)
    {
        GLuint frag, vert;
        GLuint program;
        GLint status;
        frag = createShader(ctx, GL_FRAGMENT_SHADER, frag_shader_text);
        vert = createShader(ctx, GL_VERTEX_SHADER, vert_shader_text);
        program = glCreateProgram();
        glAttachShader(program, frag);
        glAttachShader(program, vert);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status)
        {
            char log[1000];
            GLsizei len;
            glGetProgramInfoLog(program, 1000, &len, log);
            RDKWM_TEST_ERROR(("Error: linking:\n%*s\n", len, log));
            goto exit;
        }
        glUseProgram(program);
        ctx->gl.pos = 0;
        ctx->gl.col = 1;
        glBindAttribLocation(program, ctx->gl.pos, "pos");
        glBindAttribLocation(program, ctx->gl.col, "color");
        glLinkProgram(program);
        ctx->gl.rotation_uniform = glGetUniformLocation(program, "rotation");
        result = true;
    }

exit:
    if(ctx->isOpengl && eglConfigs)
    {
        free(eglConfigs);
        eglConfigs = NULL;
    }
    return result;
}

static int set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1)
    {
        RDKWM_TEST_ERROR(("Invalid file descriptor"));
        return -1;
    }

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
    {
        RDKWM_TEST_ERROR(("Error getting file descriptor flags"));
        goto err;
    }

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
    {
        RDKWM_TEST_ERROR(("Error setting FD_CLOEXEC flag"));
        goto err;
    }

    return fd;

err:
    close(fd);
    return -1;
}

static int create_tmpfile_cloexec(char *tmpName)
{
    int fd;
    mode_t oldUmask;

    /* Save the current umask and remove all restrictions */
    oldUmask = umask(S_IWOTH);
    fd = mkstemp(tmpName);

    /* Restore the original umask */
    umask(oldUmask);

    /* Check if mkstemp failed */
    if(-1 == fd)
    {
        RDKWM_TEST_ERROR(("Failed to create tmpfile - %s", tmpName));
        goto err;
    }

    /* Set fd to close-on-exec or close */
    fd = set_cloexec_or_close(fd);
    if(-1 == fd)
    {
        RDKWM_TEST_ERROR(("Failed to set CLOEXEC or close fd for %s", tmpName));
        goto err;
    }

    /* Unlink the tmpfile */
    if(0 != unlink(tmpName))
    {
        RDKWM_TEST_ERROR(("Failed to unlink tmpfile - %s", tmpName));
        goto err;
    }
    return fd;

err:
    return -1;
}

static int os_create_anonymous_file(off_t size)
{
    static const char templateFile[] = "/wm-shared-XXXXXX";
    const char *path = NULL;
    char *name = NULL;
    int fd = -1;
    int result = -1;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path)
    {
        RDKWM_TEST_ERROR(("XDG_RUNTIME_DIR environment variable not set"));
        goto exit;
    }

    name = (char*)malloc(strlen(path) + sizeof(templateFile));
    if (!name)
    {
        RDKWM_TEST_ERROR(("Error allocating memory for temporary file name"));
        goto exit;
    }

    strcpy(name, path);
    strcat(name, templateFile);

    fd = create_tmpfile_cloexec(name);
    if (fd < 0)
    {
        RDKWM_TEST_ERROR(("Error creating temporary file"));
        goto exit;
    }

    if (ftruncate(fd, size) < 0)
    {
        RDKWM_TEST_ERROR(("Error setting size of anonymous file with ftruncate"));
        goto exit;
    }

    result = 0;
exit:
    if (name)
        free(name);
    if (result == -1 && fd > 0)
        close(fd);
    return (result == 0) ? fd : -1;
}

static int createShmBuffer(RdkWmTestAppCtx *ctx, struct wl_buffer **buffer, uint32_t format)
{
    struct wl_shm_pool *pool;
    int fd, size, stride;
    int result = -1;

    stride = ctx->display.displayWidth * 4; // 4 bytes per pixel (XRGB8888)
    size = stride * ctx->display.displayHeight;

    fd = os_create_anonymous_file(size);
    if (fd < 0)
    {
        RDKWM_TEST_ERROR(("creating a buffer file for %d B failed: %m\n", size));
        goto exit;
    }

    ctx->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ctx->data == MAP_FAILED)
    {
        RDKWM_TEST_ERROR(("mmap failed: %m\n"));
        goto exit;
    }

    pool = wl_shm_create_pool(ctx->wlShm, fd, size);
    if (!pool)
    {
        RDKWM_TEST_ERROR(("wl_shm_create_pool failed\n"));
        munmap(ctx->data, size);
        ctx->data = NULL;
        goto exit;
    }

    *buffer = wl_shm_pool_create_buffer(pool, 0,
        ctx->display.displayWidth, ctx->display.displayHeight,
        stride, format);

    if (!*buffer)
    {
        RDKWM_TEST_ERROR(("wl_shm_pool_create_buffer failed\n"));
        munmap(ctx->data, size);
        ctx->data = NULL;
        wl_shm_pool_destroy(pool);
        goto exit;
    }

    result = 0;
exit:
    if (fd != -1)
    {
        close(fd);
    }
    if (result == -1 && (ctx->data != MAP_FAILED) && (NULL !=ctx->data))
    {
        munmap(ctx->data, size);
        ctx->data = NULL;
    }
    return result;
}

static void RdkWmtestRenderGraphics(RdkWmTestAppCtx *ctx)
{
    if(ctx->isOpengl)
    {
    // Define vertices for a rectangle (two triangles forming a rectangle)
    static const GLfloat verts[6][2] = {
        { -0.5, -0.5 },  // Bottom left
        {  0.5, -0.5 },  // Bottom right
        { -0.5,  0.5 },  // Top left
        {  0.5, -0.5 },  // Bottom right
        {  0.5,  0.5 },  // Top right
        { -0.5,  0.5 }   // Top left
        };

    // Define colors for each vertex
    static const GLfloat colors[6][4] = {
        { 1.f, 1.f, 0.f, 1.0 },
        { 0.f, 1.f, 1.f, 1.0 },
        { 1.f, 0.f, 1.f, 1.0 },
        { 0.f, 1.f, 1.f, 1.0 },
        { 1.f, 0.f, 1.f, 1.0 },
        { 1.f, 1.f, 0.f, 1.0 }
        };

    GLfloat angle;
    GLfloat rotation[4][4] = {
        { 1.f, 0.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
        { 0.f, 0.f, 1.f, 0.f },
        { 0.f, 0.f, 0.f, 1.f }
        };

        static const uint32_t speed_div = 5;
        EGLint rect[4];

        glViewport(0, 0, ctx->display.outputDisplayWidth, ctx->display.outputDisplayHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ctx->currTime = currentTimeMillis();
        angle = ((ctx->currTime - ctx->startTime) / speed_div) % 360 * M_PI / 180.0;

        rotation[0][0] = cos(angle);
        rotation[0][2] = sin(angle);
        rotation[2][0] = -sin(angle);
        rotation[2][2] = cos(angle);

        glUniformMatrix4fv(ctx->gl.rotation_uniform, 1, GL_FALSE, (GLfloat *)rotation);
        glVertexAttribPointer(ctx->gl.pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
        glVertexAttribPointer(ctx->gl.col, 4, GL_FLOAT, GL_FALSE, 0, colors);

        glEnableVertexAttribArray(ctx->gl.pos);
        glEnableVertexAttribArray(ctx->gl.col);

        /* Draw the rectangle using two triangles*/
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableVertexAttribArray(ctx->gl.pos);
        glDisableVertexAttribArray(ctx->gl.col);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR)
        {
            RDKWM_TEST_ERROR(("renderGL: glGetError() = %X", err));
        }
    }
    else
    {
        if (ctx->data == MAP_FAILED)
        {
            fprintf(stderr, "mmap failed: %m\n");
            return;
        }
        uint32_t *pixel_data = (uint32_t *)ctx->data;

        if (ctx->drawShape == RDKWM_TEST_DRAW_TRIANGLE)
        {
            RDKWM_TEST_ERROR(("renderGL:ctx->drawShape %d",ctx->drawShape));
            /* Calculate dynamic vertex positions based on display size */
            int v1_x = ctx->display.displayWidth / 2;
            int v1_y = ctx->display.displayHeight / 4;

            int v2_x = ctx->display.displayWidth / 10;
            int v2_y = (ctx->display.displayHeight * 9) / 10;

            int v3_x = (ctx->display.displayWidth * 9) / 10;
            int v3_y = v2_y;

            for (int y = v1_y; y <= v2_y; y++)
            {
                float alpha_left = (float)(y - v1_y) / (v2_y - v1_y);
                int start_x = v1_x + alpha_left * (v2_x - v1_x);  // Left edge
                
                float alpha_right = (float)(y - v1_y) / (v3_y - v1_y);
                int end_x = v1_x + alpha_right * (v3_x - v1_x);  // Right edge

                /* Ensure x coordinates are within the valid display width*/
                if (start_x < 0)
                    start_x = 0;
                if (end_x >= ctx->display.displayWidth)
                    end_x = ctx->display.displayWidth - 1;

                /* Draw horizontal line between start_x and end_x for this scanline*/
                for (int x = start_x; x <= end_x; x++)
                {
                    // Ensure x is within display width bounds
                    if (x >= 0 && x < ctx->display.displayWidth)
                    {
                        pixel_data[y * ctx->display.displayWidth + x] = 0xFFFF0000;  // Red color (ARGB)
                    }
                }
            }
        }
        else
        {
            int centerRectWidth = ctx->display.displayWidth/2;
            int centerRectHeight = ctx->display.displayHeight/2;
            /* Drawing rectangle based on input width and height*/
            int rect_x = (ctx->display.displayWidth - centerRectWidth) / 2;  // Center the rectangle horizontally
            int rect_y = (ctx->display.displayHeight - centerRectHeight) / 2;  // Center the rectangle vertically

            for (int y = rect_y; y < rect_y + centerRectHeight; y++)
            {
                for (int x = rect_x; x < rect_x + centerRectWidth; x++)
                {
                    pixel_data[y * ctx->display.displayWidth + x] = 0xFF0000FF;  // Blue color (ARGB)
                }
            }
        }
    }
}
