using Gtk, Gdk;

public struct DeviceSettings {
	char * inputDeviceName;
	char * attachedOutput;
	int autoOutput;
	int * inputDeviceIDs;
	int inputDeviceCount;
	int autoCalibration;
	int outputMinX;
	int outputMaxX;
	int outputMinY;
	int outputMaxY;
	int swapAxes;
}

public struct DeviceSettingsList {
	DeviceSettings * deviceSettings;
	int nDeviceSettings;
	int nDeviceSettingsSpace;
}

public struct InputDeviceInformation {
	char* deviceName;
	int deviceID; /* For the last entry in the array, deviceID is -1. */
} 


extern int loadSettings(DeviceSettingsList * list, char * onlyForDevice, char * onlyFile);
extern void freeSettings(DeviceSettingsList * list);
extern char* getGlobalFileName();
extern char* getPrivateFileName();
extern void changeProfile(DeviceSettingsList * list, DeviceSettings * newSettings);
extern void deleteProfile(DeviceSettingsList * list, char * deviceName);
extern int saveDeviceSettingsToFile(char * fileName, DeviceSettingsList * list);

extern InputDeviceInformation * getTouchscreens(void* display);
extern void freeInputDevices(InputDeviceInformation * information);

public struct MonitorInformation {
	public string name;
	public string displayName;
	public int x;
	public int y;
	public int width;
	public int height;
}

public class SettingsWindow {

	public Gtk.Window window;
	HBox hbNoSettings;
	HBox hbNoDevices;
	VBox vboxGeneral;
	VBox vbMain;
	VBox vbDeviceSelection;
	Button btnClose;
	Button btnClearTest;
	Button btnTestFullscreen;
	Button btnCalibrate;
	Button btnMonitors;
	Button btnRevert;
	Button btnApplyForAll;
	TestArea testArea;
	ComboBox cmbOutDevice;
	ComboBox cmbDevice;
	AspectFrame aspScreen;
	
	DrawingArea drwMonitors;
	
	Calibrator calibrator = null;

	MonitorInformation[] monitors;
	/* If set to true, changing the active item in cmbOutDevice will temporarily be ignored */
	bool ignoreOutputChange = false;
	int selectedMonitorIndex = -1;
	int monitorCount = 0;
	int screenWidth;
	int screenHeight;

	InputDeviceInformation * touchscreens = null;

	/* Current settings */
	string selectedDeviceName;
	string oldSelectedMonitorName;
	string selectedMonitorName = null;
	public bool autoCalibration;
	public int outputMinX; public int outputMaxX; public int outputMinY; public int outputMaxY;
	public bool swapAxes;

	void *display;

	public SettingsWindow(void *display) {
		this.display = display;
		Builder builder = new Builder();
		try {
			builder.add_from_file(SHARE_DIR + "/settingswindow.glade");
		} catch(Error e) {
			Gtk.main_quit();
			return;
		}

		window = (Gtk.Window) builder.get_object("dlgSettings");
		hbNoDevices = (HBox) builder.get_object("hbNoDevices");
		hbNoSettings = (HBox) builder.get_object("hbNoSettings");
		vboxGeneral = (VBox) builder.get_object("vboxGeneral");
		btnClose = (Button) builder.get_object("btnClose");
		vbDeviceSelection = (VBox) builder.get_object("vbDeviceSelection");
		btnClearTest = (Button) builder.get_object("btnClearTest");
		btnTestFullscreen = (Button) builder.get_object("btnTestFullscreen");
		btnApplyForAll = (Button) builder.get_object("btnApplyForAll");
		btnRevert = (Button) builder.get_object("btnRevert");
		btnCalibrate = (Button) builder.get_object("btnCalibrate");
		btnMonitors = (Button) builder.get_object("btnMonitors");
		cmbOutDevice = (ComboBox) builder.get_object("cmbOutDevice");
		cmbDevice = (ComboBox) builder.get_object("cmbDevice");
		aspScreen = (AspectFrame) builder.get_object("aspScreen");

		vbMain = (VBox) builder.get_object("vbMain");

		vboxGeneral.remove(hbNoSettings);

		testArea = new TestArea((DrawingArea) builder.get_object("drwTest"), null);

		drwMonitors = (DrawingArea) builder.get_object("drwMonitors");

		connectSignals();

		loadDevices();
	}

