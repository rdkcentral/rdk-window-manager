#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <ctype.h>
#include "wayland-client.h"

#include "firebolt_surface_protocol_client.h"

typedef struct _AppCtx
{
   struct wl_display *display;
   struct wl_registry *registry;
   struct wl_compositor *compositor;
   struct wl_surface *surface;
   struct firebolt_surface *fireboltSurface;
   //struct firebolt_shell *fireboltShell;
   //struct firebolt_wm *fireboltWm;

   char const *clientName;

} AppCtx;

static void registryHandleGlobal(void *data,
                                 struct wl_registry *registry, uint32_t id,
                                 const char *interface, uint32_t version);
static void registryHandleGlobalRemove(void *data,
                                       struct wl_registry *registry,
                                       uint32_t name);

static const struct wl_registry_listener registryListener =
{
   registryHandleGlobal,
   registryHandleGlobalRemove
};

static void registryHandleGlobal(void *data,
                                 struct wl_registry *registry, uint32_t id,
                                 const char *interface, uint32_t version)
{
   AppCtx *ctx = (AppCtx*)data;
   int len;

   printf("firebolt_surface_extension_test: registry: id %d interface (%s) version %d\n", id, interface, version);
/*
   len = strlen(interface);
   if ((len == 13) && !strncmp(interface, "wl_compositor", len)) {
      ctx->compositor = (struct wl_compositor*)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
      printf("compositor %p\n", ctx->compositor);
   }
   else if ((len == strlen("firebolt_shell")) && !strncmp(interface, "firebolt_shell", len)) {
      ctx->fireboltSurface = (struct firebolt_shell*)wl_registry_bind(registry, id, &firebolt_shell_interface, 1);
      printf("firebolt_shell %p\n", ctx->fireboltShell);
   }
   else if ((len == strlen("firebolt_surface")) && !strncmp(interface, "firebolt_surface", len)) {
      ctx->fireboltSurface = (struct firebolt_surface*)wl_registry_bind(registry, id, &firebolt_surface_interface, 1);
      printf("firebolt_surface %p\n", ctx->fireboltSurface);
   }
   else if ((len == strlen("firebolt_wm")) && !strncmp(interface, "firebolt_wm", len)) {
      ctx->fireboltSurface = (struct firebolt_wm*)wl_registry_bind(registry, id, &firebolt_wm_interface, 1);
      printf("firebolt_wm %p\n", ctx->fireboltWm);
   }
*/
      ctx->fireboltSurface = (struct firebolt_surface*)wl_registry_bind(registry, id, &firebolt_surface_interface, 1);
      printf("firebolt_surface %p\n", ctx->fireboltSurface);

}

static void registryHandleGlobalRemove(void *data,
                                       struct wl_registry *registry,
                                       uint32_t name)
{
}

void callApi(AppCtx *ctx)
{
   if (!ctx->fireboltSurface)
   {
      printf("firebolt_surface_extension_test ctx->fireboltSurface \n");
      return;
   }
   firebolt_surface_set_name(ctx->fireboltSurface ,"FireboltApp");
   firebolt_surface_set_visible(ctx->fireboltSurface ,1);
   firebolt_surface_set_bounds(ctx->fireboltSurface ,1,1,1080,920);
   firebolt_surface_set_crop(ctx->fireboltSurface ,0.5,0,0.5,0.5);
   firebolt_surface_set_zorder(ctx->fireboltSurface ,0.1);
   firebolt_surface_set_opacity(ctx->fireboltSurface ,0.2);
   
   printf("Inside callApi firebolt_surface_extension_test\n");
   return;
}

int main(int argc, char** argv)
{
   AppCtx ctx;
   struct wl_display *display = 0;
   struct wl_registry *registry = 0;
   const char *display_name = 0;

   printf("firebolt_surface_extension_test\n");

   memset(&ctx, 0, sizeof(AppCtx));
   
   for (int i = 1; i < argc; ++i)
   {
      if (!strcmp((const char*)argv[i], "--display"))
      {
         if (i+1 < argc)
         {
            ++i;
            display_name = argv[i];
         }
      }
      else if (!strcmp((const char*)argv[i], "--client"))
      {
         if (i+1 < argc)
         {
            ++i;
            ctx.clientName = argv[i];
         }
      }
   }
   if (!ctx.clientName)
   {
      ctx.clientName = "fireboltApp";
   }
   printf("controling client: %s\n", ctx.clientName);

   if (display_name)
   {
      printf("calling wl_display_connect for display name %s\n", display_name);
   }
   else
   {
      printf("calling wl_display_connect for default display\n");
   }

   display = wl_display_connect(nullptr);

   printf("wl_display_connect: display=%p\n", display);
   if (!display)
   {
      printf("error: unable to connect to primary display\n");
      return 1;
   }

   printf("calling wl_display_get_registry\n");
   registry = wl_display_get_registry(display);

   printf("wl_display_get_registry: registry=%p\n", registry);
   if (!registry)
   {
      printf("error: unable to get display registry\n");
      return 1;
   }

   ctx.display = display;
   ctx.registry = registry;

   wl_registry_add_listener(registry, &registryListener, &ctx);

   printf("callApi\n");
   callApi(&ctx);
   
   printf("callApi Done \n");
   return 0;
}
