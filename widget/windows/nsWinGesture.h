/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WinGesture_h__
#define WinGesture_h__

/*
 * nsWinGesture - Touch input handling for tablet displays.
 */

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0601)
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0601
#endif
#if !defined(WINVER) || (WINVER < 0x0601)
#  undef WINVER
#  define WINVER 0x0601
#endif
#include "nsdefs.h"
#include <tpcshrd.h>
#include "nsPoint.h"
#include "mozilla/EventForwards.h"
#include "mozilla/TouchEvents.h"
#include <math.h>

// Some builds pull in windows.h before WINVER/_WIN32_WINNT are raised,
// which hides Win7+ touch/gesture declarations. Provide compatibility
// declarations so this code still compiles; functions are resolved at runtime.
#ifndef WM_GESTURE
#  define WM_GESTURE 0x0119
#endif
#ifndef WM_GESTURENOTIFY
#  define WM_GESTURENOTIFY 0x011A
#endif
#ifndef WM_TOUCH
#  define WM_TOUCH 0x0240
#endif

#ifndef TOUCHEVENTF_MOVE
typedef HANDLE HTOUCHINPUT;
typedef struct tagTOUCHINPUT {
  LONG x;
  LONG y;
  HANDLE hSource;
  DWORD dwID;
  DWORD dwFlags;
  DWORD dwMask;
  DWORD dwTime;
  ULONG_PTR dwExtraInfo;
  DWORD cxContact;
  DWORD cyContact;
} TOUCHINPUT, *PTOUCHINPUT;

#  define TOUCHEVENTF_MOVE 0x0001
#  define TOUCHEVENTF_DOWN 0x0002
#  define TOUCHEVENTF_UP 0x0004
#  define TOUCHINPUTMASKF_CONTACTAREA 0x0004
#  define TWF_WANTPALM 0x00000002
#  define TOUCH_COORD_TO_PIXEL(l) ((l) / 100)
#endif

#ifndef GID_BEGIN
typedef HANDLE HGESTUREINFO;
typedef struct tagGESTUREINFO {
  UINT cbSize;
  DWORD dwFlags;
  DWORD dwID;
  HWND hwndTarget;
  POINTS ptsLocation;
  DWORD dwInstanceID;
  DWORD dwSequenceID;
  ULONGLONG ullArguments;
  UINT cbExtraArgs;
} GESTUREINFO, *PGESTUREINFO;

typedef struct tagGESTURENOTIFYSTRUCT {
  UINT cbSize;
  DWORD dwFlags;
  HWND hwndTarget;
  POINTS ptsLocation;
  DWORD dwInstanceID;
} GESTURENOTIFYSTRUCT, *PGESTURENOTIFYSTRUCT;

typedef struct tagGESTURECONFIG {
  DWORD dwID;
  DWORD dwWant;
  DWORD dwBlock;
} GESTURECONFIG, *PGESTURECONFIG;

#  define GID_BEGIN 1
#  define GID_END 2
#  define GID_ZOOM 3
#  define GID_PAN 4
#  define GID_ROTATE 5
#  define GID_TWOFINGERTAP 6
#  define GID_PRESSANDTAP 7

#  define GC_ZOOM 0x00000001
#  define GC_PAN 0x00000001
#  define GC_PAN_WITH_SINGLE_FINGER_VERTICALLY 0x00000002
#  define GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY 0x00000004
#  define GC_PAN_WITH_GUTTER 0x00000008
#  define GC_PAN_WITH_INERTIA 0x00000010
#  define GC_ROTATE 0x00000001
#  define GC_TWOFINGERTAP 0x00000001
#  define GC_PRESSANDTAP 0x00000001

#  define GF_BEGIN 0x00000001
#  define GF_INERTIA 0x00000002
#  define GF_END 0x00000004

#  define GID_ROTATE_ANGLE_FROM_ARGUMENT(arg) \
    ((((double)(arg)) / 65535.0) * 4.0 * M_PI - 2.0 * M_PI)
#endif

// WM_TABLET_QUERYSYSTEMGESTURESTATUS return values
#define TABLET_ROTATE_GESTURE_ENABLE    0x02000000

class nsPointWin : public nsIntPoint
{
public:
   nsPointWin& operator=(const POINTS& aPoint) {
     x = aPoint.x; y = aPoint.y;
     return *this;
   }
   nsPointWin& operator=(const POINT& aPoint) {
     x = aPoint.x; y = aPoint.y;
     return *this;
   }
   nsPointWin& operator=(int val) {
     x = y = val;
     return *this;
   }
   void ScreenToClient(HWND hWnd) {
     POINT tmp;
     tmp.x = x; tmp.y = y;
     ::ScreenToClient(hWnd, &tmp);
     *this = tmp;
   }
};

