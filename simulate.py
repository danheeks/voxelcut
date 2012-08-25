import voxelcut
import wx
import wx.aui
import sys
from GraphicsCanvas import myGLCanvas
from ControlView import ControlView
from Toolpath import toolpath
from coords import Coords
import tempfile

def OnTimer(event):
    if toolpath.running:
        new_pos = toolpath.current_pos + toolpath.mm_per_sec/30
        toolpath.cut_to_position(new_pos)
        graphics_view.Refresh()

save_out = sys.stdout
save_err = sys.stderr

app = wx.App()

sys.stdout = save_out
sys.stderr = save_err

# make a wxWidgets application
frame= wx.Frame(None, -1, 'Solid Removal Simulation', size = wx.Size(400, 400))

aui_manager = wx.aui.AuiManager()
aui_manager.SetManagedWindow(frame)

graphics_view = myGLCanvas(frame)
voxelcut.Init(graphics_view.GetHandle())

p = wx.aui.AuiPaneInfo()

aui_manager.AddPane(graphics_view, wx.aui.AuiPaneInfo().Name('graphics').Center().CaptionVisible(False))

control_view = ControlView(frame)
aui_manager.AddPane(control_view, wx.aui.AuiPaneInfo().Name('Controls').Bottom().CaptionVisible(False).Fixed())

timer = wx.Timer(frame, wx.ID_ANY)
timer.Start(33)
frame.Bind(wx.EVT_TIMER, OnTimer)


frame.Center()
aui_manager.Update()
frame.Show()

execfile(tempfile.gettempdir()+'/initial.py')

app.MainLoop()
