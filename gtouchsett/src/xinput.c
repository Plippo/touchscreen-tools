#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>

Atom absXAtom;
Atom absYAtom;

typedef struct _InputDeviceInformation {
	char* deviceName;
	int deviceID; /* For the last entry in the array, deviceID is -1. */
} InputDeviceInformation;

int isAbsoluteInputDevice(XIDeviceInfo * deviceInfo) {
	//return 1;
	int xFound = 0, yFound = 0;
	int c;
	for(c = 0; c < deviceInfo->num_classes; c++) {
		if(deviceInfo->classes[c]->type == XIValuatorClass) {
			XIValuatorClassInfo* valuatorInfo = (XIValuatorClassInfo *) deviceInfo->classes[c];
			if(valuatorInfo->mode == XIModeAbsolute) {
				if(valuatorInfo->label == absXAtom) {
					xFound = 1;
					if(yFound) return 1;
				} else if(valuatorInfo->label == absYAtom) {
					yFound = 1;
					if(xFound) return 1;
				}
			}
		}
	}	

	return 0;
}

InputDeviceInformation * getTouchscreens(Display* display) {
	absXAtom = XInternAtom(display, "Abs X", False);
	absYAtom = XInternAtom(display, "Abs Y", False);

	int n;
	int touchscreenCount = 0;
	XIDeviceInfo *info = XIQueryDevice(display, XIAllDevices, &n);
	if (!info) {
		return NULL;
	}
	int i;
	for (i = 0; i < n; i++) {
		if (info[i].use == XIMasterPointer || info[i].use == XIMasterKeyboard) {
		} else {
			if(isAbsoluteInputDevice(&(info[i]))) {
				touchscreenCount++;
			}
		}
	}

	if(touchscreenCount == 0) {
		XIFreeDeviceInfo(info);
		return NULL;
	}

	InputDeviceInformation * result;
	result = malloc((touchscreenCount + 1) * sizeof(InputDeviceInformation));

	int j = -1;
	for (i = 0; i < n; i++) {
		if (info[i].use == XIMasterPointer || info[i].use == XIMasterKeyboard) {
		} else {
			if(isAbsoluteInputDevice(&(info[i]))) {
				j++;
				result[j].deviceID = info[i].deviceid;
				if(info[i].name == NULL) {
					result[j].deviceName = NULL;
				} else {
					result[j].deviceName = malloc((strlen(info[i].name) + 1) * sizeof(char));
					strcpy(result[j].deviceName, info[i].name);
				}
			}
		}
	}
	j++;
	result[j].deviceID = -1;	

	XIFreeDeviceInfo(info);
	return result;

}

void freeInputDevices(InputDeviceInformation * information) {
	if(information == NULL) return;
	int i = -1;
	while(1) {
		i++;
		if(information[i].deviceID == -1) break;
		if(information[i].deviceName != NULL) {
			free(information[i].deviceName);
		}
	}
	free(information);
}

int getLastRawCoordinates(Display* display, int deviceID, int * out_x, int * out_y) {
	int n;
	XIDeviceInfo *info = XIQueryDevice(display, deviceID, &n);
	if (!info) {
		return 0;
	}
	if(n != 1) {
		XIFreeDeviceInfo(info);
		return 0;
	}

	int c, xFound, yFound;
	for(c = 0; c < info[0].num_classes; c++) {
		if(info[0].classes[c]->type == XIValuatorClass) {
			XIValuatorClassInfo* valuatorInfo = (XIValuatorClassInfo *) info[0].classes[c];
			if(valuatorInfo->mode == XIModeAbsolute) {
				if(valuatorInfo->label == absXAtom) {
					xFound = 1;
					* out_x = valuatorInfo->value;
				} else if(valuatorInfo->label == absYAtom) {
					yFound = 1;
					* out_y = valuatorInfo->value;
				}
			}
		}
	}	
	
	XIFreeDeviceInfo(info);
	if(xFound && yFound) {
		return 1;
	} else {
		return 0;
	}

}

void resetCalibration(Display* display, int deviceID) {
	XDevice *dev = XOpenDevice(display, deviceID);
	if(dev) {
		long data[] = { };
		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Evdev Axis Calibration", 0), XA_INTEGER, 32, PropModeReplace, (unsigned char*) data, 0);

		unsigned char data2[] = {0, 0};
		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Evdev Axis Inversion", 0), XA_INTEGER, 8, PropModeReplace, data2, 2);

		unsigned char axesSwap = 0;

		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Evdev Axes Swap", 0), XA_INTEGER, 8, PropModeReplace, &axesSwap, 1);

		float data4[] = { 1., 0., 0., 
		                  0., 1., 0.,
		                  0., 0., 1. };

		XChangeDeviceProperty(display, dev, XInternAtom(display,
			"Coordinate Transformation Matrix", 0), XInternAtom(display,
			"FLOAT", 0), 32, PropModeReplace, (unsigned char*) data4, 9);

		XCloseDevice(display, dev);
		XFlush(display);
	}
}
