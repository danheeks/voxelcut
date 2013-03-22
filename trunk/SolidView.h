// SolidView.h

#include "MouseEvent.h"

class Point{
public:
	int x, y;

	Point(){x = -1; y = -1;}
	Point(int X, int Y){x = X; y = Y;}
	Point operator=(const Point& p){x = p.x; y = p.y; return *this;}
	Point operator+(const Point& p){return Point(x + p.x, y + p.y);}
	Point operator-(const Point& p){return Point(x - p.x, y - p.y);}
};

class CSolidView
{
	Point m_button_down_point;
	Point m_current_point;
	Point m_initial_point;
	Point m_size;

public:
	double m_lens_point[3];
	double m_target_point[3];
	double m_vertical[3];
	bool m_initdirectdraw_called;
	bool m_refresh_wanted;

	CSolidView();
	~CSolidView(void){};

	void ViewScale(double fraction);
	void LimitCamera();

	void OnPaint();
	void OnSize(int x, int y);
	void OnMouse( MouseEvent& event );
	void Refresh(){ m_refresh_wanted = true; }
	void ViewReset();
};

extern CSolidView solid_view;
