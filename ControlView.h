// ControlView.h

#include "wx/wx.h"

// Define a new canvas which can receive some events
class CControlView: public wxWindow
{
public:
	CControlView(wxWindow *parent);
	~CControlView(void){};

	DECLARE_EVENT_TABLE()
};

