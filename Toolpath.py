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
        
x_for_cut = 0
y_for_cut = 0
z_for_cut = 0
        
class VoxelCyl:
    def __init__(self, radius, z, color):
        self.radius = int(radius)
        self.z_bottom = int(z)
        self.z_top = int(z) + 1
        self.color = color

    def cut(self):
        voxelcut.set_current_color(self.color)
        voxelcut.remove_cylinder(int(x_for_cut), int(y_for_cut), z_for_cut + int(self.z_bottom), int(x_for_cut), int(y_for_cut), z_for_cut + int(self.z_top), int(self.radius))
        
    def draw(self):

        for i in range(0, 21):
            a = 0.31415926 * i
            x = float(x_for_cut) + self.radius * math.cos(a)
            y = float(y_for_cut) + self.radius * math.sin(a)
            z_bottom = float(z_for_cut) + self.z_bottom
            z_top = float(z_for_cut) + self.z_top

            voxelcut.drawline3d(x, y, z_bottom, x, y, z_top, self.color)
            
            if i > 0:
                voxelcut.drawline3d(prevx, prevy, prevz_bottom, x, y, z_bottom, self.color)
                voxelcut.drawline3d(prevx, prevy, prevz_top, x, y, z_top, self.color)
            prevx = x
            prevy = y
            prevz_bottom = z_bottom
            prevz_top = z_top
        
class Tool:
    def __init__(self, span_list):
        # this is made from a list of (area.Span, colour_ref)
        # the spans should be defined with the y-axis representing the centre of the tool, with the tip of the tool being defined at y = 0
        self.span_list = span_list
        self.cylinders = []
        self.cylinders_calculated = False
        self.calculate_cylinders()
        
    def calculate_span_cylinders(self, span, color):
        sz = span.p.y * toolpath.coords.voxels_per_mm
        ez = span.v.p.y * toolpath.coords.voxels_per_mm
        
        z = sz
        while z < ez:
            # make a line at this z
            intersection_line = area.Span(area.Point(0, z), area.Vertex(0, area.Point(300, z), area.Point(0, 0)), False)
            intersections = span.Intersect(intersection_line)
            if len(intersections):
                radius = intersections[0].x * toolpath.coords.voxels_per_mm
                self.cylinders.append(VoxelCyl(radius, z * toolpath.coords.voxels_per_mm, color))
            z += 1/toolpath.coords.voxels_per_mm
            
    def refine_cylinders(self):
        cur_cylinder = None
        old_cylinders = self.cylinders
        self.cylinders = []
        for cylinder in old_cylinders:
            if cur_cylinder == None:
                cur_cylinder = cylinder
            else:
                if cur_cylinder.radius == cylinder.radius:
                    cur_cylinder.z_top = cylinder.z_top
                else:
                    self.cylinders.append(cur_cylinder)
                    cur_cylinder = cylinder
        if cur_cylinder != None:
            self.cylinders.append(cur_cylinder)                    
        
    def calculate_cylinders(self):
        self.cylinders = []
        for span_and_color in self.span_list:
            self.calculate_span_cylinders(span_and_color[0], span_and_color[1])
            
        self.refine_cylinders()
                                  
        self.cylinders_calculated = True
            
    def cut(self, x, y, z):
        global x_for_cut
        global y_for_cut
        global z_for_cut
        x_for_cut = x
        y_for_cut = y
        z_for_cut = z
        for cylinder in self.cylinders:
            cylinder.cut()
            
    def draw(self, x, y, z):
        global x_for_cut
        global y_for_cut
        global z_for_cut
        x_for_cut = x
        y_for_cut = y
        z_for_cut = z
        for cylinder in self.cylinders:
            cylinder.draw()

class Toolpath:
    def __init__(self):
        self.length = 0.0
        self.lines = []
        self.current_pos = 0.0
        self.current_point = Point(0, 0, 0)
        self.current_line_index = 0
        self.tools = {} # dictionary, tool id to Tool object
        self.current_tool = 1
        self.rapid = True
        self.mm_per_sec = 50.0
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
                        self.add_line(Point(curx, cury, curz ), Point(curx, cury, 30.0))
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
               
        file.close()
        
        self.rewind()
        
    def rewind(self):
        self.current_point = Point(0, 0, 0)
        if len(self.lines)>0:
            self.current_point = self.lines[0].p0
        self.current_pos = 0.0
        self.current_line_index = 0
        self.running = False
        
    def draw_tool(self):
        voxelcut.drawclear()
        
        index = self.current_line_index - 1
        if index < 0: index = 0
        tool_number = self.lines[index].tool_number
        
        if tool_number in self.tools:
            x, y, z = self.coords.mm_to_voxels(self.current_point.x, self.current_point.y, self.current_point.z)
            self.tools[tool_number].draw(x, y, z)
        
    def cut_point(self, p):
        x, y, z = self.coords.mm_to_voxels(p.x, p.y, p.z)
        index = self.current_line_index - 1
        if index < 0: index = 0
        tool_number = self.lines[index].tool_number
        
        if tool_number in self.tools:
            self.tools[tool_number].cut(x, y, z)
         
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
    