	~SettingsWindow() {
		/* NULL is allowed, so we may call freeInputDevices in any case */
		freeInputDevices(touchscreens);
	}

	private void loadDeviceSettings() {
		bool firstLVDS;
		
		DeviceSettingsList list = DeviceSettingsList();
		loadSettings(&list, (char *) selectedDeviceName, null);
		if(list.nDeviceSettings == 0) {
			firstLVDS = true;
			oldSelectedMonitorName = null;
			selectedMonitorName = null;
			autoCalibration = true;
		} else {
			firstLVDS = (list.deviceSettings[0].autoOutput != 0);
			oldSelectedMonitorName = (string) list.deviceSettings[0].attachedOutput;
			selectedMonitorName = oldSelectedMonitorName;
			autoCalibration = (list.deviceSettings[0].autoCalibration != 0);
			outputMinX = list.deviceSettings[0].outputMinX;
			outputMaxX = list.deviceSettings[0].outputMaxX;
			outputMinY = list.deviceSettings[0].outputMinY;
			outputMaxY = list.deviceSettings[0].outputMaxY;
			swapAxes = (list.deviceSettings[0].swapAxes != 0);
		}

		freeSettings(&list);

		loadMonitors();
		if(firstLVDS) selectFirstLVDS();
	}
	
	public void saveDeviceSettings() {
		DeviceSettingsList list = DeviceSettingsList();
		loadSettings(&list, null, getPrivateFileName());

		DeviceSettings d = DeviceSettings();
		d.inputDeviceName = (char *) selectedDeviceName;
		d.attachedOutput = (char *) selectedMonitorName;
		d.autoOutput = 0;
		d.autoCalibration = ( autoCalibration ? 1 : 0);
		d.outputMinX = outputMinX;
		d.outputMaxX = outputMaxX;
		d.outputMinY = outputMinY;
		d.outputMaxY = outputMaxY;
		d.swapAxes = ( swapAxes ? 1 : 0);


		changeProfile(&list, &d);
		saveDeviceSettingsToFile(getPrivateFileName(), &list);
		freeSettings(&list);
	}

	private void resetDeviceSettings() {
		DeviceSettingsList list = DeviceSettingsList();
		loadSettings(&list, null, getPrivateFileName());


		deleteProfile(&list, (char *) selectedDeviceName);
		saveDeviceSettingsToFile(getPrivateFileName(), &list);
		freeSettings(&list);
	}

	private void makeGlobal() {

		try {
			int exitcode;
			string err;
			//TODO escape string
			string cmd = "/usr/bin/gtouchsett --set-global '" + selectedDeviceName + "' '" + (selectedMonitorName == null? "" : selectedMonitorName) + "' " + (autoCalibration?"1":"0") + " " +  (autoCalibration?"0":outputMinX.to_string()) + " " + (autoCalibration?"0":outputMaxX.to_string()) + " " + (autoCalibration?"0":outputMinY.to_string()) + " " + (autoCalibration?"0":outputMaxY.to_string()) + " " + (swapAxes ?"1":"0");
			Process.spawn_sync(null, { "/usr/bin/gksu", "--message", "Please enter your password to apply the settings for all users.", cmd}, null, 0, null, null, out err, out exitcode );

			if(exitcode == 0) {
				resetDeviceSettings();
			} else {
				MessageDialog md = new MessageDialog(window, Gtk.DialogFlags.MODAL, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE, "Error applying the settings for all users");
				md.secondary_text = err;
				md.run();
				md.destroy();
			}


		} catch(SpawnError e) {
			MessageDialog md = new MessageDialog(window, Gtk.DialogFlags.MODAL, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE, "Error applying the settings for all users");
			md.secondary_text = e.message;
			md.run();
			md.destroy();
		}

		reloadHelper();
	}

