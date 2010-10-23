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
#include "profiles.h"

#define HOME_SETTINGS_FILE "/.touchscreen-helper"
#define ETC_SETTINGS_FILE "/etc/touchscreen-helper"

#define BIT_0 1
#define BIT_1 2
#define BIT_2 4
#define BIT_3 8

char* globalFileName = ETC_SETTINGS_FILE;
char* privateFileName = NULL;

char* getGlobalFileName() {
	return globalFileName;
}
char* getPrivateFileName() {
	if(privateFileName == NULL) {
		char * home = getenv("HOME");
		privateFileName = malloc((strlen(home) + strlen(HOME_SETTINGS_FILE) + 1)*sizeof(char));
		strcpy(privateFileName, home);
		strcat(privateFileName, HOME_SETTINGS_FILE);

	}
	return privateFileName;
}


int loadSettings(DeviceSettingsList * list, char * onlyForDevice, char * onlyFile) {

	/* Allocate initial space for settings */
	list->nDeviceSettings = 0;
	list->nDeviceSettingsSpace = 10;
	list->deviceSettings = malloc (sizeof(DeviceSettings) * list->nDeviceSettingsSpace);
	if (list->deviceSettings == NULL) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}

	if(!onlyFile) {
		if(!addDeviceSettingsFromFile(getPrivateFileName(), list, onlyForDevice)) {
			printf("INFO: Configuration file %s could not be loaded.\n", getPrivateFileName());
		}
		if(!addDeviceSettingsFromFile(getGlobalFileName(), list, onlyForDevice)) {
			printf("INFO: Configuration file %s could not be loaded.\n", getGlobalFileName());
		}
	} else {
		if(!addDeviceSettingsFromFile(onlyFile, list, onlyForDevice)) {
			return 0;
		}
	}

	//addDeviceSettings(list, "eGalax Inc. USB TouchController", null, TRUE , FALSE, 414, 32635, 17, 32697);
	return 1;
}

/* attachedOutput and inputDeviceName will be used, don't free them afterwards!! */
void addDeviceSettings(DeviceSettingsList * list, char* inputDeviceName, char* attachedOutput, int autoOutput, int autoCalibration, int outputMinX, int outputMaxX, int outputMinY, int outputMaxY, int swapAxes) {
	if(list->nDeviceSettings + 1 > list->nDeviceSettingsSpace) {
		list->nDeviceSettingsSpace += 10;
		list->deviceSettings = realloc(list->deviceSettings, sizeof(DeviceSettings) * list->nDeviceSettingsSpace);
		if (list->deviceSettings == NULL) {
			fprintf(stderr, "Out of memory.\n");
			exit(1);
		}
	}
	
	list->nDeviceSettings++;
	
	int i = list->nDeviceSettings - 1;
	list->deviceSettings[i].inputDeviceName = inputDeviceName;
	list->deviceSettings[i].attachedOutput = attachedOutput;
	list->deviceSettings[i].autoOutput = autoOutput;
	list->deviceSettings[i].autoCalibration = autoCalibration;
	list->deviceSettings[i].inputDeviceCount = 0;
	list->deviceSettings[i].outputMinX = outputMinX;
	list->deviceSettings[i].outputMaxX = outputMaxX;
	list->deviceSettings[i].outputMinY = outputMinY;
	list->deviceSettings[i].outputMaxY = outputMaxY;
	list->deviceSettings[i].swapAxes = swapAxes;
	list->deviceSettings[i].inputDeviceIDs = malloc(MAX_DEVICES_PER_PROFILE * sizeof(int));
	if (list->deviceSettings[i].inputDeviceIDs == NULL) {
		fprintf(stderr, "Out of memory.\n");
	}

}

/* settings->attachedOutput and settings->inputDeviceName will be used, don't free them afterwards!! */
void addDeviceSettingsEntry(DeviceSettingsList * list, DeviceSettings* settings) {
	addDeviceSettings(list, settings->inputDeviceName, settings->attachedOutput, settings->autoOutput, settings->autoCalibration, settings->outputMinX, settings->outputMaxX, settings->outputMinY, settings->outputMaxY, settings->swapAxes);
}

