// VCFrame.cpp

#include "VCFrame.h"
#include "VCApp.h"
#include "SolidView.h"
#include "ControlView.h"
#include "voxlap5.h"
#include "Tool.h"

extern long initapp (long argc, char **argv);
extern HWND ghwnd;
extern void VoxelCutRunScript();
extern long xres, yres;

enum EnumMessageID{
	MenuAbout = 1,
	MenuQuit,
	MenuRun
};

BEGIN_EVENT_TABLE(VCFrame, wxFrame)
	EVT_CLOSE(VCFrame::OnClose)
    EVT_MENU(MenuQuit,      VCFrame::OnQuit)
    EVT_MENU(MenuAbout,     VCFrame::OnAbout)
    EVT_MENU(MenuRun,     VCFrame::OnRunScript)
    EVT_TIMER(wxID_ANY, VCFrame::OnTimerEvent)
END_EVENT_TABLE()

// Define my frame constructor
VCFrame::VCFrame(wxWindow *parent, const wxString& title, const wxPoint& pos, const wxSize& size):wxFrame(parent, wxID_ANY, title, pos, size)
{
#if wxUSE_STATUSBAR
	// Give it a status line
	CreateStatusBar(2);
#endif

	// Make a menubar
	wxMenu *file_menu = new wxMenu;
	wxMenu *help_menu = new wxMenu;
	wxMenu *run_menu = new wxMenu;

	file_menu->Append(MenuQuit, _T("E&xit"),                _T("Quit program"));
	help_menu->Append(MenuAbout, _T("&About"),              _T("About VoxelCut"));
	run_menu->Append(MenuRun, _T("&Run Test Script"),              _T("About VoxelCut"));

	wxMenuBar *menu_bar = new wxMenuBar;

	menu_bar->Append(file_menu, _T("&File"));
	menu_bar->Append(run_menu, _T("&Run"));
	menu_bar->Append(help_menu, _T("&Help"));

	// Associate the menu bar with the frame
	SetMenuBar(menu_bar);

	// make the splitter window
    m_splitter = new wxSplitterWindow(this);  
    m_splitter->SetSashGravity(0.0);

	CControlView* control_view = new CControlView(m_splitter);

	// make the solid view
	m_solid_view = new CSolidView(m_splitter, wxPoint(0, 0), wxSize(400, 400));
	//wxTextCtrl* tttt2 = new wxTextCtrl(m_splitter, wxID_ANY);

    m_splitter->SplitVertically(control_view, m_solid_view, 5);

	ghwnd = (HWND)(m_solid_view->GetHandle());

	xres = m_solid_view->GetSize().GetWidth();
	yres = m_solid_view->GetSize().GetHeight();

#if wxUSE_STATUSBAR
	SetStatusText(_T("Hello, wxWidgets"));
#endif

	initapp(0,NULL);

	{
		// make a cuboid
		lpoint3d p0, p1;
		p0.x = 212;
		p0.y = 212;
		p0.z = 0;
		p1.x = 812;
		p1.y = 812;
		p1.z = 256;
		setrect(&p0, &p1, 0);
	}
}

// frame destructor
VCFrame::~VCFrame()
{
}

void VCFrame::OnClose( wxCloseEvent& event )
{
	event.Skip();
}

void VCFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void VCFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    (void)wxMessageBox(_T("VoxelCut\n built using the voxlap engine by Ken Silverman"),
            _T("About VoxelCut"), wxOK);
}

void VCFrame::OnRunScript(wxCommandEvent& WXUNUSED(event))
{
	VoxelCutRunScript();
}

void Repaint()
{
	wxGetApp().m_frame->m_solid_view->Refresh(0);
	wxGetApp().m_frame->m_solid_view->Update();
	while (wxGetApp().Pending())
		wxGetApp().Dispatch();

}

void VCFrame::OnTimerEvent( wxTimerEvent& WXUNUSED(event) )
{
	if(wxGetApp().m_xminus_pressed)
	{
		wxGetApp().m_current_tool->m_p0[0]--;
		wxGetApp().m_current_tool->m_p1[0]--;
	}
	if(wxGetApp().m_xplus_pressed)
	{
		wxGetApp().m_current_tool->m_p0[0]++;
		wxGetApp().m_current_tool->m_p1[0]++;
	}
	if(wxGetApp().m_yminus_pressed)
	{
		wxGetApp().m_current_tool->m_p0[1]--;
		wxGetApp().m_current_tool->m_p1[1]--;
	}
	if(wxGetApp().m_yplus_pressed)
	{
		wxGetApp().m_current_tool->m_p0[1]++;
		wxGetApp().m_current_tool->m_p1[1]++;
	}
	if(wxGetApp().m_zminus_pressed)
	{
		wxGetApp().m_current_tool->m_p0[2]--;
		wxGetApp().m_current_tool->m_p1[2]--;
	}
	if(wxGetApp().m_zplus_pressed)
	{
		wxGetApp().m_current_tool->m_p0[2]++;
		wxGetApp().m_current_tool->m_p1[2]++;
	}
	Repaint();
}
