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

#ifndef _TESTRDKWM_H
#define _TESTRDKWM_H
#include <unistd.h>
#include <ctype.h>
#include <mqueue.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <list>
#include "wayland-client.h"
#include "wayland-egl.h"

#define RDKWM_TESTAPP_DEBUG

#ifdef RDKWM_TESTAPP_DEBUG
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif /* !_GNU_SOURCE */
#include <sys/syscall.h>
#endif /* RDKWM_TESTAPP_DEBUG */

/* RDK Window manager firebolt wayland extensions headers */
#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
#include "firebolt_surface_protocol_client.h"
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
#include "firebolt_shell_protocol_client.h"
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
#include "firebolt_wm_protocol_client.h"
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

#define RDKWM_WAYLAND_DISPLAY_NAME               "rdkwindowmanager_display"
#define RDKWM_TEST_NAME_MAXSIZE                  (32)
#define RDKWM_TEST_REPORT_STRING_MAXSIZE         (32)
#define RDKWM_TEST_MESSAGEQUEUE_MAX_MESSAGES     (10) /* The default value for msg_max is 10 */
#define RDKWM_TEST_REPORT_FILE_SIZE              (128)
#define RDKWM_TEST_LOG_MESSAGE_MAXSIZE           (128)
#define RDKWM_TEST_API_NAME_MAXSIZE              (64)
#define RDKWM_TEST_AUTHORIZATION_HEADER_MAX_SIZE (256)
#define RDK_TEST_NUM_ENTRIES_MAXSIZE             (5)

#ifdef RDKWM_TESTAPP_DEBUG
#define RDKWM_TEST_INFO(aMessage)               do {printf( "[INFO] %s: PID-%04d Thread-%04lu F:%s <%s @%05d>: ", RDKWM_TESTAPP_NAME, getpid(), syscall(SYS_gettid), basename(__FILE__), __FUNCTION__, __LINE__); printf aMessage; printf( "\n");}while(0)
#define RDKWM_TEST_WARN(aMessage)               do {printf( "[WARN] %s: PID-%04d Thread-%04lu F:%s <%s @%05d>: ", RDKWM_TESTAPP_NAME, getpid(), syscall(SYS_gettid), basename(__FILE__), __FUNCTION__, __LINE__); printf aMessage; printf( "\n");}while(0)
#define RDKWM_TEST_ERROR(aMessage)              do {printf( "[ERROR] %s: PID-%04d Thread-%04lu F:%s <%s @%05d>: ", RDKWM_TESTAPP_NAME, getpid(), syscall(SYS_gettid), basename(__FILE__), __FUNCTION__, __LINE__); printf aMessage; printf( "\n");}while(0)
#else
#define RDKWM_TEST_INFO(aMessage)
#define RDKWM_TEST_WARN(aMessage)
#define RDKWM_TEST_ERROR(aMessage)
#endif /* RDKWM_TESTAPP_DEBUG */

/* Enumeration of RDK WM Test Message Queue File descriptor */
typedef enum _RdkWmTestMsgQueueFdEnum
{
    RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_WRITE = 0,
    RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_READ  = 1,
    RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_MAX,
} RdkWmTestMsgQueueFdEnum;

/* Enumeration of RDK WM Test message type */
typedef enum _RdkWmTestMessageType
{
    RDKWM_TEST_MESSAGE_TYPE_UNKNOWN = 0,
    RDKWM_TEST_MESSAGE_TYPE_INTEGER,
    RDKWM_TEST_MESSAGE_TYPE_STRING,
    RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_PROPERTIES,
    RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_GET_CLIENTS,
    RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_FOCUSED_CLIENT,
    RDKWM_TEST_MESSAGE_TYPE_FBWM_CB_CLIENT_OWNER,
    RDKWM_TEST_MESSAGE_TYPE_FBSURFACE_CB_PROPERTIES,
    RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_VIDEO_SURFACE_ID,
    RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_ON_FOCUS,
    RDKWM_TEST_MESSAGE_TYPE_FBSHELL_CB_ON_BLUR,
    RDKWM_TEST_MESSAGE_TYPE_WL_CB_OUTPUT_MODE,
    RDKWM_TEST_MESSAGE_TYPE_STOP_EXIT,
    RDKWM_TEST_MESSAGE_TYPE_MAX
} RdkWmTestMessageTypeEnum;

/* Enumeration of RDK WM Test result status */
typedef enum _RdkWmTestReturnStatus
{
    RDKWM_TEST_RESULT_UNKNOWN = 0,
    RDKWM_TEST_RESULT_PASS,
    RDKWM_TEST_RESULT_FAIL,
    RDKWM_TEST_RESULT_FORCE_STOP,
    RDKWM_TEST_RESULT_MAX
} RdkWmTestReturnStatus;

