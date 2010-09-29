#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

void* initXlib() {
	Display *display = XOpenDisplay((char *) NULL);
	return display;
}

void finishXlib(void* display) {
	XCloseDisplay(display);
}

int getOutputRotation(void * display, char * outputName, int * out_rotation, int * out_mirrorX, int * out_mirrorY) {
	XRRScreenResources *res = XRRGetScreenResourcesCurrent(display, DefaultRootWindow(display));
	int found = 0;

	int o;
	for(o = 0; o < res->noutput; o++) {
		XRROutputInfo *outpInf = XRRGetOutputInfo(display, res, res->outputs[o]);
		if(!strcmp(outpInf->name, outputName)) {
			if(outpInf->crtc != 0) {
				/* The output is active (has a CRTC) */

				XRRCrtcInfo* crtcInf = XRRGetCrtcInfo(display, res, outpInf->crtc);

				if(crtcInf->rotation & RR_Rotate_180) {
					* out_rotation = 180;
				} else if(crtcInf->rotation & RR_Rotate_270) {
					* out_rotation = 270;
				} else if(crtcInf->rotation & RR_Rotate_90) {
					* out_rotation = 90;
				} else {
					* out_rotation = 0;
				}

				if(crtcInf->rotation & RR_Reflect_X) {
					* out_mirrorX = 1;
				} else {
					* out_mirrorX = 0;
				}
				if(crtcInf->rotation & RR_Reflect_Y) {
					* out_mirrorY = 1;
				} else {
					* out_mirrorY = 0;
				}
				found = 1;
			}
		}
		XRRFreeOutputInfo(outpInf);
		if(found) break;
	}

	XRRFreeScreenResources(res);

	return found;

}
