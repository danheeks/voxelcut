// ControlView.cpp
#include "wx/wxprec.h"
#include "ControlView.h"
#include "ControlButton.h"

BEGIN_EVENT_TABLE(CControlView, wxWindow)
END_EVENT_TABLE()

enum{
	ID_BUTTON1 = 1000,
	ID_BUTTON2,
	ID_BUTTON_XMINUS,
	ID_BUTTON_XPLUS,
	ID_BUTTON_YMINUS,
	ID_BUTTON_YPLUS,
	ID_BUTTON_ZMINUS,
	ID_BUTTON_ZPLUS,
};

CControlView::CControlView(wxWindow *parent):
 wxWindow(parent, wxID_ANY)
{
	wxPanel* panel = new wxPanel(this, wxID_ANY);

    // create buttons
    wxButton *button1 = new wxButton(panel, ID_BUTTON1, "test1");
    wxButton *button2 = new wxButton(panel, ID_BUTTON2, "test2");

    wxBoxSizer *mainsizer = new wxBoxSizer( wxVERTICAL );

    wxFlexGridSizer *gridsizer = new wxFlexGridSizer(3, 4, 5, 5);

	gridsizer->Add(new wxStaticText(panel, wxID_ANY, _("")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new CControlButton(panel, ID_BUTTON_YPLUS, ControlButtonTypeYPlus), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new wxStaticText(panel, wxID_ANY, _("")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new CControlButton(panel, ID_BUTTON_ZPLUS, ControlButtonTypeZPlus), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new CControlButton(panel, ID_BUTTON_XMINUS, ControlButtonTypeXMinus), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new wxStaticText(panel, wxID_ANY, _("")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new CControlButton(panel, ID_BUTTON_XPLUS, ControlButtonTypeXPlus), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new wxStaticText(panel, wxID_ANY, _("")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new wxStaticText(panel, wxID_ANY, _("")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new CControlButton(panel, ID_BUTTON_YMINUS, ControlButtonTypeYMinus), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new wxStaticText(panel, wxID_ANY, _("")), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));
	gridsizer->Add(new CControlButton(panel, ID_BUTTON_ZMINUS, ControlButtonTypeZMinus), wxSizerFlags().Align(wxALIGN_CENTER_VERTICAL));

    wxBoxSizer *bottomsizer = new wxBoxSizer( wxHORIZONTAL );

    bottomsizer->Add( button1, 0, wxALL, 10 );
    bottomsizer->Add( button2, 0, wxALL, 10 );

    mainsizer->Add( gridsizer, wxSizerFlags().Align(wxALIGN_CENTER).Border(wxALL, 10).Expand() );
    mainsizer->Add( bottomsizer, wxSizerFlags().Align(wxALIGN_CENTER) );

    // tell frame to make use of sizer (or constraints, if any)
    panel->SetAutoLayout( true );
    panel->SetSizer( mainsizer );

#ifndef __WXWINCE__
    mainsizer->SetSizeHints( this );
#endif

	mainsizer->Layout();
  mainsizer->SetSizeHints( panel );   // set size hints to honour minimum size

    Show(true);
 }