class nsWinGesture
{
public:
  nsWinGesture();

public:
  bool SetWinGestureSupport(HWND hWnd, mozilla::WidgetGestureNotifyEvent::PanDirection aDirection);
  bool ShutdownWinGestureSupport();
  bool RegisterTouchWindow(HWND hWnd);
  bool UnregisterTouchWindow(HWND hWnd);
  bool GetTouchInputInfo(HTOUCHINPUT hTouchInput, uint32_t cInputs, PTOUCHINPUT pInputs);
  bool CloseTouchInputHandle(HTOUCHINPUT hTouchInput);
  bool IsAvailable();

  // Simple gesture process
  bool ProcessGestureMessage(HWND hWnd, WPARAM wParam, LPARAM lParam, mozilla::WidgetSimpleGestureEvent& evt);

  // Pan processing
  bool IsPanEvent(LPARAM lParam);
  bool ProcessPanMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);
  bool PanDeltaToPixelScroll(mozilla::WidgetWheelEvent& aWheelEvent);
  void UpdatePanFeedbackX(HWND hWnd, int32_t scrollOverflow, bool& endFeedback);
  void UpdatePanFeedbackY(HWND hWnd, int32_t scrollOverflow, bool& endFeedback);
  void PanFeedbackFinalize(HWND hWnd, bool endFeedback);

public:
  // Helpers
  bool GetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
  bool CloseGestureInfoHandle(HGESTUREINFO hGestureInfo);
  bool GetGestureExtraArgs(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs);
  bool SetGestureConfig(HWND hWnd, UINT cIDs, PGESTURECONFIG pGestureConfig);
  bool GetGestureConfig(HWND hWnd, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig);
  bool BeginPanningFeedback(HWND hWnd);
  bool EndPanningFeedback(HWND hWnd);
  bool UpdatePanningFeedback(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia);

protected:

private:
  // Function prototypes
  typedef BOOL (WINAPI * GetGestureInfoPtr)(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
  typedef BOOL (WINAPI * CloseGestureInfoHandlePtr)(HGESTUREINFO hGestureInfo);
  typedef BOOL (WINAPI * GetGestureExtraArgsPtr)(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs);
  typedef BOOL (WINAPI * SetGestureConfigPtr)(HWND hwnd, DWORD dwReserved, UINT cIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
  typedef BOOL (WINAPI * GetGestureConfigPtr)(HWND hwnd, DWORD dwReserved, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
  typedef BOOL (WINAPI * BeginPanningFeedbackPtr)(HWND hWnd);
  typedef BOOL (WINAPI * EndPanningFeedbackPtr)(HWND hWnd, BOOL fAnimateBack);
  typedef BOOL (WINAPI * UpdatePanningFeedbackPtr)(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia);
  typedef BOOL (WINAPI * RegisterTouchWindowPtr)(HWND hWnd, ULONG flags);
  typedef BOOL (WINAPI * UnregisterTouchWindowPtr)(HWND hWnd);
  typedef BOOL (WINAPI * GetTouchInputInfoPtr)(HTOUCHINPUT hTouchInput, uint32_t cInputs, PTOUCHINPUT pInputs, int32_t cbSize);
  typedef BOOL (WINAPI * CloseTouchInputHandlePtr)(HTOUCHINPUT hTouchInput);

  // Static function pointers
  static GetGestureInfoPtr getGestureInfo;
  static CloseGestureInfoHandlePtr closeGestureInfoHandle;
  static GetGestureExtraArgsPtr getGestureExtraArgs;
  static SetGestureConfigPtr setGestureConfig;
  static GetGestureConfigPtr getGestureConfig;
  static BeginPanningFeedbackPtr beginPanningFeedback;
  static EndPanningFeedbackPtr endPanningFeedback;
  static UpdatePanningFeedbackPtr updatePanningFeedback;
  static RegisterTouchWindowPtr registerTouchWindow;
  static UnregisterTouchWindowPtr unregisterTouchWindow;
  static GetTouchInputInfoPtr getTouchInputInfo;
  static CloseTouchInputHandlePtr closeTouchInputHandle;

  // Delay load info 
  bool InitLibrary();

  static HMODULE sLibraryHandle;
  static const wchar_t kGestureLibraryName[];

  // Pan and feedback state
  nsPointWin mPanIntermediate;
  nsPointWin mPanRefPoint;
  nsPointWin mPixelScrollDelta;
  bool mPanActive;
  bool mFeedbackActive;
  bool mXAxisFeedback;
  bool mYAxisFeedback;
  bool mPanInertiaActive;
  nsPointWin mPixelScrollOverflow;

  // Zoom state
  double mZoomIntermediate;

  // Rotate state
  double mRotateIntermediate;
};

#endif /* WinGesture_h__ */
