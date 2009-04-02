// ControlButton.cpp

#include "wx/wx.h"
#include "ControlButton.h"
#include "VCApp.h"
#include "VCFrame.h"
#include "Tool.h"

#include "bmArrowXMinusBlue.xpm"
#include "bmArrowXMinusGreen.xpm"
#include "bmArrowXPlusBlue.xpm"
#include "bmArrowXPlusGreen.xpm"
#include "bmArrowYMinusBlue.xpm"
#include "bmArrowYMinusGreen.xpm"
#include "bmArrowYPlusBlue.xpm"
#include "bmArrowYPlusGreen.xpm"
#include "bmArrowZMinusBlue.xpm"
#include "bmArrowZMinusGreen.xpm"
#include "bmArrowZPlusBlue.xpm"
#include "bmArrowZPlusGreen.xpm"

BEGIN_EVENT_TABLE(CControlButton, wxBitmapButton)
    EVT_MOUSE_EVENTS(CControlButton::OnMouse)
END_EVENT_TABLE()

CControlButton::CControlButton(wxWindow *parent, wxWindowID id, ControlButtonType type):wxBitmapButton(parent, id, GetBitmap(type, false)),m_type(type)
{
	SetBitmapSelected(GetBitmap(type, true));
}

//static
wxBitmap CControlButton::GetBitmap(ControlButtonType type, bool pushed)
{
	char **str;

	switch(type)
	{
		case ControlButtonTypeXMinus:
			str = pushed ? bmArrowXMinusGreen_xpm:bmArrowXMinusBlue_xpm;
			break;

		case ControlButtonTypeXPlus:
			str = pushed ? bmArrowXPlusGreen_xpm:bmArrowXPlusBlue_xpm;
			break;

		case ControlButtonTypeYMinus:
			str = pushed ? bmArrowYMinusGreen_xpm:bmArrowYMinusBlue_xpm;
			break;

		case ControlButtonTypeYPlus:
			str = pushed ? bmArrowYPlusGreen_xpm:bmArrowYPlusBlue_xpm;
			break;

		case ControlButtonTypeZMinus:
			str = pushed ? bmArrowZMinusGreen_xpm:bmArrowZMinusBlue_xpm;
			break;

		case ControlButtonTypeZPlus:
			str = pushed ? bmArrowZPlusGreen_xpm:bmArrowZPlusBlue_xpm;
			break;

	}

	return wxBitmap(str);
}

void CControlButton::OnMouse( wxMouseEvent& event )
{
	if(event.LeftDown())
	{
		switch(m_type)
		{
		case ControlButtonTypeXMinus:
			wxGetApp().m_xminus_pressed = true;
			break;

		case ControlButtonTypeXPlus:
			wxGetApp().m_xplus_pressed = true;
			break;

		case ControlButtonTypeYMinus:
			wxGetApp().m_yminus_pressed = true;
			break;

		case ControlButtonTypeYPlus:
			wxGetApp().m_yplus_pressed = true;
			break;

		case ControlButtonTypeZMinus:
			wxGetApp().m_zminus_pressed = true;
			break;

		case ControlButtonTypeZPlus:
			wxGetApp().m_zplus_pressed = true;
			break;
		}

		wxGetApp().m_current_tool->m_cutting = true;
		wxGetApp().m_frame->m_animTimer.SetOwner( wxGetApp().m_frame, wxID_ANY );
		wxGetApp().m_frame->m_animTimer.Start( 10, wxTIMER_CONTINUOUS );
	}

	if(event.LeftUp())
	{
		switch(m_type)
		{
		case ControlButtonTypeXMinus:
			wxGetApp().m_xminus_pressed = false;
			break;

		case ControlButtonTypeXPlus:
			wxGetApp().m_xplus_pressed = false;
			break;

		case ControlButtonTypeYMinus:
			wxGetApp().m_yminus_pressed = false;
			break;

		case ControlButtonTypeYPlus:
			wxGetApp().m_yplus_pressed = false;
			break;

		case ControlButtonTypeZMinus:
			wxGetApp().m_zminus_pressed = false;
			break;

		case ControlButtonTypeZPlus:
			wxGetApp().m_zplus_pressed = false;
			break;
		}

		wxGetApp().m_frame->m_animTimer.Stop();
		wxGetApp().m_current_tool->m_cutting = false;
	}

	event.Skip();
}
