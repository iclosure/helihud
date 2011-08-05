#if IBM
#include <windows.h>
#endif
#if LIN
#include <GL/gl.h>
#else
#if __GNUC__
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include <string.h>
#include "XPLMDisplay.h"
#include "XPLMDataAccess.h"
#include "XPLMGraphics.h"
#include "drawing.h"
#include "fonts.h"
#include "data_access.h"

#if IBM
BOOL APIENTRY DllMain( HANDLE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
  switch (ul_reason_for_call)
  {
  case DLL_PROCESS_ATTACH:
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
#endif

int status;
HudConfig* config;

int MyDrawCallback(
  XPLMDrawingPhase     inPhase,
  int                  inIsBefore,
  void *               inRefcon);

PLUGIN_API int XPluginStart(
  char *  outName,
  char *  outSig,
  char *  outDesc)
{
  /* First record our plugin information. */
  strcpy(outName, "HeliHUD");
  strcpy(outSig, "PetrMedek.tools.helihud");
  strcpy(outDesc, "HUD for helicopters flying");

  config = initConfig();
  initGlResources();
  if ((status = initDataRefs()) == 0)
    XPLMRegisterDrawCallback(
      MyDrawCallback,
      xplm_Phase_Window,  /* Draw when sim is doing windows */
      1,   /* Before plugin windows */
      NULL);   /* No refcon needed */


  return 1;
}

PLUGIN_API void XPluginStop(void)
{
  unregisterData();
  if (status == 0)
    destroyGlResources();
  XPLMUnregisterDrawCallback(
    MyDrawCallback,
    xplm_Phase_Window,
    1,
    NULL);
}

PLUGIN_API void XPluginDisable(void)
{
}

PLUGIN_API int XPluginEnable(void)
{
  return 1;
}

PLUGIN_API void XPluginReceiveMessage(
  XPLMPluginID inFromWho,
  long   inMessage,
  void *   inParam)
{
}

/*
 * MyDrawCallback
 * 
 * This is the actual drawing callback that does the work; for us it will 
 * be called after X-Plane has drawn its 3-d objects.  The coordinate system
 * is 'normal' for 3-d drawing, meaning 0,0,0 is at the earth's surface at the
 * lat/lon reference point, with +Y = up, +Z = South, +X = East.  (Note that
 * these relationships are only true at 0,0,0 due to the Earth's curvature!!)
 * 
 * Drawing hooks that draw before X-Plane return 1 to let X-Plane draw or 0
 * to inhibit drawing.  For drawing hooks that run after X-Plane, this return
 * value is ignored but we will return 1 anyway.
 *
 */

int MyDrawCallback(
  XPLMDrawingPhase     inPhase,
  int                  inIsBefore,
  void *               inRefcon)
{

  if (config->visible == 0)
    return 1;
  // turn off blending
  XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);

  /* Do the actual drawing.  use GL_LINES to draw sets of discrete lines.
   * Each one will go 100 meters in any direction from the plane. */
  TranslateToCenter();
  DrawCenterBox();
  DrawNoseBox(getPitch(), getRoll());
  DrawMovementArrow(getTrueHeading(), getVX(), getVY(), getVZ());
  DrawVerticalSpeedIndicator(getVV());
  DrawLandingBars(getRadarAltitude());
  DrawSpeedIndicator(getIAS());
  DrawWind(getWindDirection(), getWindSpeed(), getHeading());
  DrawTexts();
  return 1;
}