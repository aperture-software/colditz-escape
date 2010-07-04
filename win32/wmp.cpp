/*
 *  Colditz Escape! - Rewritten Engine for "Escape From Colditz"
 *  copyright (C) 2008-2009 Aperture Software
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  ---------------------------------------------------------------------------
 *  wmp.cpp: Simple Windows directShow Movie Player
 *  ---------------------------------------------------------------------------
 */

#include <dshow.h>
#include <strsafe.h>
#include "wmp.h"

#define SAFE_RELEASE(p) {if (p!=NULL) p->Release(); p=NULL;}

// Global handlers
IGraphBuilder  *pGB = NULL;
IMediaControl  *pMC = NULL;
IVideoWindow   *pVW = NULL;
IMediaEvent    *pME = NULL;

// Target window variables
HWND			hWnd=0;
HINSTANCE		hInstance;
RECT			rect;

static bool		is_video_playing = false;

void wmp_release();


bool wmp_init(char* app_name)
{
    HRESULT hr;

    is_video_playing = false;

    // The only way we have to find our GLUT window handle is name
    hWnd = FindWindowA(NULL, app_name);
    hInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);
    GetClientRect(hWnd, &rect);

    // Initialize COM (and don't care too much about errors, as it might
    // already have been initialized elsewhere)
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Instantiate a filter graph interface
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
        IID_IGraphBuilder, (void **)&pGB);
    if (hr != S_OK)
    {
        fprintf(stderr, "wmp_init: CoCreateInstance error %X\n", hr);
        wmp_release();
        return false;
    }

    // Initalize the various interfaces
    hr = pGB->QueryInterface(IID_IMediaControl, (void **)&pMC);
    if (hr != S_OK)
    {
        fprintf(stderr, "wmp_init: pGB->QueryInterface IID_IMediaControl error %X\n", hr);
        wmp_release();
        return false;
    }

    hr = pGB->QueryInterface(IID_IVideoWindow, (void **)&pVW);
    if (hr != S_OK)
    {
        printf("wmp_init: pGB->QueryInterface IID_IVideoWindow error %X\n", hr);
        wmp_release();
        return false;
    }

    hr = pGB->QueryInterface(IID_IMediaEventEx,  (void **)&pME);
    if (hr != S_OK)
    {
        fprintf(stderr, "wmp_init: pGB->QueryInterface IID_IMediaEventEx error %X\n", hr);
        wmp_release();
        return false;
    }

    return true;
}


void wmp_release()
{
    SAFE_RELEASE(pGB);
    SAFE_RELEASE(pMC);
    SAFE_RELEASE(pVW);
    SAFE_RELEASE(pME);
    CoUninitialize();
}


void wmp_stop()
{
    if (pMC != NULL)
        pMC->Stop();
    is_video_playing = false;
    wmp_release();
}


bool wmp_play(char* s)
{
    HRESULT hr;

    // Damn, considering how long they've been using UNICODE in their apps, you'd think that
    // Microsoft would have create a QUICK helper function that simply returns an LPCWSTR out
    // of an ANSI char* using CP437. But NO, you have to go through all this crap!
    BSTR unicode_s;
    int slen, unicode_slen;
    slen = lstrlenA(s);
    unicode_slen = MultiByteToWideChar(CP_ACP, 0, s, slen, 0, 0);
    unicode_s = SysAllocStringLen(0, unicode_slen);
    MultiByteToWideChar(CP_ACP, 0, s, slen, unicode_s, unicode_slen);

    // What the hell is a FilterGraph anyway?
    hr = pGB->RenderFile(unicode_s, NULL);
    SysFreeString(unicode_s);
    if (FAILED(hr))
    {
        fprintf(stderr, "Failed(0x%08lx) in RenderFile(%s)!\n", hr, s);
        return false;
    }

    // Use our GL window for video display
    hr = pVW->put_Owner((OAHWND)hWnd);
    if (FAILED(hr))
    {
        fprintf(stderr, "Failed(0x%08lx) in put_Owner!\n", hr, s);
        return false;
    }
    pVW->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    pVW->SetWindowPosition(rect.left, rect.top, rect.right, rect.bottom);

    // Start playback
    hr = pMC->Run();
    if (FAILED(hr)) {
        fprintf(stderr, "Failed(%08lx) in Run()!\n", hr);
        return false;
    }

    // And we're open for business!
    is_video_playing = true;

    return true;
}


bool wmp_isplaying()
{
    long lEventCode;
    LONG_PTR lpParam1, lpParam2;

    // Check for end of playback
    if (is_video_playing && (pME != NULL) &&
        (pME->GetEvent(&lEventCode, &lpParam1, &lpParam2, 0) != E_ABORT))
    {
        if (lEventCode == EC_COMPLETE)
            is_video_playing = false;
        pME->FreeEventParams(lEventCode, lpParam1, lpParam2);
    }

    return is_video_playing;
}

