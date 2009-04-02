// SolidView.h

#include "wx/wx.h"

// Define a new canvas which can receive some events
class CSolidView: public wxWindow
{
	wxPoint m_button_down_point;
	wxPoint m_current_point;
	wxPoint m_initial_point;
	long m_new_xres;
	long m_new_yres;

public:
	double m_lens_point[3];
	double m_target_point[3];
	double m_vertical[3];
	bool m_construction_finished;
	bool m_initdirectdraw_called;

	CSolidView(wxWindow *parent, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
	~CSolidView(void){};

	void ViewScale(double fraction);
	void LimitCamera();

	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouse( wxMouseEvent& event );

	DECLARE_EVENT_TABLE()
};