	public void reloadHelper() {
		try {
			Process.spawn_command_line_async("killall -SIGUSR1 touchscreen-helper");
		} catch(SpawnError e) {
			MessageDialog md = new MessageDialog(window, Gtk.DialogFlags.MODAL, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE, "Error applying the settings");
			md.secondary_text = e.message;
			md.run();
			md.destroy();
		}

	}

	private void connectSignals() {
	        window.destroy.connect(() => {
			Gtk.main_quit();
		});
		btnClose.clicked.connect(() => {
			window.destroy();			
		});
		btnClearTest.clicked.connect(() => {
			testArea.clear();
		});
		btnRevert.clicked.connect(() => {
			resetDeviceSettings();
			reloadHelper();
			loadDeviceSettings();
		});
		btnApplyForAll.clicked.connect(() => {
			makeGlobal();
		});
		btnTestFullscreen.clicked.connect(() => {
			testArea.fullscreen(window, selectedMonitorIndex < monitorCount ? selectedMonitorIndex : -1);
		});
		btnCalibrate.clicked.connect(() => {
			calibrator = new Calibrator(this, selectedMonitorIndex < monitorCount ? selectedMonitorIndex : -1, selectedMonitorName, display, touchscreens[cmbDevice.active].deviceID);
		});
		window.get_screen().monitors_changed.connect(() => {
			loadMonitors();
		});
		btnMonitors.clicked.connect(() => {
			try {
				Process.spawn_command_line_async("gnome-display-properties");
			} catch(SpawnError e) {
				MessageDialog md = new MessageDialog(window, Gtk.DialogFlags.MODAL, Gtk.MessageType.ERROR, Gtk.ButtonsType.CLOSE, "Error loading the application");
				md.secondary_text = e.message;
				md.run();
				md.destroy();
			}

		});
		cmbOutDevice.changed.connect(() => {
			if(!ignoreOutputChange) {
				selectedMonitorIndex = cmbOutDevice.active - 1;
				if(selectedMonitorIndex == -1) {
					selectedMonitorName = null;
				} else if(selectedMonitorIndex == monitorCount) {
					selectedMonitorName = oldSelectedMonitorName;
				} else {
					selectedMonitorName = monitors[selectedMonitorIndex].name;
				}
				drwMonitors.queue_draw();
				saveDeviceSettings();
				reloadHelper();
			}
		});
		cmbDevice.changed.connect(() => {
			if(cmbDevice.active >= 0) {
				selectedDeviceName = (string) touchscreens[cmbDevice.active].deviceName;
				loadDeviceSettings();
			}
		});

		drwMonitors.expose_event.connect(() => {
			drwMonitors.window.draw_rectangle(drwMonitors.style.bg_gc[Gtk.StateType.NORMAL],true,0,0,drwMonitors.allocation.width, drwMonitors.allocation.height);
			drawMonitors();			
			return true;
		});
		drwMonitors.button_press_event.connect((sender, evt) => {
			double relation = drwMonitors.allocation.width / (double) screenWidth;
			for(int i = 0; i < monitorCount; i++) {
				int x = (int) (monitors[i].x * relation);
				int y = (int) (monitors[i].y * relation);
				int w = ((int) ((monitors[i].x + monitors[i].width) * relation)) - x;
				int h = ((int) ((monitors[i].y + monitors[i].height) * relation)) - y;
				if(evt.x >= x + 1 && evt.x < x + w - 3 && evt.y >= y + 1 && evt.y < y + h - 3) {
					cmbOutDevice.active = i + 1;
					return true;
				}
			}
			//cmbOutDevice.active = 0;
			return true;
		});

		
		
	}

