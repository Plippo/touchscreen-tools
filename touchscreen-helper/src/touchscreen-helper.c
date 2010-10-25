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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "touchscreen-helper.h"
#include "profiles.h"
#include <signal.h> 

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* Daemonize. Source: http://www-theorie.physik.unizh.ch/~dpotter/howto/daemonize (public domain) */
static void daemonize(void) {
	pid_t pid, sid;

	/* already a daemon */
	if (getppid() == 1)
		return;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory.  This prevents the current
	 directory from being locked; hence not being able to remove it. */
	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}

	/* Redirect standard files to /dev/null */
	void *r;
	r = freopen("/dev/null", "r", stdin);
	r = freopen("/dev/null", "w", stdout);
	r = freopen("/dev/null", "w", stderr);
}

Display *display;
Window root;
int screenNum;
int lastScreenWidth;
int lastScreenHeight;

Atom absXAtom;
Atom absYAtom;
Atom floatAtom;

int debugMode = 0;

pthread_t signalThread;
sigset_t signalSet;

DeviceSettingsList profiles;
pthread_mutex_t profilesMutex = PTHREAD_MUTEX_INITIALIZER;

void swap(int *a, int *b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}

void setCalibration(int id, int minX, int maxX, int minY, int maxY, int axesSwap, int screenWidth, int screenHeight, int outputX, int outputY, int outputWidth, int outputHeight, int rotation) {

	float matrix[] = { 1., 0., 0.,    /* [0] [1] [2] */
	                   0., 1., 0.,    /* [3] [4] [5] */
	                   0., 0., 1. };  /* [6] [7] [8] */

	int matrixMode = 0;
	/* Check if transformation matrix is supported */
	long l;
	if((sizeof l) == 4 || (sizeof l) == 8) {
		/* We only support matrix mode on systems where longs are 32 or 64 bits long */
		Atom retType;
		int retFormat;
		unsigned long retItems, retBytesAfter;
		unsigned char * data = NULL;
		if(XIGetProperty(display, id, XInternAtom(display,
				"Coordinate Transformation Matrix", 0), 0, 9 * 32, False, floatAtom,
				&retType, &retFormat, &retItems, &retBytesAfter,
				&data) != Success) {
			data = NULL;
		}
		if(data != NULL && retItems == 9) {
			matrixMode = 1;
		}
		if(data != NULL) {
			XFree(data);
		}
	}	

	unsigned char flipHoriz = 0, flipVerti = 0;

	if(matrixMode) {	

		if(debugMode) printf("Use matrix method\n");

		/* Output rotation */
		if(rotation & RR_Rotate_180) {
			matrix[0] = -1.;
			matrix[4] = -1;
			matrix[2] = 1.;
			matrix[5] = 1.;
		} else if(rotation & RR_Rotate_90) {
			matrix[0] = 0.;
			matrix[1] = -1.;
			matrix[3] = 1.;
			matrix[4] = 0.;

			matrix[2] = 1.;
		} else if(rotation & RR_Rotate_270) {
			matrix[0] = 0.;
			matrix[1] = 1.;
			matrix[3] = -1.;
			matrix[4] = 0.;

			matrix[5] = 1.;
		}

		/* Output Reflection */
		if(rotation & RR_Reflect_X) {
			matrix[0]*= -1.;
			matrix[1]*= -1.;
			matrix[2]*= -1.;
			matrix[2]+= 1.;
		}
		if(rotation & RR_Reflect_Y) {
			matrix[3]*= -1.;
			matrix[4]*= -1.;
			matrix[5]*= -1.;
			matrix[5]+= 1.;
		}

		/* Output Size */
		float widthRel = outputWidth / (float) screenWidth;
		float heightRel = outputHeight / (float) screenHeight;
		matrix[0] *= widthRel;
		matrix[1] *= widthRel;
		matrix[2] *= widthRel;
		matrix[3] *= heightRel;
		matrix[4] *= heightRel;
		matrix[5] *= heightRel;

		/* Output Position */
		matrix[2] += outputX / (float) screenWidth;
		matrix[5] += outputY / (float) screenHeight;

	} else {

		if(debugMode) printf("Use legacy method\n");

		/* No support for transformations, so use legacy method */

		if(rotation & RR_Rotate_180) {
			flipHoriz = !flipHoriz;
			flipVerti = !flipVerti;
	
		} else if(rotation & RR_Rotate_90) {
			flipVerti = !flipVerti;
			axesSwap = !axesSwap;
		} else if(rotation & RR_Rotate_270) {
			flipHoriz = !flipHoriz;
			axesSwap = !axesSwap;
		}

		/* Output Reflection */
		if(rotation & RR_Reflect_X) {
			flipHoriz = !flipHoriz;
		}
		if(rotation & RR_Reflect_Y) {
			flipVerti = !flipVerti;
		}


		if(axesSwap) {
			swap(&maxX, &maxY);
			swap(&minX, &minY);
		}

		int leftSpace, rightSpace, topSpace, bottomSpace;
		leftSpace = outputX;
		rightSpace = screenWidth - outputX - outputWidth;

		topSpace = outputY;
		bottomSpace = screenHeight - outputY - outputHeight;

		if(flipHoriz) swap(&leftSpace, &rightSpace);
		if(flipVerti) swap(&topSpace, &bottomSpace);
		
		float fctX = ((float) (maxX - minX)) / ((float) outputWidth);
		float fctY = ((float) (maxY - minY)) / ((float) outputHeight);
		minX = minX - (int) (leftSpace * fctX);
		maxX = maxX + (int) (rightSpace * fctX);
		minY = minY - (int) (topSpace * fctY);
		maxY = maxY + (int) (bottomSpace * fctY);

	}

	XDevice *dev = XOpenDevice(display, id);
	if(dev) {
		if(matrixMode) {
			if((sizeof l) == 4) {
				XChangeDeviceProperty(display, dev, XInternAtom(display,
					"Coordinate Transformation Matrix", 0), floatAtom, 32, PropModeReplace, (unsigned char*) matrix, 9);
			} else if((sizeof l) == 8) {
				/* Xlib needs the floats long-aligned, so let's align them. */
				float matrix2[] = { matrix[0], 0., matrix[1], 0., matrix[2], 0.,
				                    matrix[3], 0., matrix[4], 0., matrix[5], 0.,
				                    matrix[6], 0., matrix[7], 0., matrix[8], 0.};
				XChangeDeviceProperty(display, dev, XInternAtom(display,
					"Coordinate Transformation Matrix", 0), floatAtom, 32, PropModeReplace, (unsigned char*) matrix2, 9);
			}
		}

		long calib[] = { minX, maxX, minY, maxY };
		//TODO instead of long, use platform 32 bit type
		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Evdev Axis Calibration", 0), XA_INTEGER, 32, PropModeReplace, (unsigned char*) calib, 4);

		unsigned char flipData[] = {flipHoriz, flipVerti};
		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Evdev Axis Inversion", 0), XA_INTEGER, 8, PropModeReplace, flipData, 2);

		unsigned char cAxesSwap = (unsigned char) axesSwap;
		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Evdev Axes Swap", 0), XA_INTEGER, 8, PropModeReplace, &cAxesSwap, 1);

		XCloseDevice(display, dev);
	}
}


