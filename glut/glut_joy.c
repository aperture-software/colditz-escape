
/* Copyright (c) Mark J. Kilgard, 1997, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>  /* Win32 Multimedia API header. */
#endif

#include "glutint.h"

#ifdef XBOX360_CONTROLLER_DPAD_SUPPORT
/* Table used to insert an XBox360 DPAD controller
   position into the button mask */
DWORD XBox360DpadMask[9] = { 0x10000000, 0x30000000,
     0x20000000, 0x60000000, 0x40000000, 0xC0000000,
     0x80000000, 0x90000000, 0x00000000 };
#endif

/* CENTRY */
void APIENTRY
glutJoystickFunc(GLUTjoystickCB joystickFunc, int pollInterval)
{
#ifdef _WIN32
  if (joystickFunc && (pollInterval > 0)) {
    if (__glutCurrentWindow->entryState == WM_SETFOCUS) {
      MMRESULT result;

      /* Capture joystick focus if current window has
         focus now. */
      result = joySetCapture(__glutCurrentWindow->win,
        JOYSTICKID1, 0, TRUE);
      if (result == JOYERR_NOERROR) {
        (void) joySetThreshold(JOYSTICKID1, pollInterval);
      }
    }
    __glutCurrentWindow->joyPollInterval = pollInterval;
  } else {
    /* Release joystick focus if current window has
       focus now. */
    if (__glutCurrentWindow->joystick
      && (__glutCurrentWindow->joyPollInterval > 0)
      && (__glutCurrentWindow->entryState == WM_SETFOCUS)) {
      (void) joyReleaseCapture(JOYSTICKID1);
    }
    __glutCurrentWindow->joyPollInterval = 0;
  }
  __glutCurrentWindow->joystick = joystickFunc;
#else
  /* XXX No support currently for X11 joysticks. */
#endif
}

void APIENTRY
glutForceJoystickFunc(void)
{
#ifdef _WIN32
  if (__glutCurrentWindow->joystick) {
    JOYINFOEX jix;
    MMRESULT res;
    int x, y, z;

    /* Poll the joystick. */
    jix.dwSize = sizeof(jix);
    jix.dwFlags = JOY_RETURNALL;
    res = joyGetPosEx(JOYSTICKID1,&jix);
    if (res == JOYERR_NOERROR) {

      /* Convert to int for scaling. */
      x = jix.dwXpos;
      y = jix.dwYpos;
      z = jix.dwZpos;

#ifdef XBOX360_CONTROLLER_DPAD_SUPPORT
      /* 0x1194 = 1/8th position change around the DPAD */
      jix.dwPOV /= 0x1194;
      /* Rest state of DPAD is 0x0000FFFF which is greater
         than 8 x 0x1194 */
      if (jix.dwPOV >= 8)
        jix.dwPOV = 8;
      jix.dwButtons |= XBox360DpadMask[jix.dwPOV];
#endif

#define SCALE(v)  ((int) ((v - 32767)/32.768))

      __glutCurrentWindow->joystick(jix.dwButtons,
        SCALE(x), SCALE(y), SCALE(z));
    }
  }
#else
  /* XXX No support currently for X11 joysticks. */
#endif
}

/* ENDCENTRY */