	private void drawMonitors() {
		double relation = drwMonitors.allocation.width / (double) screenWidth;
		int sw = (int) (screenWidth * relation);
		int sh = (int) (screenHeight * relation);
		if(selectedMonitorIndex == -1) {
			drwMonitors.window.draw_rectangle(drwMonitors.style.bg_gc[Gtk.StateType.SELECTED], true, 0, 0, sw, sh);
		}
		for(int i = 0; i < monitorCount; i++) {
			int x = (int) (monitors[i].x * relation);
			int y = (int) (monitors[i].y * relation);
			int w = ((int) ((monitors[i].x + monitors[i].width) * relation)) - x;
			int h = ((int) ((monitors[i].y + monitors[i].height) * relation)) - y;
			/* Shadow; only if not all are selected */
			if(selectedMonitorIndex != -1) {
				drwMonitors.window.draw_rectangle(drwMonitors.style.dark_gc[Gtk.StateType.NORMAL], true, x + 2, y + 2, w - 2, h - 2);
			}
			drwMonitors.window.draw_rectangle(drwMonitors.style.base_gc[Gtk.StateType.NORMAL], true, x + 1, y + 1, w - 2, h - 2);
			drwMonitors.window.draw_rectangle(drwMonitors.style.text_gc[Gtk.StateType.NORMAL],false, x + 1, y + 1, w - 3, h - 3);
			if(i == selectedMonitorIndex) {
				drwMonitors.window.draw_rectangle(drwMonitors.style.bg_gc[Gtk.StateType.SELECTED] ,false,
					x + 2, y + 2, w - 5, h - 5);
				drwMonitors.window.draw_rectangle(drwMonitors.style.base_gc[Gtk.StateType.SELECTED] ,false,
					x + 3, y + 3, w - 7, h - 7);
			}
			Gdk.GC gc = new Gdk.GC(drwMonitors.window);
			gc.set_foreground(drwMonitors.style.text[Gtk.StateType.NORMAL]);
			Gdk.Rectangle r = Gdk.Rectangle();
			r.x = x + 2; r.y = y + 2; r.width = w - 4; r.height = h - 4;
			gc.set_clip_rectangle(r);
			
			Pango.Layout layout = drwMonitors.create_pango_layout(monitors[i].displayName);
			int fw, fh;
			layout.get_pixel_size(out fw, out fh);
			Gdk.draw_layout(drwMonitors.window,gc,x + (w - fw) / 2, y + (h - fh) / 2, layout);
		}
	}

	public void show() {
		window.show();
	}

	private void loadDevices() {
		int deviceCount;

		/* NULL is allowed, so we may call freeInputDevices in any case */
		freeInputDevices(touchscreens);

		touchscreens = getTouchscreens(display);
		if(touchscreens == null) {
			deviceCount = 0;
		} else {
			int i = -1;
			while(true) {
				i++;
				if(touchscreens[i].deviceID == -1) {
					break;
				}
			}
			deviceCount = i;
		}

		if(cmbDevice.get_model() == null) {
			/* Is first call */
			var iCell = new CellRendererPixbuf ();
			iCell.stock_size=Gtk.IconSize.LARGE_TOOLBAR;
			cmbDevice.pack_start(iCell, false);
			cmbDevice.add_attribute(iCell, "icon-name", 1);

			var cell = new CellRendererText ();
			cmbDevice.pack_start(cell, true);
			cmbDevice.set_attributes(cell, "text", 0);
		}


		var model = new ListStore(3, typeof (string), typeof (string), typeof(int));
		cmbDevice.set_model(model);
		TreeIter iter;

		for(int i = 0; i < deviceCount; i++) {
			model.append (out iter);
			model.set(iter, 0, (string) touchscreens[i].deviceName, 1, "touchscreen", 2, touchscreens[i].deviceID);
		}


		vboxGeneral.set_visible(deviceCount > 0);
		hbNoDevices.set_visible(deviceCount == 0);
		vbDeviceSelection.set_visible(deviceCount > 1);

		if(deviceCount > 0) {
			cmbDevice.active = 0;
			/* Will trigger loadDeviceSettings() */

		}
	}