void handleDisplayChange(XRRScreenChangeNotifyEvent *evt) {
	int screenWidth, screenHeight;
	if(evt==NULL) {
		screenWidth = lastScreenWidth;
		screenHeight = lastScreenHeight;
	} else {
		screenWidth = evt->width;
		lastScreenWidth = screenWidth;
		screenHeight = evt->height;
		lastScreenHeight = screenHeight;
	}

	if(debugMode) {
		printf("Screen size: %ix%i\n", screenWidth, screenHeight);
	}

	XRRScreenResources *res = XRRGetScreenResourcesCurrent(display, root);

	int d;
	for(d = 0; d < profiles.nDeviceSettings; d++) {
		if(profiles.deviceSettings[d].attachedOutput == NULL && !(profiles.deviceSettings[d].autoOutput)) {
	
			/* Set calibration of whole screen */

			int id = 0;
			for(id = 0; id<profiles.deviceSettings[d].inputDeviceCount; id++) {
				if(debugMode) {
					printf("Calibrate Device with ID %i\n", profiles.deviceSettings[d].inputDeviceIDs[id]);
				}
							setCalibration(profiles.deviceSettings[d].inputDeviceIDs[id], profiles.deviceSettings[d].outputMinX, profiles.deviceSettings[d].outputMaxX, profiles.deviceSettings[d].outputMinY, profiles.deviceSettings[d].outputMaxY, profiles.deviceSettings[d].swapAxes, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, 0); 

			}

		} else if(profiles.deviceSettings[d].inputDeviceCount > 0) {
			int o;
			for(o = 0; o < res->noutput; o++) {
				XRROutputInfo *outpInf = XRRGetOutputInfo(display, res, res->outputs[o]);
				if((profiles.deviceSettings[d].autoOutput && (strstr(outpInf->name, "LVDS") || strstr(outpInf->name, "lvds")) )
					|| (profiles.deviceSettings[d].attachedOutput && !strcmp(outpInf->name, profiles.deviceSettings[d].attachedOutput))) {
					/* This is the attached output */
					if(outpInf->crtc != 0) {
						/* The output is active (has a CRTC) */

						XRRCrtcInfo* crtcInf = XRRGetCrtcInfo(display, res, outpInf->crtc);
						if(debugMode) {
							printf("Output %s -- x: %i; y: %i; w: %i; h: %i\n", outpInf->name, crtcInf->x, crtcInf->y, crtcInf->width, crtcInf->height);
						}

						/* Set calibration */
						int id = 0;
						for(id = 0; id<profiles.deviceSettings[d].inputDeviceCount; id++) {
							if(debugMode) {
								printf("Calibrate Device with ID %i\n", profiles.deviceSettings[d].inputDeviceIDs[id]);
							}

							setCalibration(profiles.deviceSettings[d].inputDeviceIDs[id], profiles.deviceSettings[d].outputMinX, profiles.deviceSettings[d].outputMaxX, profiles.deviceSettings[d].outputMinY, profiles.deviceSettings[d].outputMaxY, profiles.deviceSettings[d].swapAxes, screenWidth, screenHeight, crtcInf->x, crtcInf->y, crtcInf->width, crtcInf->height, crtcInf->rotation); 

						}

						XRRFreeCrtcInfo(crtcInf);
					}
					/* Output found, so break */
					break;
				}
				XRRFreeOutputInfo(outpInf);
			}
		}
	}
	XRRFreeScreenResources(res);
}

