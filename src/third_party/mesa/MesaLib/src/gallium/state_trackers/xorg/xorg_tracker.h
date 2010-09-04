/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 *
 */

#ifndef _XORG_TRACKER_H_
#define _XORG_TRACKER_H_

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <xorg-server.h>
#include <xf86.h>
#include "xf86Crtc.h"
#include <exa.h>

#ifdef DRM_MODE_FEATURE_DIRTYFB
#include <damage.h>
#endif

#include "pipe/p_screen.h"
#include "state_tracker/drm_api.h"

#define DRV_ERROR(msg)	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, msg);

struct kms_bo;
struct kms_driver;
struct exa_context;

typedef struct
{
    int lastInstance;
    int refCount;
    ScrnInfoPtr pScrn_1;
    ScrnInfoPtr pScrn_2;
} EntRec, *EntPtr;

#define XORG_NR_FENCES 3

typedef struct _modesettingRec
{
    /* drm */
    int fd;
    unsigned fb_id;

    /* X */
    EntPtr entityPrivate;

    int Chipset;
    EntityInfoPtr pEnt;
    struct pci_device *PciInfo;

    Bool noAccel;
    Bool SWCursor;
    CloseScreenProcPtr CloseScreen;

    /* Broken-out options. */
    OptionInfoPtr Options;

    unsigned int SaveGeneration;

    void (*blockHandler)(int, pointer, pointer, pointer);
    struct pipe_fence_handle *fence[XORG_NR_FENCES];

    CreateScreenResourcesProcPtr createScreenResources;

    /* for frontbuffer backing store */
    Bool (*destroy_front_buffer)(ScrnInfoPtr pScrn);
    Bool (*create_front_buffer)(ScrnInfoPtr pScrn);
    Bool (*bind_front_buffer)(ScrnInfoPtr pScrn);

    /* kms */
    struct kms_driver *kms;
    struct kms_bo *root_bo;

    /* gallium */
    struct drm_api *api;
    struct pipe_screen *screen;
    struct pipe_context *ctx;
    boolean d_depth_bits_last;
    boolean ds_depth_bits_last;
    struct pipe_texture *root_texture;

    /* exa */
    struct exa_context *exa;
    Bool noEvict;
    Bool debug_fallback;

    /* winsys hocks */
    Bool (*winsys_screen_init)(ScrnInfoPtr pScr);
    Bool (*winsys_screen_close)(ScrnInfoPtr pScr);
    Bool (*winsys_enter_vt)(ScrnInfoPtr pScr);
    Bool (*winsys_leave_vt)(ScrnInfoPtr pScr);
    void *winsys_priv;

#ifdef DRM_MODE_FEATURE_DIRTYFB
    DamagePtr damage;
#endif
} modesettingRec, *modesettingPtr;

#define modesettingPTR(p) ((modesettingPtr)((p)->driverPrivate))


/***********************************************************************
 * xorg_exa.c
 */
struct pipe_texture *
xorg_exa_get_texture(PixmapPtr pPixmap);

unsigned
xorg_exa_get_pixmap_handle(PixmapPtr pPixmap, unsigned *stride);

int
xorg_exa_set_displayed_usage(PixmapPtr pPixmap);

int
xorg_exa_set_shared_usage(PixmapPtr pPixmap);

Bool
xorg_exa_set_texture(PixmapPtr pPixmap, struct  pipe_texture *tex);

struct pipe_texture *
xorg_exa_create_root_texture(ScrnInfoPtr pScrn,
			     int width, int height,
			     int depth, int bpp);

void *
xorg_exa_init(ScrnInfoPtr pScrn, Bool accel);

void
xorg_exa_close(ScrnInfoPtr pScrn);


/***********************************************************************
 * xorg_dri2.c
 */
Bool
xorg_dri2_init(ScreenPtr pScreen);

void
xorg_dri2_close(ScreenPtr pScreen);


/***********************************************************************
 * xorg_crtc.c
 */
void
xorg_crtc_init(ScrnInfoPtr pScrn);

void
xorg_crtc_cursor_destroy(xf86CrtcPtr crtc);


/***********************************************************************
 * xorg_output.c
 */
void
xorg_output_init(ScrnInfoPtr pScrn);


/***********************************************************************
 * xorg_xv.c
 */
void
xorg_xv_init(ScreenPtr pScreen);


#endif /* _XORG_TRACKER_H_ */