	private void selectFirstLVDS() {
		selectedMonitorIndex = -1;
		for(int i = 0; i < monitorCount; i++) {
			if(monitors[i].name.contains("lvds") || monitors[i].name.contains("LVDS")) {
				selectedMonitorIndex = i;
				selectedMonitorName = monitors[i].name;
				break;
			}
		}
		ignoreOutputChange = true;
		cmbOutDevice.active = selectedMonitorIndex + 1;
		drwMonitors.queue_draw();
		ignoreOutputChange = false;
	}

	private string getDisplayName(string name) {
		if(name.has_prefix("LVDS") || name.has_prefix("lvds")) {
			return "Integrated Monitor " + name.substring(4, -1);
		}
		return name;
	}

	private void loadMonitors() {
		selectedMonitorIndex = -1;
		bool oldMonitorFound = false;

		if(cmbOutDevice.get_model() == null) {
			/* Is first call */
			var iCell = new CellRendererPixbuf ();
			iCell.stock_size=Gtk.IconSize.LARGE_TOOLBAR;
			cmbOutDevice.pack_start(iCell, false);
			cmbOutDevice.add_attribute(iCell, "icon-name", 1);
			var cell = new CellRendererText ();
			cell.ellipsize_set = true;
			cell.ellipsize = Pango.EllipsizeMode.END;
			cmbOutDevice.pack_start(cell, true);
			cmbOutDevice.add_attribute(cell, "markup", 0);
		}
		Gdk.Screen screen = window.get_screen();
		monitorCount = screen.get_n_monitors();
		monitors = new MonitorInformation[monitorCount];
		for(int i = 0; i < monitorCount; i++) {
			monitors[i].name = screen.get_monitor_plug_name(i);
			if(oldSelectedMonitorName != null && monitors[i].name == oldSelectedMonitorName) oldMonitorFound = true;
			if(selectedMonitorName != null && monitors[i].name == selectedMonitorName) selectedMonitorIndex = i;
			monitors[i].displayName = getDisplayName(monitors[i].name);
			Gdk.Rectangle r;
			screen.get_monitor_geometry(i, out r);
			monitors[i].x = r.x;
			monitors[i].y = r.y;
			monitors[i].width = r.width;
			monitors[i].height = r.height;
		}
		
		var model = new ListStore(4, typeof (string), typeof(string), typeof(int), typeof(bool));
		cmbOutDevice.set_model(model);
		TreeIter iter;

		model.append (out iter);
		//model.set(iter, 0, " " +"Whole Desktop"+(monitorCount == 1 ? " <b>(Ignores Rotation)</b>" : ""), 1, "user-desktop", 2, -1, 3, true);
		if(monitorCount == 1) {
			model.set(iter, 0, " " +"<i>None</i> – Stand-alone device", 1, "user-desktop", 2, -1, 3, true);
		} else {
			model.set(iter, 0, " " +"<i>None</i> – Whole desktop", 1, "user-desktop", 2, -1, 3, true);
		}
		for(int i = 0; i < monitorCount; i++) {
			string name = monitors[i].displayName;
			model.append (out iter);
			model.set(iter, 0, " " + name, 1, "video-display", 2, i, 3, true);
		}
		if(oldSelectedMonitorName != null && !oldMonitorFound) {
			model.append(out iter);
			model.set(iter, 0, " <i>" + getDisplayName(oldSelectedMonitorName) + " (not available)</i>", 1, "dialog-question", 2, monitorCount, 3, false);
			if(selectedMonitorName == oldSelectedMonitorName) {
				selectedMonitorIndex = monitorCount;
			}
		}
		ignoreOutputChange = true;
		cmbOutDevice.active = selectedMonitorIndex + 1;
		ignoreOutputChange = false;
		
		screenWidth = screen.get_width();
		screenHeight = screen.get_height();
		
		aspScreen.ratio = screenWidth / (float) screenHeight;

		drwMonitors.queue_draw();
	}

}