void setAutoCalibrationData(int d, XIDeviceInfo * deviceInfo) {
	profiles.deviceSettings[d].swapAxes = 0;

	int c;
	for(c = 0; c < deviceInfo->num_classes; c++) {
		if(deviceInfo->classes[c]->type == XIValuatorClass) {
			XIValuatorClassInfo* valuatorInfo = (XIValuatorClassInfo *) deviceInfo->classes[c];
			if(valuatorInfo->mode == XIModeAbsolute) {
				if(valuatorInfo->label == absXAtom) {
					profiles.deviceSettings[d].outputMinX = valuatorInfo->min;
					profiles.deviceSettings[d].outputMaxX = valuatorInfo->max;
				} else if(valuatorInfo->label == absYAtom) {
					profiles.deviceSettings[d].outputMinY = valuatorInfo->min;
					profiles.deviceSettings[d].outputMaxY = valuatorInfo->max;
				}
			}
		}
	}	


	profiles.deviceSettings[d].autoCalibration = FALSE;
}

int isAbsoluteInputDevice(XIDeviceInfo * deviceInfo) {
	int xFound = FALSE, yFound = FALSE;
	int c;
	for(c = 0; c < deviceInfo->num_classes; c++) {
		if(deviceInfo->classes[c]->type == XIValuatorClass) {
			XIValuatorClassInfo* valuatorInfo = (XIValuatorClassInfo *) deviceInfo->classes[c];
			if(valuatorInfo->mode == XIModeAbsolute) {
				if(valuatorInfo->label == absXAtom) {
					xFound = TRUE;
					if(yFound) return TRUE;
				} else if(valuatorInfo->label == absYAtom) {
					yFound = TRUE;
					if(xFound) return TRUE;
				}
			}
		}
	}	

	return FALSE;
}

