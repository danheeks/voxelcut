import voxelcut
import wx
import wx.aui
import sys
from GraphicsCanvas import myGLCanvas
from ControlView import ControlView
from Toolpath import toolpath
from Toolpath import Toolpath
from coords import Coords
import tempfile
from area import *
from Toolpath import Tool

def OnTimer(event):
    if toolpath.running:
        new_pos = toolpath.current_pos + toolpath.mm_per_sec/30
        toolpath.cut_to_position(new_pos)
        graphics_view.Refresh()
        
class VFrame(wx.Frame):
    def __init__(self):
        wx.Frame.__init__(self, None, -1, 'Solid Removal Simulation', size = wx.Size(450, 400))
        self.Bind(wx.EVT_CLOSE, self.OnExit)
        
    def OnExit(self, event):
        timer.Stop()
        app.ExitMainLoop()

def DoExecFile():
    execfile(tempfile.gettempdir()+'/initial.py')
    #execfile('initial.py')

def OnReset():
    voxelcut.Init(graphics_view.GetHandle())
    toolpath = Toolpath()
    graphics_view.Refresh()
    DoExecFile()

def OnViewReset():
    voxelcut.ViewReset()
    graphics_view.Refresh()

save_out = sys.stdout
save_err = sys.stderr

app = wx.App()

sys.stdout = save_out
sys.stderr = save_err

# make a wxWidgets application
frame= VFrame()

aui_manager = wx.aui.AuiManager()
aui_manager.SetManagedWindow(frame)

graphics_view = myGLCanvas(frame)
voxelcut.Init(graphics_view.GetHandle())

p = wx.aui.AuiPaneInfo()

aui_manager.AddPane(graphics_view, wx.aui.AuiPaneInfo().Name('graphics').Center().CaptionVisible(False))

control_view = ControlView(frame, OnReset, OnViewReset)
aui_manager.AddPane(control_view, wx.aui.AuiPaneInfo().Name('Controls').Bottom().CaptionVisible(False).Fixed())

timer = wx.Timer(frame, wx.ID_ANY)
timer.Start(33)
frame.Bind(wx.EVT_TIMER, OnTimer)

frame.Center()
aui_manager.Update()
frame.Show()

DoExecFile()

app.MainLoop()
    