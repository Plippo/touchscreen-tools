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

#ifndef PROFILES_H_
#define PROFILES_H_

#define MAX_DEVICES_PER_PROFILE 10

typedef struct _DeviceSettings {
	char * inputDeviceName;
	char * attachedOutput;
	int autoOutput;
	int* inputDeviceIDs;
	int inputDeviceCount;
	int autoCalibration;
	int outputMinX;
	int outputMaxX;
	int outputMinY;
	int outputMaxY;
	int inverseX;
	int inverseY;
	int swapAxes;
} DeviceSettings;

typedef struct _DeviceSettingsList {
	DeviceSettings * deviceSettings;
	int nDeviceSettings;
	int nDeviceSettingsSpace;
} DeviceSettingsList;

void addDeviceSettings(DeviceSettingsList*, char*, char*, int, int, int, int, int, int, int, int, int);
int loadSettings(DeviceSettingsList*, char *, char *);
void freeSettings(DeviceSettingsList*);
int addDeviceSettingsFromFile(char *, DeviceSettingsList *, char *);
char* getGlobalFileName();
char* getPrivateFileName();

#endif /* PROFILES_H_ */