void handleDeviceChange() {
	

	int n;
	XIDeviceInfo *info = XIQueryDevice(display, XIAllDevices, &n);
	if (!info) {
		printf("No XInput devices available\n");
		exit(1);
	}

	int d;
	for(d = 0; d < profiles.nDeviceSettings; d++) {
		profiles.deviceSettings[d].inputDeviceCount = 0;
	}

	/* Go through input devices and add to matching profile  */
	int i;
	for (i = 0; i < n; i++) {
		if (info[i].use == XIMasterPointer || info[i].use == XIMasterKeyboard) {
		} else {
			int foundProfile = FALSE;
			for(d = 0; d < profiles.nDeviceSettings; d++) {
				if (profiles.deviceSettings[d].inputDeviceName != NULL && !strcmp(info[i].name, profiles.deviceSettings[d].inputDeviceName)) {
					if(debugMode) {
						printf("Device %s for profile %s found with ID %i\n", info[i].name, profiles.deviceSettings[d].inputDeviceName, info[i].deviceid);
					}
					if(profiles.deviceSettings[d].inputDeviceCount < MAX_DEVICES_PER_PROFILE) {
						profiles.deviceSettings[d].inputDeviceCount++;
						profiles.deviceSettings[d].inputDeviceIDs[profiles.deviceSettings[d].inputDeviceCount-1] = info[i].deviceid;

						if(profiles.deviceSettings[d].autoCalibration) {
							/* Set default calibration from axes */
							setAutoCalibrationData(d, &(info[i]));
						}
					}
					foundProfile = TRUE;
					break;
				}
			}
			if(foundProfile == FALSE) {
				/* No profile available. If touchscreen, create dummy profile */
				if(isAbsoluteInputDevice(&(info[i]))) {
					if(debugMode) {
						printf("Found absolute X and Y axis on device %i, assume it's a touchscreen.\n", info[i].deviceid);
						printf("No profile found for it, create dummy profile.\n");
					}
					/* Create dummy profile */
					char *deviceName = malloc((strlen(info[i].name) + 1) * sizeof (char));
					strcpy(deviceName, info[i].name);
					addDeviceSettings(&profiles, deviceName, NULL, TRUE, TRUE, 0, 0, 0, 0, 0);
					profiles.deviceSettings[profiles.nDeviceSettings-1].inputDeviceCount = 1;
					profiles.deviceSettings[profiles.nDeviceSettings-1].inputDeviceIDs[0] = info[i].deviceid;

					/* Set default calibration from axes */
					setAutoCalibrationData(profiles.nDeviceSettings - 1, &(info[i]));
				}

			}
		}

	}

	XIFreeDeviceInfo(info);


	handleDisplayChange((XRRScreenChangeNotifyEvent*) NULL);
}

void xLoop() {
	XEvent ev;
	while (1) {
		XNextEvent(display, &ev);
	
		if(ev.type == 101) {
			/* Why isn't this magic constant explained anywhere?? */
			pthread_mutex_lock( &profilesMutex );
			handleDisplayChange((XRRScreenChangeNotifyEvent *) &ev);	
			pthread_mutex_unlock( &profilesMutex );
		} else if(XGetEventData(display, &ev.xcookie)) {
			if(ev.xcookie.evtype == XI_HierarchyChanged) {
				pthread_mutex_lock( &profilesMutex );
				if(debugMode) {
					printf("XInput device change, reload devices.\n");
				}
				handleDeviceChange();
				XFreeEventData(display, &ev.xcookie);
				pthread_mutex_unlock( &profilesMutex );
			}
		}
	}
}


