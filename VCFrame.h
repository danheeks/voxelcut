// VCFrame.h
#include "wx/wx.h"
#include "wx/splitter.h"

// The main VoxelCut frame window
class CSolidView;

class VCFrame: public wxFrame
{
  public:
	wxScrolledWindow *m_left;
	CSolidView *m_solid_view; // right pane
	wxSplitterWindow* m_splitter;
    wxTimer m_animTimer;

    VCFrame(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size);
    virtual ~VCFrame();

    void OnActivate(bool) {}
	void OnClose( wxCloseEvent& event );
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
	void OnRunScript(wxCommandEvent& event);
	void OnTimerEvent( wxTimerEvent& WXUNUSED(event) );
DECLARE_EVENT_TABLE()
};
