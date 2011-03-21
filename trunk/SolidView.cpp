// SolidView.cpp
#include "SolidView.h"
#include "voxlap5.h"
#include "Tool.h"
#include <math.h>

extern void doframe ();
extern long xres, yres, colbits, fullscreen, maxpages;
extern long initdirectdraw (long daxres, long dayres, long dacolbits);
extern void uninitdirectdraw ();

CSolidView solid_view;

CSolidView::CSolidView()
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
	m_size = Point(0, 0);
	m_refresh_wanted = false;
}

void CSolidView::OnPaint()
{
		if(m_size.x > 0 && m_size.y > 0 && ((m_size.x != xres) || (m_size.y != yres)))
		{
			uninitdirectdraw();
			xres = m_size.x;
			yres = m_size.y;
			initdirectdraw(xres,yres, colbits);
			m_initdirectdraw_called = true;
		}
		if(m_initdirectdraw_called)
		{
			//setcylinder(&p0, &p1, cr, -1, 0);
			//updatevxl();
			doframe();
		}

		this->m_refresh_wanted = false;
}

void CSolidView::OnSize(int x, int y)
{
	if(x > 0)m_size.x = x;
	if(y > 0)m_size.y = y;
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

void CSolidView::OnMouse( MouseEvent& event )
{
	if(event.LeftDown() || event.MiddleDown() || event.RightDown())
	{
		m_button_down_point = Point(event.GetX(), event.GetY());
		m_current_point = m_button_down_point;
		//StoreViewPoint();
		m_initial_point = m_button_down_point;
	}
	else if(event.Dragging())
	{
		Point point_diff = Point(event.GetX(), event.GetY()) - m_current_point;

		if(event.LeftIsDown())
		{
			if(point_diff.x > 100)point_diff.x = 100;
			else if(point_diff.x < -100)point_diff.x = -100;
			if(point_diff.y > 100)point_diff.y = 100;
			else if(point_diff.y < -100)point_diff.y = -100;
			double c=(m_size.x+m_size.y)/20;

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

		Refresh();
		m_current_point = Point(event.GetX(), event.GetY());
	}

	if(event.GetWheelRotation() != 0)
	{
		double wheel_value = (double)(event.GetWheelRotation());
		double multiplier = -wheel_value /1000.0;
		ViewScale(multiplier);
		Refresh();
	}
}

void get_canvas_camera(dpoint3d &ipos, dpoint3d &istr, dpoint3d &ihei, dpoint3d &ifor)
{
	//ipo: camera position
	//ist: camera's unit RIGHT vector
	//ihe: camera's unit DOWN vector
	//ifo: camera's unit FORWARD vector

	ipos.x = 1024 -solid_view.m_lens_point[0];
	ipos.y = solid_view.m_lens_point[1];
	ipos.z = 256.0 - solid_view.m_lens_point[2];

	double f[3];
	for(int i = 0; i<3; i++)f[i] = solid_view.m_target_point[i] - solid_view.m_lens_point[i];
	norm(f);

	double down[3];
	for(int i = 0; i<3; i++)down[i] = -solid_view.m_vertical[i];

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