/* Enumeration of RDK WM Test report File format Type */
typedef enum RdkWmTestReportFormatType
{
    RDKWM_TEST_REPORT_FORMAT_UNKNOWN = 0,
    RDKWM_TEST_REPORT_FORMAT_PLAIN_TEXT
} RdkWmTestReportFileType;


#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
typedef struct _RdkTestFbWmClientInfo
{
    char        id[RDKWM_TEST_NAME_MAXSIZE];
    int32_t     x;
    int32_t     y;
    int32_t     width;
    int32_t     height;
    double      opacity;
    int32_t     zorder;
    int32_t     visible;
    double      cropX;
    double      cropY;
    double      cropWidth;
    double      cropHeight;
    int32_t     texture;
} RdkTestFbWmClientInfo;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
typedef struct _RdkTestFbSurfaceInfo
{
    char        name[RDKWM_TEST_NAME_MAXSIZE];
    int32_t     x;
    int32_t     y;
    int32_t     width;
    int32_t     height;
    double      opacity;
    int32_t     zorder;
    int32_t     visible;
    double      cropX;
    double      cropY;
    double      cropWidth;
    double      cropHeight;
} RdkTestFbSurfaceInfo;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

/* Enumeration of RDK WM Test input type */
typedef enum _RdkWmTestInputType
{
    RDKWM_TEST_INPUT_PARAM_TYPE_UNKNOWN = 0,
    RDKWM_TEST_INPUT_PARAM_TYPE_NOT_NEEDED,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_VISIBLITY_TOGGLE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VISIBLITY_TOGGLE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_BOUNDS_TOGGLE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_BOUNDS_TOGGLE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_GET_FOCUSED_CLIENT,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_GET_FOCUSED_CLIENT,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CROP,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CROP_TOGGLE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_OPACITY,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_CLIENT_DISPLAY_BOUNDS,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_CROP_TOGGLE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_SET_ZORDER,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_ZORDER,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_NAME,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_SET_OPACITY,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_VIDEO_HOLE_PUNCH,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSURFACE_DESTROY,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBSHELL_GET_FB_SURFACE,
    RDKWM_TEST_INPUT_PARAM_TYPE_FBWM_CLIENT_OWNERID,
    RDKWM_TEST_INPUT_PARAM_MAX
} RdkWmTestInputType;

typedef enum _RdkWmTestConditions
{
    RDKWM_TEST_CONDITIONS_NONE          = 0, /* Test doesn't need any preconditions to be set , and/or Expected to run on previously configured conditions */

    RDKWM_TEST_RUNS_ON_HIDDEN_MODE      = (1 << 0), /* Test expected to run on hidden mode, so this enum value enforces test API to set visibility property to OFF */
    RDKWM_TEST_RUNS_ON_VISIBILITY_MODE  = (1 << 1), /* Test expected to run on visible mode, so this enum value enforces test API to set visibility property to ON  */
    RDKWM_TEST_RUNS_ON_TRANSPARENT_MODE = (1 << 2), /* Test expected to run on transparent mode, , so this enum value enforces test API to set opacity property to OFF */
    RDKWM_TEST_RUNS_ON_OPAQUE_MODE      = (1 << 3), /* Test expected to run on opaque mode, so this enum value enforces test API to set opacity property to ON */
    RDKWM_TEST_RUNS_ON_CROPPED_MODE     = (1 << 4), /* Test expected to run on crop mode(values provided by user), so this enum value enforces test API to set crop property to ON */
    RDKWM_TEST_RESET_CROPPED_MODE       = (1 << 5), /* Test expected to reset crop mode to default values, so this enum value enforces test API to set crop property to OFF */
    RDKWM_TEST_RUNS_ON_BOUNDS_MODE      = (1 << 6), /* Test expected to run on bound mode(values provided by user), so this enum value enforces test API to set bounds property to ON */
    RDKWM_TEST_RESET_BOUNDS_MODE        = (1 << 7), /* Test expected to reset bounds mode to default values, so this enum value enforces test API to set bounds property to OFF */
    RDKWM_TEST_CONVERT_SURFACE_TYPE     = (1 << 8)  /* Test expected to convert provided surface type to firebolt surface */
} RdkWmTestConditions;


typedef struct _RdkWindowProperties
{
    int32_t     x;
    int32_t     y;
    int32_t     width;
    int32_t     height;
}RdkWindowProperties;

