// VCApp.cpp

#include "wx/wxprec.h"
#include "wx/splitter.h"

#include "VCApp.h"
#include "VCFrame.h"
#include "SolidView.h"
#include "Tool.h"
#include "voxlap5.h"

IMPLEMENT_APP(VCApp)

VCApp::VCApp(void){
	m_frame = NULL;
	m_xminus_pressed = false;
	m_xplus_pressed = false;
	m_yminus_pressed = false;
	m_yplus_pressed = false;
	m_zminus_pressed = false;
	m_zplus_pressed = false;

	m_current_tool = new CTool();
}

VCApp::~VCApp()
{
	delete m_current_tool;
}

bool VCApp::OnInit(void)
{
	// initialise image handlers
	wxInitAllImageHandlers();

	// Create the main frame window
	m_frame = new VCFrame(NULL, _T("VoxelCut"), wxPoint(0, 0), wxSize(800, 600));

	m_frame->Show(true);

	m_frame->m_solid_view->m_construction_finished = true;

	return true;
}

int VCApp::OnExit()
{
	int result = wxApp::OnExit();
	return result;
}
