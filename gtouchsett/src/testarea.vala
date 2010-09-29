using Gtk, Gdk;

public class TestArea {

	public DrawingArea drwTest;
	bool testDown = false;
	Cairo.Context testContext = null;
	double testLastX = 0;
	double testLastY = 0;
	public Gdk.Pixmap testPixmap;

	public TestAreaFullscreen fullscreenWindow = null;

	public TestArea(DrawingArea drawingArea, Gdk.Pixmap? pixmap) {
		drwTest = drawingArea;
		testPixmap = pixmap;

		//drwTest.modify_bg(Gtk.StateType.NORMAL, drwTest.style.white);

		connectSignals();
	}
	

	private void connectSignals() {
		drwTest.configure_event.connect(() => {
			int w=0, h=0;
			if(testPixmap != null) testPixmap.get_size(out w, out h);
			/* Make pixmap larger if necessary. */
			int nw = max(w, drwTest.allocation.width);
			int nh = max(h, drwTest.allocation.height);
			if(nw!=w || nh!=h) {
				var pixmap = new Gdk.Pixmap(drwTest.window, nw, nh, -1);
				pixmap.draw_rectangle(drwTest.style.white_gc, true, 0, 0, nw, nh);
				if(testPixmap != null) {
					/* Draw old content */
					pixmap.draw_drawable(drwTest.style.fg_gc[Gtk.StateType.NORMAL],testPixmap, 0, 0, 0, 0,  w, h);
				}
				testPixmap = pixmap;
			}
			return true;
		});
		drwTest.expose_event.connect((sender, evt) => {
			drwTest.window.draw_drawable(drwTest.style.fg_gc[Gtk.StateType.NORMAL],testPixmap, evt.area.x, evt.area.y, evt.area.x, evt.area.y, evt.area.width, evt.area.height);
			return true;
		});
		drwTest.button_press_event.connect((sender, evt) => {
			testDown = true;
			testContext = Gdk.cairo_create(testPixmap);
			testContext.set_line_width(1.0);
			testLastX = evt.x;
			testLastY = evt.y;
			return true;
		});
		drwTest.button_release_event.connect((sender,evt) => {
			if(testDown) {
				testDown = false;
				drawTestSegment(evt.x, evt.y);
				testContext = null;
			}
			return true;
		});
		drwTest.motion_notify_event.connect((sender,evt) => {
			if(testDown) {
				drawTestSegment(evt.x, evt.y);
			}
			return true;
		});

	}
	
	private int max(int x1, int x2) {
		return x1>x2?x1:x2;
	}

	private double minDbl(double x1, double x2) {
		return x1<x2?x1:x2;
	}
	private double maxDbl(double x1, double x2) {
		return x1>x2?x1:x2;
	}


	private void drawTestSegment(double x, double y) {
		testContext.set_source_rgb(0,0,0);
		testContext.move_to(testLastX, testLastY);
		testContext.line_to(x,y);
		testContext.stroke();
		drwTest.queue_draw_area((int) minDbl(testLastX, x) - 1, (int) minDbl(testLastY, y) - 1, (int) maxDbl(testLastX, x)- (int) minDbl(testLastX, x) + 2, (int) maxDbl(testLastY, y)- (int) minDbl(testLastY, y) + 2);
		testLastX = x;
		testLastY = y;
	}


	public void clear() {
		int w, h;
		testPixmap.get_size(out w, out h);
		testPixmap.draw_rectangle(drwTest.style.white_gc, true, 0, 0, w, h);
		drwTest.queue_draw();
	}
	
	public void fullscreen(Gtk.Window parent, int monitor) {
		fullscreenWindow = new TestAreaFullscreen(this, parent, monitor);
	
	}
	
	public void redraw() {
		drwTest.queue_draw();
	}

}

public class TestAreaFullscreen {

	//~TestAreaFullscreen() {
	//	stdout.printf("Destroy\n");
	//}

	uint KEY_ESCAPE = Gdk.keyval_from_name("Escape"); 

	Gtk.Window window;
	TestArea testArea;
	Button btnLeave;
	Button btnClear;
	Button btnBarUp;
	Button btnBarDown;
	TestArea parentTestArea;
	HSeparator toolbarSeparator;
	HBox boxToolbar;
	VBox boxMain;

	public TestAreaFullscreen(TestArea parentTestArea, Gtk.Window parentWindow, int monitor) {
		this.parentTestArea = parentTestArea;
		Builder builder = new Builder();
		try {
			builder.add_from_file(SHARE_DIR + "/testfullscreen.glade");
		} catch(Error e) {
			Gtk.main_quit();
			return;
		}

		window = (Gtk.Window) builder.get_object("winFullScreenTest");
		testArea = new TestArea(builder.get_object("drwTest") as DrawingArea, parentTestArea.testPixmap);
		btnLeave = (Button) builder.get_object("btnLeave");
		btnClear = (Button) builder.get_object("btnClear");
		btnBarUp = (Button) builder.get_object("btnBarUp");
		btnBarDown = (Button) builder.get_object("btnBarDown");
		boxToolbar = (HBox) builder.get_object("boxToolbar");
		toolbarSeparator = (HSeparator) builder.get_object("toolbarSeparator");
		boxMain = (VBox) builder.get_object("boxMain");
		
		
		connect_signals();
		
		window.set_modal(true);
		window.set_transient_for(parentWindow);
		Gdk.Screen screen = window.get_screen();
		if(monitor >= 0 && monitor < screen.get_n_monitors()) {
			Gdk.Rectangle rct;
			screen.get_monitor_geometry(monitor, out rct);
			window.move(rct.x, rct.y);
		}
		window.fullscreen();
		
		window.show();
	}
	
	private void close() {
		parentTestArea.testPixmap = testArea.testPixmap;
		parentTestArea.redraw();
		window.dispose();
		//parentTestArea.fullscreenWindow = null;
	}
	
	private void connect_signals() {
	
		window.key_press_event.connect((widget, evt) => {
			if(evt.keyval == KEY_ESCAPE) {
				close();
				return true;
			}
			return false;
		});
		btnLeave.clicked.connect(() => {
			close();
		});
		btnClear.clicked.connect(() => {
			testArea.clear();
		});
		btnBarUp.clicked.connect(() => {
			boxMain.reorder_child(boxToolbar,0);
			boxMain.reorder_child(toolbarSeparator,1);
			btnBarUp.set_visible(false);
			btnBarDown.set_visible(true);
		});
		btnBarDown.clicked.connect(() => {
			boxMain.reorder_child(testArea.drwTest,0);
			boxMain.reorder_child(toolbarSeparator,1);
			btnBarDown.set_visible(false);
			btnBarUp.set_visible(true);
		});
		window.delete_event.connect(() => {
			close();
			return true;
		});
	}
}
