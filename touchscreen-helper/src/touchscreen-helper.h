/*
 Copyright (C) 2010, Philipp Merkel <linux@philmerk.de>

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.

 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef TOUCHSCREEN_HELPER_H_
#define TOUCHSCREEN_HELPER_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>

#define FALSE 0
#define TRUE 1

void swap(int*, int*);
void handleDeviceChange();
void handleDisplayChange(XRRScreenChangeNotifyEvent *);
void xLoop();
void setAutoCalibrationData(int d, XIDeviceInfo * deviceInfo);

#endif /* TOUCHSCREEN_HELPER_H_ */
