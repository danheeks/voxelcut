// PythonStuff.cpp
#include "PythonStuff.h"
#include "voxlap5.h"
#include "MouseEvent.h"
#include "SolidView.h"

#if _DEBUG
#undef _DEBUG
#include <python.h>
#define _DEBUG
#else
#include <python.h>
#endif

#include "windows.h"

#include <list>

#include <boost/progress.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/wrapper.hpp>
#include <boost/python/call.hpp>

extern long initapp (long argc, char **argv);
extern HWND ghwnd;

namespace bp = boost::python;

static void add_sphere(int x, int y, int z, int r)
{
	lpoint3d p;
	p.x = 1024-y;
	p.y = 1024-x;
	p.z = 256 - z;
    setsphere(&p, r, 0);
	updatevxl();
}

static void remove_sphere(int x, int y, int z, int r)
{
	lpoint3d p;
	p.x = 1024-y;
	p.y = 1024-x;
	p.z = 256 - z;
    setsphere(&p, r, -1);
	updatevxl();
}

static void add_block(int x, int y, int z, int xw, int yw, int zw)
{
	lpoint3d p1;
	p1.x = 1024-y;
	p1.y = 1024-x;
	p1.z = 256 - z;
	lpoint3d p2;
	p2.x = 1024-(y + yw);
	p2.y = 1024-(x + xw);
	p2.z = 256 - (z + zw);
	setrect(&p1, &p2, 0);
	updatevxl();
}

static void remove_block(int x, int y, int z, int xw, int yw, int zw)
{
	lpoint3d p1;
	p1.x = 1024-y;
	p1.y = 1024-x;
	p1.z = 256 - z;
	lpoint3d p2;
	p2.x = 1024-(y + yw);
	p2.y = 1024-(x + xw);
	p2.z = 256 - (z + zw);
	setrect(&p1, &p2, -1);
	updatevxl();
}

static void remove_line(int x0, int y0, int z0, int x1, int y1, int z1, int thickness)
{
	point3d points[3];
	points[0].x = 512; points[0].y = 512; points[0].z = 20;
	points[1].x = 600; points[1].y = 512; points[1].z = 20;
	points[2].x = 512; points[2].y = 600; points[2].z = 20;

	long p[3] = {1, 2, 0};
	setsector (points, p, 1, 4, 0, 0);
	updatevxl();
}

static void add_cylinder(int x, int y, int z, int x2, int y2, int z2, int r)
{
	lpoint3d p1;
	p1.x = 1024-y;
	p1.y = 1024-x;
	p1.z = 256 - z;
	lpoint3d p2;
	p2.x = 1024-y2;
	p2.y = 1024-x2;
	p2.z = 256 - z2;
	setcylinder(&p1, &p2, r, 0, 0);
	updatevxl();
}

static void remove_cylinder(int x, int y, int z, int x2, int y2, int z2, int r)
{
	lpoint3d p1;
	p1.x = 1024-y;
	p1.y = 1024-x;
	p1.z = 256 - z;
	lpoint3d p2;
	p2.x = 1024-y2;
	p2.y = 1024-x2;
	p2.z = 256 - z2;
	setcylinder(&p1, &p2, r, -1, 0);
	//setellipsoid(&p1, &p2, r, -1, 0);
	updatevxl();
}

static void remove_cone(int x, int y, int z, int x2, int y2, int z2, int r)
{
	lpoint3d p1;
	p1.x = 1024-y;
	p1.y = 1024-x;
	p1.z = 256 - z;
	lpoint3d p2;
	p2.x = 1024-y2;
	p2.y = 1024-x2;
	p2.z = 256 - z2;
	setellipsoid(&p1, &p2, r, -1, 0);
	updatevxl();
}

static void set_current_color(int col)
{
	vx5.curcol = col;
}

static void OnPaint()
{
	solid_view.OnPaint();
}

static void OnSize(int x, int y)
{
	solid_view.OnSize(x, y);
}

