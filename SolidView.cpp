// SolidView.cpp
#include "wx/wxprec.h"
#include "SolidView.h"
#include "voxlap5.h"
#include "VCApp.h"
#include "VCFrame.h"
#include "Tool.h"

extern void doframe ();
extern long xres, yres, colbits, fullscreen, maxpages;
extern long initdirectdraw (long daxres, long dayres, long dacolbits);
extern void uninitdirectdraw ();

BEGIN_EVENT_TABLE(CSolidView, wxWindow)
    EVT_PAINT(CSolidView::OnPaint)
    EVT_SIZE(CSolidView::OnSize)
    EVT_MOUSE_EVENTS(CSolidView::OnMouse)
END_EVENT_TABLE()

// Define a constructor for my canvas
CSolidView::CSolidView(wxWindow *parent, const wxPoint& pos, const wxSize& size):
 wxWindow(parent, wxID_ANY, pos, size, wxTRANSPARENT_WINDOW),m_construction_finished(false), m_initdirectdraw_called(false), m_new_xres(0), m_new_yres(0)
{
	m_lens_point[0] = 256.0;
	m_lens_point[1] = 512.0;
	m_lens_point[2] = 384.0;
	m_target_point[0] = 512.0;
	m_target_point[1] = 512.0;
	m_target_point[2] = 128.0;
	m_vertical[0] = 0.707;
	m_vertical[1] = 0.0;
	m_vertical[2] = 0.707;
}

// Define the repainting behaviour
void CSolidView::OnPaint(wxPaintEvent& event)
{
	if(m_construction_finished)
	{
		if(m_new_xres > 0 && m_new_yres > 0 && ((m_new_xres != xres) || (m_new_yres != yres)))
		{
			uninitdirectdraw();
			xres = m_new_xres;
			yres = m_new_yres;
			initdirectdraw(xres,yres, colbits);
			m_initdirectdraw_called = true;
		}
		if(m_initdirectdraw_called)
		{
			if(wxGetApp().m_current_tool->m_cutting)
			{
				lpoint3d p0, p1;
				p0.x = wxGetApp().m_current_tool->m_p0[0];
				p0.y = wxGetApp().m_current_tool->m_p0[1];
				p0.z = 256 - wxGetApp().m_current_tool->m_p0[2];
				p1.x = wxGetApp().m_current_tool->m_p1[0];
				p1.y = wxGetApp().m_current_tool->m_p1[1];
				p1.z = 256 - wxGetApp().m_current_tool->m_p1[2];
				long cr = wxGetApp().m_current_tool->m_r;
				setcylinder(&p0, &p1, cr, -1, 0);
				updatevxl();
			}

			doframe();
		}
	}

	event.Skip();
}

void CSolidView::OnSize(wxSizeEvent& event)
{
	if(!m_construction_finished)
	{
		m_new_xres = 0;
		m_new_yres = 0;
	}
	else
	{
		if(event.GetSize().GetWidth() > 0)m_new_xres = event.GetSize().GetWidth();
		if(event.GetSize().GetHeight() > 0)m_new_yres = event.GetSize().GetHeight();
	}

	event.Skip();
	//Refresh();
}

void crossp(const double* a, const double* b, double* ab)
{
	ab[0] = a[1]*b[2] - a[2]*b[1];
	ab[1] = a[2]*b[0] - a[0]*b[2];
	ab[2] = a[0]*b[1] - a[1]*b[0];
}

void norm(double* v)
{
	double magn = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	if(magn<0.0000000000000001)return;
	v[0] /= magn;
	v[1] /= magn;
	v[2] /= magn;
}

double dist(const double* p1, const double* p2)
{
	double d[3];
	for(int i = 0; i<3; i++)d[i] = p2[i] - p1[i];
	double dist = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
	return dist;
}

void CSolidView::ViewScale(double fraction){
	double multiplier = fraction;
	bool increasing=(multiplier>0);
	if(!increasing)multiplier = -multiplier;
	multiplier = 1 - multiplier;
	if(multiplier<0.00001)multiplier=0.00001;
	if(increasing)multiplier = 1/multiplier;
	if(multiplier< 0.1)multiplier = 0.1;
	// for perspective, move forward
	double f[3];
	for(int i = 0; i<3; i++)f[i] = m_target_point[i] - m_lens_point[i];
	double v[3];
	for(int i = 0; i<3; i++)v[i] = f[i] * fraction;
	for(int i = 0; i<3; i++)m_lens_point[i] += v[i];
	if(dist(m_lens_point, m_target_point) < 10){
		norm(f);
		for(int i = 0; i<3; i++)m_target_point[i] = m_lens_point[i] + f[i] * 10;
	}

	LimitCamera();
}

void CSolidView::LimitCamera(){
	if(m_lens_point[0] < 1)m_lens_point[0] = 1;
	if(m_lens_point[0] > 1023)m_lens_point[0] = 1023;
	if(m_lens_point[1] < 1)m_lens_point[1] = 1;
	if(m_lens_point[1] > 1023)m_lens_point[1] = 1023;
	if(m_lens_point[2] < 0)m_lens_point[2] = 0;
	if(m_lens_point[2] > 2048)m_lens_point[2] = 2048;

	// recalculate m_vertical
	double f[3];
	for(int i = 0; i<3; i++)f[i] = m_target_point[i] - m_lens_point[i];
	norm(f);
	double right[3];
	crossp(f, m_vertical, right);
	crossp(right, f, m_vertical);
	norm(m_vertical);
}

