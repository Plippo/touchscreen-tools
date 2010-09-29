using Gtk, Gdk;

extern void* initXlib();
extern void finishXlib(void* display);

public const string SHARE_DIR = "/usr/share/gtouchsett";

int main(string[] args) {
	if(args.length >= 12 && args[1] == "--set-global") {

		DeviceSettings d = DeviceSettings();

		d.inputDeviceName = (char *) args[2];
		string outputName = args[3];
		if(outputName.length == 0) outputName = null;
		d.attachedOutput = (char *) outputName;
		d.autoOutput = 0;
		d.autoCalibration = (args[4].to_int() == 0 ? 0 : 1);
		d.outputMinX = args[5].to_int();
		d.outputMaxX = args[6].to_int();
		d.outputMinY = args[7].to_int();
		d.outputMaxY = args[8].to_int();
		d.inverseX = (args[9].to_int() == 0 ? 0 : 1);
		d.inverseY = (args[10].to_int() == 0 ? 0 : 1);
		d.swapAxes = (args[11].to_int() == 0 ? 0 : 1);

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