void * signalThreadFunction(void *arg) {
	int sig;
	while(1) {
		sigwait ( &signalSet, &sig );
		pthread_mutex_lock( &profilesMutex );
		if(debugMode) printf("Reload config due to signal\n");
		freeSettings(&profiles);
		loadSettings(&profiles, NULL, NULL);
		handleDeviceChange();
		XFlush(display);
		pthread_mutex_unlock( &profilesMutex );
	}
}


int main(int argc, char **argv) {

	int doDaemonize = 1;

	int i;
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--debug") == 0) {
			doDaemonize = 0;
			debugMode = 1;
		}

	}

	if (doDaemonize) {
		daemonize();
	}

	/* Connect to X server */
	if ((display = XOpenDisplay((char *) NULL)) == NULL) {
		fprintf(stderr, "Couldn't connect to X server\n");
		exit(1);
	}

	absXAtom = XInternAtom(display, "Abs X", FALSE);
	absYAtom = XInternAtom(display, "Abs Y", FALSE);
	floatAtom = XInternAtom(display, "FLOAT", FALSE);

	/* Read X data */
	screenNum = DefaultScreen(display);

	root = RootWindow(display, screenNum);

	lastScreenWidth = DisplayWidth(display, screenNum);
	lastScreenHeight = DisplayHeight(display, screenNum);

	/* We have two threads accessing X */
	XInitThreads();


	int opcode, event, error;
	if (!XQueryExtension(display, "RANDR", &opcode, &event,
			&error)) {
		printf("X RANDR extension not available.\n");
		XCloseDisplay(display);
		exit(1);
	}

	/* Which version of XRandR? We support 1.3 */
	int major = 1, minor = 3;
	if (!XRRQueryVersion(display, &major, &minor)) {
		printf("XRandR version not available.\n");
		XCloseDisplay(display);
		exit(1);
	} else if(!(major>1 || (major == 1 && minor >= 3))) {
		printf("XRandR 1.3 not available. Server supports %d.%d\n", major, minor);
		XCloseDisplay(display);
		exit(1);
	}

	/* XInput Extension available? */
	if (!XQueryExtension(display, "XInputExtension", &opcode, &event,
			&error)) {
		printf("X Input extension not available.\n");
		XCloseDisplay(display);
		exit(1);
	}

	/* Which version of XI2? We support 2.0 */
	major = 2; minor = 0;
	if (XIQueryVersion(display, &major, &minor) == BadRequest) {
		printf("XI2 not available. Server supports %d.%d\n", major, minor);
		XCloseDisplay(display);
		exit(1);
	}
	


	XRRSelectInput(display, root, RRScreenChangeNotifyMask | RROutputChangeNotifyMask | RROutputPropertyNotifyMask | RRCrtcChangeNotifyMask);

	/* Register for XInput device change events */
	XIEventMask eventmask;
	unsigned char mask[2] = { 0, 0 }; /* the actual mask */

	eventmask.deviceid = XIAllDevices;
	eventmask.mask_len = sizeof(mask); /* always in bytes */
	eventmask.mask = mask;
	/* now set the mask */
	XISetMask(mask, XI_HierarchyChanged);

	/* select on the window */
	XISelectEvents(display, root, &eventmask, 1);

	loadSettings(&profiles, NULL, NULL);

	handleDeviceChange();

	sigemptyset(&signalSet);
	sigaddset(&signalSet, SIGUSR1);
	pthread_sigmask (SIG_BLOCK, &signalSet, NULL);
	if(pthread_create(&signalThread, NULL, signalThreadFunction, NULL)) {
		printf("Couldn't create signal thread.\n");
	}
	

	xLoop();

	freeSettings(&profiles);

	XCloseDisplay(display);
	return 0;	

}