void clearDeviceSettingsEntry(DeviceSettings* entry) {
	entry->inputDeviceName = NULL;
	entry->attachedOutput = NULL;
	entry->autoOutput = 0;
	entry->autoCalibration = 0;
	entry->inputDeviceCount = 0;
	entry->inputDeviceIDs = NULL;
	entry->outputMinX = 0;
	entry->outputMaxX = 0;
	entry->outputMinY = 0;
	entry->outputMaxY = 0;
	entry->swapAxes = 0;
}

void freeSettings(DeviceSettingsList * list) {
	if(list->nDeviceSettings > 0) {
		int i;
		for(i = 0; i < list->nDeviceSettings; i++) {
			if(list->deviceSettings[i].attachedOutput != NULL) {
				free(list->deviceSettings[i].attachedOutput);
			}
			if(list->deviceSettings[i].inputDeviceName != NULL) {
				free(list->deviceSettings[i].inputDeviceName);
			}
			free(list->deviceSettings[i].inputDeviceIDs);
		}
		list->nDeviceSettings = 0;
	}

	if(list->nDeviceSettingsSpace > 0) {
		/* Free old settings */
		free(list->deviceSettings);
		list->deviceSettings = NULL;
		list->nDeviceSettingsSpace = 0;
	}

}

void deleteProfile(DeviceSettingsList * list, char * deviceName) {
	int i;
	for(i = 0; i < list->nDeviceSettings; i++) {
		if(list->deviceSettings[i].inputDeviceName != NULL && !strcmp(deviceName, list->deviceSettings[i].inputDeviceName)) {
			free(list->deviceSettings[i].inputDeviceName);
			list->deviceSettings[i].inputDeviceName = NULL;
			free(list->deviceSettings[i].attachedOutput);
			list->deviceSettings[i].attachedOutput = NULL;
		}
	}
}

void changeProfile(DeviceSettingsList * list, DeviceSettings * newSettings) {
	int found = 0;
	int i;
	char * outp = NULL;
	if(newSettings->attachedOutput != NULL) {
		outp = malloc((strlen(newSettings->attachedOutput) + 1) * sizeof(char));
		strcpy(outp, newSettings->attachedOutput);
		/* We don't have to free it as it will be added to list and thus be freed when list is freed */
	}

	for(i = 0; i < list->nDeviceSettings; i++) {
		if(list->deviceSettings[i].inputDeviceName != NULL && !strcmp(newSettings->inputDeviceName, list->deviceSettings[i].inputDeviceName)) {
			if(list->deviceSettings[i].attachedOutput != NULL) {
				free(list->deviceSettings[i].attachedOutput);
			}
			list->deviceSettings[i].attachedOutput = outp;
			list->deviceSettings[i].autoOutput = newSettings->autoOutput;
			list->deviceSettings[i].autoCalibration = newSettings->autoCalibration;
			list->deviceSettings[i].outputMinX = newSettings->outputMinX;
			list->deviceSettings[i].outputMaxX = newSettings->outputMaxX;
			list->deviceSettings[i].outputMinY = newSettings->outputMinY;
			list->deviceSettings[i].outputMaxY = newSettings->outputMaxY;
			list->deviceSettings[i].swapAxes = newSettings->swapAxes;

			found = 1;
			break;
		}
	}


	if(!found) {
		char* inp = NULL;
		if(newSettings->inputDeviceName != NULL) {
			inp = malloc((strlen(newSettings->inputDeviceName) + 1) * sizeof(char));
			strcpy(inp, newSettings->inputDeviceName);
			/* We don't have to free it as it will be added to list and thus be freed when list is freed */
		}
		addDeviceSettings(list, inp, outp, newSettings->autoOutput, newSettings->autoCalibration, newSettings->outputMinX, newSettings->outputMaxX, newSettings->outputMinY, newSettings->outputMaxY, newSettings->swapAxes);
	}

}