typedef struct _RdkTestWlOutputModeInfo
{
    int32_t     width;
    int32_t     height;
} RdkTestWlOutputModeInfo;

typedef struct _RdkTestOpacity
{
    int numEntries;
    double values[RDK_TEST_NUM_ENTRIES_MAXSIZE];
}RdkTestOpacity;

typedef struct _RdkWMTestProperties
{
    char                       displayId[RDKWM_TEST_NAME_MAXSIZE];
    char                       surfaceName[RDKWM_TEST_NAME_MAXSIZE];
    int32_t                    x;
    int32_t                    y;
    int32_t                    width;
    int32_t                    height;
    RdkTestOpacity             opacity; /* Only one value of opacity supported in precondition */
    int32_t                    zorder;
    int32_t                    visible;
    double                     cropX;
    double                     cropY;
    double                     cropWidth;
    double                     cropHeight;
    int32_t                    texture;
    firebolt_shell_firebolt_surface_type    surfaceType;
    int32_t                                 surfaceId;
} RdkWMTestProperties;

typedef struct _RdkWmTestPrerequisites
{
    uint32_t                condition; /* Combination of RdkWmTestConditions value(s) to be defined for visibililty, opacity, crop, bounds properties */
    RdkWMTestProperties     property;  /* Associated test properties and its values to be passed during perquisite call */
} RdkWmTestPrerequisites;

/* Surface structure */
typedef struct _RdkWmTestSurface
{

    firebolt_shell_firebolt_surface_type    surfaceType;
    int32_t                                 surfaceId;
    const char                              *name;
    int32_t                                 zOrder;
    double                                  opacity;
    int32_t                                 x;
    int32_t                                 y;
    int32_t                                 width;
    int32_t                                 height;

    /* 
     * If surface visibility state change to OFF -> true;
     * If surface visibility state change to ON -> false;
     */
    bool isHidden;

    /*
     * If surface opacity value set it to 0.0 -> true;
     * other opacity values -> false;
     */
    bool isTransparent;

    /*
     * Any surface crop settings orther than full surface size(x,y,w,h) -> true;
     * default size (or) set back to full surface size(0,0,0,0) -> false;
     */
    bool isCropMode;

    /*
     * Any surface bound settings orther than full surface size(x,y,w,h) -> true;
     * default size (or) set back to full display size -> false;
     */
    bool isBoundMode;

  /*
   * To indicate the firebolt surface type each time it gets converted via rdkWmShellGetFireboltSurface()
   */
firebolt_shell_firebolt_surface_type fbSurfaceType;
} RdkWmTestSurface;

/* WM Display structure */
typedef struct _RdkWmTestWmDisplay
{
    char       clientName[RDKWM_TEST_NAME_MAXSIZE];
    const char *displayName;
    uint32_t   displayWidth;
    uint32_t   displayHeight;
    uint32_t   outputDisplayWidth;
    uint32_t   outputDisplayHeight;
    bool       virtualDisplay;
    uint32_t   virtualWidth;
    uint32_t   virtualHeight;
    bool       topmost;
    bool       focus;

    /* For Wm SetBounds if callback for outputMode is called */
    bool       bNotifyOutputModeEvent;
    /* 
     * If WM visibility state change to OFF -> true;
     * If WM visibility state change to ON -> false;
     */
    bool isHidden;

    /*
     * If WM opacity value set it to 0.0 -> true;
     * other opacity values -> false;
     */
    bool isTransparent;

    /*
     * Any WM crop settings orther than full display size(x,y,w,h) -> true;
     * default size (0,0,0,0) -> false;
     */
    bool isCropMode;

    /*
     * Any WM Bound settings orther than full display size(x,y,w,h) -> true;
     * default size (or) set back to full display size  -> false;
     */
    bool isBoundMode;

    /* Surface properties */
    RdkWmTestSurface surface;
} RdkWmTestWmDisplay;

typedef struct _RdkWmTestInputParam
{
    RdkWmTestInputType          inputParamType;    /* Enum value for input type */
    union
    {
        RdkWindowProperties     wmProperties;
        int32_t                 zOrder;
        int32_t                 visiblity;
        RdkTestOpacity          opacity;
        RdkWmTestSurface surface;
        int32_t                 ownerId;
    } u;
    RdkWmTestPrerequisites      prerequisite;   /* Test perquisite conditions and associated test properties */
}RdkWmTestInputParam;

typedef struct _RdkWmTestRunStatus
{
    RdkWmTestReturnStatus   testResult;
    struct timespec         testStart;
    struct timespec         testEnd;
    std::list<std::string>  message;
    std::list<std::string>  wmLogMessage;
}RdkWmTestRunStatus;


