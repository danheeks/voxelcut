import wx
import voxelcut
from Mouse import MouseEventFromWx
from Toolpath import toolpath

class myGLCanvas(wx.Window):
   def __init__(self, parent):
      wx.Window.__init__(self, parent,-1)
      self.Bind(wx.EVT_PAINT, self.OnPaint)
      self.Bind(wx.EVT_SIZE, self.OnSize)
      self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)
      self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
      self.Resize()
        
   def OnSize(self, event):
      self.Resize()
      event.Skip()
        
   def OnMouse(self, event):
      e = MouseEventFromWx(event)
      voxelcut.OnMouse(e)
      if voxelcut.refresh_wanted() == True:
          self.Refresh()
      event.Skip()
        
   def OnEraseBackground(self, event):
      pass # Do nothing, to avoid flashing on MSW
        
   def Resize(self):
      s = self.GetClientSize()
      voxelcut.OnSize(s.GetWidth(), s.GetHeight())
      self.Refresh()

   def OnPaint(self, event):
      dc = wx.PaintDC(self)
      toolpath.draw_tool()
      voxelcut.OnPaint()
