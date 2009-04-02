// ControlButton.h

enum ControlButtonType
{
	ControlButtonTypeXMinus,
	ControlButtonTypeXPlus,
	ControlButtonTypeYMinus,
	ControlButtonTypeYPlus,
	ControlButtonTypeZMinus,
	ControlButtonTypeZPlus
};

class CControlButton:public wxBitmapButton
{
	ControlButtonType m_type;
public:
	CControlButton(wxWindow *parent, wxWindowID id, ControlButtonType type);

	static wxBitmap GetBitmap(ControlButtonType type, bool pushed);

    void OnMouse( wxMouseEvent& event );

	DECLARE_EVENT_TABLE()
};