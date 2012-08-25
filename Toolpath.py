import area
import math
import voxelcut
import re
import copy
from coords import Coords

class Point:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z
        
    def dist(self, p):
        dx = math.fabs(p.x - self.x)
        dy = math.fabs(p.y - self.y)
        dz = math.fabs(p.z - self.z)
        return math.sqrt(dx*dx + dy*dy + dz*dz)
    
    def __mul__(self, value):
        return Point(self.x * value, self.y * value, self.z * value)
    
    def __add__(self, p):
        return Point(self.x + p.x, self.y + p.y, self.z + p.z)
    
    def __sub__(self, p):
        return Point(self.x - p.x, self.y - p.y, self.z - p.z)

class Line:
    def __init__(self, p0, p1, rapid, tool_number):
        self.p0 = p0
        self.p1 = p1
        self.rapid = rapid
        self.tool_number = tool_number
        
    def Length(self):
        return self.p0.dist(self.p1)

class Toolpath:
    def __init__(self):
        self.length = 0.0
        self.points = []
        self.current_pos = 0.0
        self.current_point = Point(0, 0, 0)
        self.current_line_index = 0
        self.tools = {} # dictionary, tool id to diameter
        self.current_tool = 1
        self.rapid = True
        self.mm_per_sec = 500.0
        self.running = False
        self.coords = Coords(0, 0, 0, 0, 0, 0)
        self.in_cut_to_position = False
        
    def add_line(self, p0, p1):
        self.lines.append(Line(p0, p1, self.rapid, self.current_tool))
        
    def load(self, nc_filepath):
        # this converts the G1s in an NC file into arcs with G2 or G3
        pattern_main = re.compile('([(!;].*|\s+|[a-zA-Z0-9_:](?:[+-])?\d*(?:\.\d*)?|\w\#\d+|\(.*?\)|\#\d+\=(?:[+-])?\d*(?:\.\d*)?)')

        self.lines = []
        self.length = 0.0
        file = open(nc_filepath, 'r')
        arc = 0
        self.rapid = False
        curx = None
        cury = None
        curz = None
        
        while(True):
            line = file.readline().rstrip()
            if len(line)== 0: break
            
            move = False
            
            x = None
            y = None
            z = None
            i = None
            j = None
            
            words = pattern_main.findall(line)
            for word in words:
                word = word.upper()
                if word == 'G1' or word == 'G01':
                    self.rapid = False
                    arc = 0
                elif word == 'G2' or word == 'G02':
                    self.rapid = False
                    arc = -1
                elif word == 'G3' or word == 'G03':
                    self.rapid = False
                    arc = 1
                elif word == 'G0' or word == 'G00':
                    self.rapid = True
                    arc = 0
                elif word[0] == 'X':
                    x = eval(word[1:])
                    move = True
                elif word[0] == 'Y':
                    y = eval(word[1:])
                    move = True
                elif word[0] == 'Z':
                    z = eval(word[1:])
                    move = True
                elif word[0] == 'I':
                    i = float(eval(word[1:]))
                elif word[0] == 'J':
                    j = float(eval(word[1:]))
                elif word[0] == 'T':
                    self.current_tool = eval(word[1:])
                    if (curx != None) and (cury != None) and (curz != None):
                        self.add_line(Point(curx, cury, curz), Point(curx, cury, 30.0))
                        curz = 30.0
                elif word[0] == ';' : break

            if move:
                if (curx != None) and (cury != None) and (curz != None):
                    newx = curx
                    newy = cury
                    newz = curz
                    if x != None: newx = float(x)
                    if y != None: newy = float(y)
                    if z != None: newz = float(z)
                    if arc != 0:
                        area.set_units(0.05)
                        curve = area.Curve()
                        curve.append(area.Point(curx, cury))
                        # next 4 lines were for Bridgeport.
                        # this only works for LinuxCNC now
                        #if (newx > curx) != (arc > 0):
                        #    j = -j
                        #if (newy > cury) != (arc < 0):
                        #    i = -i
                        curve.append(area.Vertex(arc, area.Point(newx, newy), area.Point(curx+i, cury+j)))
                        curve.UnFitArcs()
                        for span in curve.GetSpans():
                            self.add_line(Point(span.p.x, span.p.y, newz), Point(span.v.p.x, span.v.p.y, newz))
                    else:
                        self.add_line(Point(curx, cury, curz), Point(newx, newy, newz))
                                    
                if x != None: curx = float(x)
                if y != None: cury = float(y)
                if z != None: curz = float(z)

        for line in self.lines:
            self.length += line.Length()
            
        if len(self.lines)>0:
            self.current_point = self.lines[0].p0
               
        file.close()
        
    def draw_cylinder(self, p, radius, h0, h1, col):
        vh0 = h0*self.coords.voxels_per_mm
        vh1 = h1*self.coords.voxels_per_mm
        for i in range(0, 21):
            a = 0.31415926 * i
            x = self.current_point.x + radius * math.cos(a)
            y = self.current_point.y + radius * math.sin(a)
            z = self.current_point.z
            
            x, y, z = self.coords.mm_to_voxels(x, y, z)
            voxelcut.drawline3d(x, y, z + vh0, x, y, z + vh1, col)
            
            if i > 0:
                voxelcut.drawline3d(prevx, prevy, prevz + vh0, x, y, z + vh0, col)
                voxelcut.drawline3d(prevx, prevy, prevz+vh1, x, y, z+vh1, col)
            prevx = x
            prevy = y
            prevz = z
        
    def draw_tool(self):
        voxelcut.drawclear()
        
        index = self.current_line_index - 1
        if index < 0: index = 0
        tool_number = self.lines[index].tool_number
        
        h = 0
        
        if tool_number in self.tools:
            for cyl in self.tools[tool_number]:
                diameter = cyl[0]
                height = cyl[1]
                color = cyl[2]
                h1 = h + height
                self.draw_cylinder(self.current_point, diameter / 2, h, h1, color)
                h = h1
        
    def cut_point(self, p):
        x, y, z = self.coords.mm_to_voxels(p.x, p.y, p.z)
        index = self.current_line_index - 1
        if index < 0: index = 0
        tool_number = self.lines[index].tool_number
        
        h = 0
        
        if tool_number in self.tools:
            for cyl in self.tools[tool_number]:
                diameter = float(cyl[0])
                height = float(cyl[1])
                color = cyl[2]
                h1 = h + height
                vh0 = int(h*self.coords.voxels_per_mm)
                vh1 = int(h1*self.coords.voxels_per_mm)
                voxelcut.set_current_color(color)
                voxelcut.remove_cylinder(x, y, z + vh0, x, y, z + vh1, int(diameter / 2 * self.coords.voxels_per_mm))
                h = h1
         
    def cut_line(self, line):
