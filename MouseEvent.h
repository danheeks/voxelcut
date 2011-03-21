#pragma once

class MouseEvent
{
public:
	int m_event_type;
    int m_x, m_y;

    bool          m_leftDown;
    bool          m_middleDown;
    bool          m_rightDown;

    bool          m_controlDown;
    bool          m_shiftDown;
    bool          m_altDown;
    bool          m_metaDown;

    int           m_wheelRotation;
    int           m_wheelDelta;
    int           m_linesPerAction;

    bool LeftDown() const { return (m_event_type == 1); }
    bool MiddleDown() const { return (m_event_type == 6); }
    bool RightDown() const { return (m_event_type == 4); }
    bool LeftUp() const { return (m_event_type == 2); }
    bool MiddleUp() const { return (m_event_type == 7); }
    bool RightUp() const { return (m_event_type == 5); }
    bool LeftDClick() const { return (m_event_type == 3); }
    bool LeftIsDown() const { return m_leftDown; }
    bool MiddleIsDown() const { return m_middleDown; }
    bool RightIsDown() const { return m_rightDown; }
    bool Dragging() const{ return (m_event_type == 8) && (m_leftDown || m_middleDown || m_rightDown);}
    bool Moving() const{ return (m_event_type == 8) && !(m_leftDown || m_middleDown || m_rightDown);}
    int GetX() const { return m_x; }
    int GetY() const { return m_y; }
    int GetWheelRotation() const { return m_wheelRotation; }
    int GetWheelDelta() const { return m_wheelDelta; }
    int GetLinesPerAction() const { return m_linesPerAction; }
};
