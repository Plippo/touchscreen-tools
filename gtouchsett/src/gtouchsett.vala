using Gtk, Gdk;

extern void* initXlib();
extern void finishXlib(void* display);

public const string SHARE_DIR = "/usr/share/gtouchsett";

int main(string[] args) {
	if(args.length >= 10 && args[1] == "--set-global") {

		DeviceSettings d = DeviceSettings();

		d.inputDeviceName = (char *) args[2];
		string outputName = args[3];
		if(outputName.length == 0) outputName = null;
		d.attachedOutput = (char *) outputName;
		d.autoOutput = 0;
		d.autoCalibration = (int.parse(args[4]) == 0 ? 0 : 1);
		d.outputMinX = int.parse(args[5]);
		d.outputMaxX = int.parse(args[6]);
		d.outputMinY = int.parse(args[7]);
		d.outputMaxY = int.parse(args[8]);
		d.swapAxes = (int.parse(args[9]) == 0 ? 0 : 1);

		DeviceSettingsList list = DeviceSettingsList();
		loadSettings(&list, null, getGlobalFileName());

		changeProfile(&list, &d);
		int result = saveDeviceSettingsToFile(getGlobalFileName(), &list);
		freeSettings(&list);
		if(result == 1) {
			stdout.printf("Settings successfully applied.\n");
			return 0;
		} else {
			string f = (string) getGlobalFileName();
			stderr.printf("Could not save '%s'.\n", f);
			return 1;
		}


	} else {

		/* As there seems to be no way to get the Xlib Display object from GTK, we need our
		   own connection to the X Server to be able to access XInput2 directly for the calibration. */
		void* display = initXlib();
			/* GTK initialisation and main loop */
		Gtk.init(ref args);
		SettingsWindow sw = new SettingsWindow(display);
		sw.show();
		Gtk.main();

		/* Close our own Xlib connection. */
		finishXlib(display);
	}
	return 0;
}
