// VCApp.h
#include "wx/wx.h"

class VCFrame;
class CTool;

// The VoxelCut application
class VCApp: public wxApp
{
  public:
	  VCFrame* m_frame;

	  bool m_xminus_pressed;
	  bool m_xplus_pressed;
	  bool m_yminus_pressed;
	  bool m_yplus_pressed;
	  bool m_zminus_pressed;
	  bool m_zplus_pressed;

	  CTool* m_current_tool;

    VCApp(void);
	~VCApp();
	virtual bool OnInit(void);
    int OnExit();
};

DECLARE_APP(VCApp)