static void OnMouse(MouseEvent &e)
{
	solid_view.OnMouse(e);
}

static void ViewReset()
{
	solid_view.ViewReset();
}

static void Init(int hwnd)
{
	ghwnd = (HWND)hwnd;
	initapp(0,NULL);
}

class PPoint
{
public:
	float x, y, z;
	long col;
	PPoint(float X, float Y, float Z, long Col){x = X; y = Y; z = Z; col = Col;}
};

class PLine
{
public:
	float x0, y0, z0, x1, y1, z1;
	long col;
	PLine(float X0, float Y0, float Z0, float X1, float Y1, float Z1, long Col){x0 = X0; y0 = Y0; z0 = Z0; x1 = X1; y1 = Y1; z1 = Z1; col = Col;}
};

static std::list<PLine> lines;
static std::list<PPoint> points;

void draw_callback()
{
	for(std::list<PLine>::iterator It = lines.begin(); It != lines.end(); It++)
	{
		PLine &line = *It;
		drawline3d(line.x0, line.y0, line.z0, line.x1, line.y1, line.z1, line.col);
	}

	for(std::list<PPoint>::iterator It = points.begin(); It != points.end(); It++)
	{
		PPoint &point = *It;
		drawpoint3d(point.x, point.y, point.z, point.col);
	}
}

static void Pdrawpoint3d(float x, float y, float z, long col)
{
	points.push_back(PPoint(1024-y, 1024-x, 256-z, col));
}

static void Pdrawline3d(float x0, float y0, float z0, float x1, float y1, float z1, long col)
{
	lines.push_back(PLine(1024-y0, 1024-x0, 256-z0, 1024-y1, 1024-x1, 256-z1, col));
}

static void Pdrawclear()
{
	points.clear();
	lines.clear();
}

static bool refresh_wanted()
{
	return solid_view.m_refresh_wanted;
}


BOOST_PYTHON_MODULE(voxelcut)
{
	bp::class_<MouseEvent>("MouseEvent") 
        .def(bp::init<MouseEvent>())
        .def_readwrite("m_event_type", &MouseEvent::m_event_type)
        .def_readwrite("m_x", &MouseEvent::m_x)
        .def_readwrite("m_y", &MouseEvent::m_y)
        .def_readwrite("m_leftDown", &MouseEvent::m_leftDown)
        .def_readwrite("m_middleDown", &MouseEvent::m_middleDown)
        .def_readwrite("m_rightDown", &MouseEvent::m_rightDown)
        .def_readwrite("m_controlDown", &MouseEvent::m_controlDown)
        .def_readwrite("m_shiftDown", &MouseEvent::m_shiftDown)
        .def_readwrite("m_altDown", &MouseEvent::m_altDown)
        .def_readwrite("m_metaDown", &MouseEvent::m_metaDown)
        .def_readwrite("m_wheelRotation", &MouseEvent::m_wheelRotation)
        .def_readwrite("m_wheelDelta", &MouseEvent::m_wheelDelta)
        .def_readwrite("m_linesPerAction", &MouseEvent::m_linesPerAction)
    ;

	bp::def("add_sphere", add_sphere);
	bp::def("remove_sphere", remove_sphere);
	bp::def("add_block", add_block);
	bp::def("remove_block", remove_block);
	bp::def("remove_line", remove_line);
	bp::def("add_cylinder", add_cylinder);
	bp::def("remove_cylinder", remove_cylinder);
	bp::def("remove_cone", remove_cone);
	bp::def("set_current_color", set_current_color);
	bp::def("drawpoint3d", Pdrawpoint3d);
	bp::def("drawline3d", Pdrawline3d);
	bp::def("drawclear", Pdrawclear);
	bp::def("OnPaint", OnPaint);
	bp::def("OnSize", OnSize);
	bp::def("OnMouse", OnMouse);
	bp::def("Init", Init);
	bp::def("refresh_wanted", refresh_wanted);
	bp::def("ViewReset", ViewReset);
}