#        self.cut_point(line.p0)
#        self.cut_point(line.p1)
#        voxelcut.remove_line(int(line.p0.x), int(line.p0.y), int(line.p0.z), int(line.p1.x), int(line.p1.y), int(line.p1.z), 5)
        
        length = line.Length()
        num_segments = int(1 + length * self.coords.voxels_per_mm * 0.06)
        step = length/num_segments
        dv = (line.p1 - line.p0) * (1.0/num_segments)
        for i in range (0, num_segments + 1):
            p = line.p0 + (dv * i)
            self.cut_point(p)
            
    def cut_to_position(self, pos):
        if self.current_line_index >= len(self.lines):
            return
        
        if self.cut_to_position == True:
            import wx
            wx.MessageBox("in cut_to_position again!")
        
        self.in_cut_to_position = True
        start_pos = self.current_pos
        while self.current_line_index < len(self.lines):
            line = copy.copy(self.lines[self.current_line_index])
            line.p0 = self.current_point
            line_length = line.Length()
            if line_length > 0:
                end_pos = self.current_pos + line_length
                if pos < end_pos:
                    fraction = (pos - self.current_pos)/(end_pos - self.current_pos)
                    line.p1 = line.p0 + ((line.p1 - line.p0) * fraction)
                    self.cut_line(line)
                    self.current_pos = pos
                    self.current_point = line.p1
                    break
                self.cut_line(line)
                self.current_pos = end_pos
            self.current_point = line.p1
            self.current_line_index = self.current_line_index + 1
            
        self.in_cut_to_position = False
    
toolpath = Toolpath()
    