#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

/* Struct of RDK WM Test message */
typedef struct _RdkWmTestMessage
{
    uint32_t                    msgId;      /* msgId of callback message */
    RdkWmTestMessageTypeEnum    msgType;    /* Enum value for callback message type */
    union
    {
        int     value;
        int32_t fbWmClientOwnerId;
        char    string[256];
        char    videoSufaceID[256];
#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
        RdkTestFbWmClientInfo fbWmClientInfo;
        char    fbWmClients[256];
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
        RdkTestFbSurfaceInfo fbSurfaceInfo;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */
        RdkTestWlOutputModeInfo  wlOutputInfo;
    } u;
} RdkWmTestMessage;

typedef struct _RdkWmTestAppCtx RdkWmTestAppCtx;
typedef struct _RdkWmTestcase RdkWmTestcase;
typedef std::map<char*,RdkWmTestcase> RdkWmTestList;

typedef RdkWmTestReturnStatus (*RdkWmTestcaseFunc)(RdkWmTestAppCtx *ctx, RdkWmTestcase *testCase);

typedef struct _RdkWmTestcase
{
   const char *name;
   const char *desc;
   RdkWmTestcaseFunc func;
   RdkWmTestInputParam  testInputs;
   RdkWmTestRunStatus  runStatus;
} RdkWmTestcase;

/* Enumeration of RDK WM Test cURL param type */
typedef enum _RdkWmTestCurlThunderPluginEnum
{
    RDKWM_TEST_PARAM_TYPE_NONE = 0,
    RDKWM_TEST_PARAM_TYPE_CREATEDISPLAY
} RdkWmTestCurlThunderPluginEnum;

typedef enum _RdkWmTestCurlMethodEnum {
    RDKWM_TEST_GETCLIENTS_ENUM,
    RDKWM_TEST_ACTIVATE_METHOD_ENUM,
    RDKWM_TEST_CREATEDISPLAY_METHOD_ENUM,
    RDKWM_TEST_UNKNOWN
} RdkWmTestCurlMethodEnum;

typedef enum _RdkWmTestDrawImageEnum
{
    RDKWM_TEST_DRAW_RECTANGLE = 0,
    RDKWM_TEST_DRAW_TRIANGLE
} RdkWmTestDrawImageEnum;

/* Struct of Rdk WM test context detail */
typedef struct _RdkWmTestAppCtx
{
    struct wl_shm          *wlShm;
    struct wl_shell        *wlShell;
    struct wl_display      *wlDisplay;
    struct wl_registry     *wlRegistry;
    struct wl_compositor   *wlCompositor;
    struct wl_surface      *wlSurface;
    struct wl_callback     *frameCallback;
    struct wl_output       *wlOutput;
    int                     drawShape;
    long long               startTime;
    long long               currTime;
    bool                    needRedraw;
    mqd_t                   msgQueueFds[RDKWM_TEST_MESSAGEQUEUE_FD_INDEX_MAX];
    uint32_t                msgId;
    pthread_t               executorPthread;
    int32_t                *returnStatus;
    bool                    bExecutorActive;
    char                    msgQueueName[RDKWM_TEST_NAME_MAXSIZE];
    bool                    isOpengl;
    struct wl_egl_window   *native;
    EGLDisplay              eglDisplay;
    EGLConfig               eglConfig;
    EGLSurface              eglSurfaceWindow;
    EGLContext              eglContext;
    struct
    {
        GLuint rotation_uniform;
        GLuint pos;
        GLuint col;
    } gl;
    struct wl_shell_surface *shell_surface;
    struct wl_buffer **buffers;
    void *data;
    /* Display properties */
    RdkWmTestWmDisplay display;
#ifdef RDK_WINDOW_MANAGER_BUILD_EXTENSIONS
#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION
    struct firebolt_surface  *fbSurface;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SURFACE_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION
    struct firebolt_shell    *fbShell;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_SHELL_EXTENSION */

#ifdef RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION
    struct firebolt_wm       *fbWm;
#endif /* RDK_WINDOW_MANAGER_BUILD_FIREBOLT_WM_EXTENSION */
#endif /* RDK_WINDOW_MANAGER_BUILD_EXTENSIONS */

    RdkWmTestList           testList;
    uint32_t                passCount;
    uint32_t                failCount;
    char                    logMessage[RDKWM_TEST_LOG_MESSAGE_MAXSIZE];
} RdkWmTestAppCtx;

#endif /* !_TESTRDKWM_H */
