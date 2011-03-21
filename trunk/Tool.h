// Tool.h

class CTool
{
public:
	double m_r; // radius
	double m_p0[3];
	double m_p1[3];
	bool m_cutting;

	CTool(){
		m_r = 5;
		m_p0[0] = 0.0;
		m_p0[1] = 0.0;
		m_p0[2] = 0.0;
		m_p1[0] = 0.0;
		m_p1[1] = 0.0;
		m_p1[2] = 0.0;
	}
};

