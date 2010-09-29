using Gtk, Gdk;

extern void getRandrData();

public class SettingsWindow {

	Gtk.Window window;
	HBox hbNoSettings;
	HBox hbNoDevices;
	VBox vboxGeneral;
	VBox vbMain;
	VBox vbDeviceSelection;
	Button btnClose;
	Button btnClearTest;
	
	DrawingArea drwTest;
	bool testDown = false;
	GC testGC = null;
	int testLastX = 0;
	int testLastY = 0;

	public SettingsWindow() {
		Builder builder = new Builder();
		try {
			builder.add_from_file("settings.glade");
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
		drwTest = (DrawingArea) builder.get_object("drwTest");
		btnClearTest = (Button) builder.get_object("btnClearTest");

		vbMain = (VBox) builder.get_object("vbMain");

		vboxGeneral.remove(hbNoSettings);

		drwTest.add_events(EventMask.POINTER_MOTION_MASK | EventMask.BUTTON_PRESS_MASK | EventMask.BUTTON_RELEASE_MASK );
		Color clrWhite;
		Color.parse("#ffffff", out clrWhite);
		drwTest.modify_bg(Gtk.StateType.NORMAL, clrWhite);

		connectSignals();

		loadDevices();
		loadScreens();
	}
	
	private void connectSignals() {
	        window.destroy.connect (() => {
			Gtk.main_quit();
		});
		btnClose.clicked.connect(() => {
			window.destroy();			
		});
		drwTest.button_press_event.connect((sender, evt) => {
			testDown = true;
			Color clrBlack;
			Color.parse("#000000", out clrBlack);
			testGC = new GC(drwTest.window);
			testGC.set_foreground(clrBlack);
			testLastX = (int) evt.x;
			testLastY = (int) evt.y;
			stdout.printf("%i\n", testLastX);
			return false;
		});
		drwTest.button_release_event.connect((sender,evt) => {
			if(testDown) {
				testDown = false;
				drawTestSegment((int) evt.x, (int) evt.y);
				testGC.dispose();
				testGC = null;
			}
			return false;
		});
		drwTest.motion_notify_event.connect((sender,evt) => {
			if(testDown) {
				drawTestSegment((int) evt.x, (int) evt.y);
			}
			return false;
		});
		btnClearTest.clicked.connect(() => {
			drwTest.window.clear();
		});
	}

	private void drawTestSegment(int x, int y) {
		Point pt1 = Point();
		Point pt2 = Point();
		pt1.x = testLastX;
		pt1.y = testLastY;
		pt2.x = x;
		pt2.y = y;
		drwTest.window.draw_lines(testGC, {pt1, pt2});
		testLastX = x;
		testLastY = y;
	}

	public void show() {
		window.show_all();
	}

	private void loadDevices() {
		int deviceCount = 2;

		if(deviceCount == 0) {
			vbMain.remove(vboxGeneral);
		} else {
			vbMain.remove(hbNoDevices);
		}
		if(deviceCount == 0 || deviceCount == 1) {
			vbMain.remove(vbDeviceSelection);
		}
	}

	private void loadScreens() {
		//stdout.printf("%i\n", a);
	}

}

int main(string[] args) {
	Gtk.init(ref args);
	SettingsWindow sw = new SettingsWindow();
	sw.show();
	Gtk.main();
	return 0;
}