void CSolidView::OnMouse( wxMouseEvent& event )
{
	if(event.Entering()){
		SetFocus(); // so middle wheel works
	}

	if(event.LeftDown() || event.MiddleDown() || event.RightDown())
	{
		m_button_down_point = wxPoint(event.GetX(), event.GetY());
		m_current_point = m_button_down_point;
		//StoreViewPoint();
		m_initial_point = m_button_down_point;
	}
	else if(event.Dragging())
	{
		wxPoint point_diff;
		point_diff.x = event.GetX() - m_current_point.x;
		point_diff.y = event.GetY() - m_current_point.y;

		if(event.LeftIsDown())
		{
			if(point_diff.x > 100)point_diff.x = 100;
			else if(point_diff.x < -100)point_diff.x = -100;
			if(point_diff.y > 100)point_diff.y = 100;
			else if(point_diff.y < -100)point_diff.y = -100;
			wxSize size = GetClientSize();
			double c=(size.GetWidth()+size.GetHeight())/20;

			double ang_x = point_diff.x/c;
			double ang_y = point_diff.y/c;
			
			double f[3];
			for(int i = 0; i<3; i++)f[i] = m_target_point[i] - m_lens_point[i];
			double fl = sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
			double uu[3];
			for(int i = 0; i<3; i++)uu[i] = m_vertical[i];

			double r[3];
			crossp(f, uu, r);
			norm(r);

			double sinangx = sin(ang_x);
			double oneminuscosangx = 1 - cos(ang_x);
			double sinangy = sin(ang_y);
			double oneminuscosangy = 1 - cos(ang_y);

			for(int i = 0; i<3; i++)
			{
				m_lens_point[i] = m_lens_point[i] - r[i] * sinangx*fl;
				m_lens_point[i] = m_lens_point[i] + f[i] * oneminuscosangx;
				f[i] = f[i] + r[i] * sinangx * fl;
				f[i] = f[i] - f[i] * oneminuscosangx;
			}

			crossp(f, uu, r);
			norm(r);

			for(int i = 0; i<3; i++)
			{
				m_lens_point[i] = m_lens_point[i] + uu[i] * sinangy*fl;
				m_lens_point[i] = m_lens_point[i] + f[i] * oneminuscosangy;
				m_vertical[i] = m_vertical[i] + f[i] * sinangy/fl;
				m_vertical[i] = m_vertical[i] - uu[i] * oneminuscosangy;	
			}
			LimitCamera();
		}
		else if(event.MiddleIsDown())
		{
			double f[3];
			for(int i = 0; i<3; i++)f[i] = m_target_point[i] - m_lens_point[i];
			norm(f);
			double r[3];
			crossp(f, m_vertical, r);

			double d = dist(m_target_point, m_lens_point);

			double div_x = (double)(point_diff.x) * d * 0.001;
			double div_y = (double)(point_diff.y) * d * 0.001;
			for(int i = 0; i<3; i++)m_target_point[i] = m_target_point[i] - r[i] * div_x + m_vertical[i] * div_y;
			for(int i = 0; i<3; i++)m_lens_point[i] = m_lens_point[i] - r[i] * div_x + m_vertical[i] * div_y;

			LimitCamera();
		}

		Refresh(0);
		m_current_point = wxPoint(event.GetX(), event.GetY());
	}

	if(event.GetWheelRotation() != 0)
	{
		double wheel_value = (double)(event.GetWheelRotation());
		double multiplier = -wheel_value /1000.0;
		ViewScale(multiplier);
		Refresh(0);
	}

	event.Skip();
}

void get_canvas_camera(dpoint3d &ipos, dpoint3d &istr, dpoint3d &ihei, dpoint3d &ifor)
{
	//ipo: camera position
	//ist: camera's unit RIGHT vector
	//ihe: camera's unit DOWN vector
	//ifo: camera's unit FORWARD vector

	ipos.x = 1024 -wxGetApp().m_frame->m_solid_view->m_lens_point[0];
	ipos.y = wxGetApp().m_frame->m_solid_view->m_lens_point[1];
	ipos.z = 256.0 - wxGetApp().m_frame->m_solid_view->m_lens_point[2];

	double f[3];
	for(int i = 0; i<3; i++)f[i] = wxGetApp().m_frame->m_solid_view->m_target_point[i] - wxGetApp().m_frame->m_solid_view->m_lens_point[i];
	norm(f);

	double down[3];
	for(int i = 0; i<3; i++)down[i] = -wxGetApp().m_frame->m_solid_view->m_vertical[i];

	double right[3];
	crossp(down, f, right);

	istr.x = -right[0];
	istr.y = right[1];
	istr.z = -right[2];

	ihei.x = -down[0];
	ihei.y = down[1];
	ihei.z = -down[2];

	ifor.x = -f[0];
	ifor.y = f[1];
	ifor.z = -f[2];
}