int saveDeviceSettingsToFile(char * fileName, DeviceSettingsList * list) {
	/* Try to open file */
	FILE * fileDesc = fopen(fileName, "w");
	if (!fileDesc ) {
		return 0;
	}

	int i;
	for(i = 0; i < list->nDeviceSettings; i++) {
		DeviceSettings* profile = &(list->deviceSettings[i]);
		if(profile->inputDeviceName == NULL) {
			/* Has been deleted */
		} else {
			fprintf(fileDesc, "[profile]\n");

			fprintf(fileDesc, "device=%s\n", profile->inputDeviceName);

			if(profile->autoOutput) {
				fprintf(fileDesc, "output=AUTO_FIRST_LVDS\n");
			} else if(profile->attachedOutput) {
				fprintf(fileDesc, "output=%s\n", profile->attachedOutput);
			}

			if(! profile->autoCalibration) {
				fprintf(fileDesc, "minx=%i\n", profile->outputMinX);
				fprintf(fileDesc, "maxx=%i\n", profile->outputMaxX);
				fprintf(fileDesc, "miny=%i\n", profile->outputMinY);
				fprintf(fileDesc, "maxy=%i\n", profile->outputMaxY);
				fprintf(fileDesc, "swapaxes=%i\n", profile->swapAxes);
			}
			fprintf(fileDesc, "\n");
		}
	}


	fclose(fileDesc);
	return 1;
}

int addDeviceSettingsFromFile(char * fileName, DeviceSettingsList * list, char * onlyForDevice) {
	/* Try to open file */
	FILE * fileDesc = fopen(fileName, "r");
	if (!fileDesc ) {
		return 0;
	}

	int profileCount = 0;
	DeviceSettings loadedSettings;
	int calibLoaded = 0;

	char line[256];

	clearDeviceSettingsEntry(&loadedSettings);
	loadedSettings.autoCalibration = 1;

	while(1) {
		char* readline = fgets(line, 256, fileDesc);
		if(!readline) break;
	
		/* Remove newline at end */
		int i; int eqAt = -1;
		for(i = 0; i<256; i++) {
			if(line[i] == '=' && eqAt == -1) eqAt = i;
			if(line[i] == '\n') line[i] = 0;
			if(line[i] == 0) break;
		}
	
		if(!strcmp(line, "[profile]")) {
			/* New profile shall be started */
			if(profileCount > 0) {
				if(onlyForDevice == NULL || (loadedSettings.inputDeviceName != NULL && !strcmp(onlyForDevice, loadedSettings.inputDeviceName))) {
					/* Add previous profile */
					addDeviceSettingsEntry(list, &loadedSettings);
				}
			}
			/* Start new profile */
			clearDeviceSettingsEntry(&loadedSettings);
			loadedSettings.autoCalibration = 1;
			calibLoaded = 0;
			profileCount++;
		} else if(eqAt > 0) {
			char * befEq=strndup(line, eqAt);
			char * afEq=strndup(line + eqAt + 1, strlen(line) - (eqAt + 1));
			if(!befEq || !afEq) {
				fprintf(stderr, "Out of memory.\n");
				exit(1);
			}
			
			if(!strcmp(befEq,"device")) {
				loadedSettings.inputDeviceName = strdup(afEq);
			} else if(!strcmp(befEq,"output")) {
				if(!strcmp(afEq, "AUTO_FIRST_LVDS")) {
					loadedSettings.attachedOutput = NULL;
					loadedSettings.autoOutput = 1;
				} else {
					loadedSettings.attachedOutput = strdup(afEq);
					loadedSettings.autoOutput = 0;
				}
			} else if(!strcmp(befEq,"minx")) {
				loadedSettings.outputMinX = strtol(afEq, NULL, 0);
				calibLoaded |= BIT_0;

			} else if(!strcmp(befEq,"maxx")) {
				loadedSettings.outputMaxX = strtol(afEq, NULL, 0);
				calibLoaded |= BIT_1;
				
			} else if(!strcmp(befEq,"miny")) {
				loadedSettings.outputMinY = strtol(afEq, NULL, 0);
				calibLoaded |= BIT_2;
				
			} else if(!strcmp(befEq,"maxy")) {
				loadedSettings.outputMaxY = strtol(afEq, NULL, 0);
				calibLoaded |= BIT_3;
				
			} else if(!strcmp(befEq,"swapaxes")) {
				loadedSettings.swapAxes = (strtol(afEq, NULL, 0) != 0);
			}
			if(calibLoaded == (BIT_0 | BIT_1 | BIT_2 | BIT_3)) {
				loadedSettings.autoCalibration = 0;
			}

			free(befEq);
			free(afEq);
		}

	}
	if(profileCount > 0) {
		/* Add last profile */
				if(onlyForDevice == NULL || (loadedSettings.inputDeviceName != NULL && !strcmp(onlyForDevice, loadedSettings.inputDeviceName))) {
			addDeviceSettingsEntry(list, &loadedSettings);
		}
	}

	fclose(fileDesc);
	return 1;
}

