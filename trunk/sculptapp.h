// sculptapp.h

// copied from Julian Smart's pngdemo.h

#include "wx/wx.h"

// Define a new application
class MyApp: public wxApp
{
  public:
    MyApp(void){};
	virtual bool OnInit(void);
    int OnExit();
};

// Define a new frame
class MyCanvas;

class MyFrame: public wxFrame
{
  public:
    MyCanvas *canvas;
    MyFrame(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~MyFrame();

    void OnActivate(bool) {}
	void OnClose( wxCloseEvent& event );
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
	void OnRunScript(wxCommandEvent& event);
DECLARE_EVENT_TABLE()
};

// Define a new canvas which can receive some events
class MyCanvas: public wxWindow
{
	wxPoint m_button_down_point;
	wxPoint m_current_point;
	wxPoint m_initial_point;

public:
	double m_lens_point[3];
	double m_target_point[3];
	double m_vertical[3];

	MyCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size);
	~MyCanvas(void){};

	void ViewScale(double fraction);
	void LimitCamera();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouse( wxMouseEvent& event );

	DECLARE_EVENT_TABLE()
};



