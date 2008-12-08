# main script file run by VoxelCut

from vc import *

import wx
import math

voxels_per_mm = float(0)
x = float(0)
y = float(0)
z = float(0)
rad = float(0)

def jump_to(nx, ny, nz):
    global x
    global y
    global z
    x, y, z = nx, ny, nz

def get_voxel_pos(nx, ny, nz):
    global voxels_per_mm
    vx = int(512 - 300 + nx * voxels_per_mm+0.5)
    vy = int(512 - 300 + ny * voxels_per_mm+0.5)
    vz = int(256 + (-10 - nz) * voxels_per_mm+0.5)
    return vx, vy, vz
    
def subtract_tool_at(nx, ny, nz):
    global rad
    global voxels_per_mm
    vx, vy, vz = get_voxel_pos(nx, ny, nz)
    vrad = int(rad * voxels_per_mm)
    setcylinder(vx, vy, vz, vx, vy, 0, vrad, -1)

def cut_to(nx, ny, nz):
    global x
    global y
    global z

    vx = float(nx) - x
    vy = float(ny) - y
    vz = float(nz) - z

    magn = math.sqrt(vx*vx + vy*vy + vz*vz)
    voxmagn = magn*voxels_per_mm
    num_steps = int(voxmagn + 0.9)

    for i in range(0, num_steps):
        fraction = float(i+1)/num_steps
        tx = x + fraction * vx
        ty = y + fraction * vy
        tz = z + fraction * vz
        subtract_tool_at(tx, ty, tz)
        
    repaint()
    x, y, z = nx, ny, nz

def get_value(s, name):
    # gets a value from the string, like:  found, x = get_value('N1000G0X4.56Y34.78M6', 'X'), gives found = true, x = 4.56
    pos = s.find(name)
    if pos == -1: return False, 0.0

    numstr = ''

    for x in range(pos + 1, len(s)):
        if s[x].isdigit() or s[x] == '.' or s[x] == '-':
            numstr = numstr + s[x]
        else: break

    if len(numstr) == 0: return False, 0.0

    return True, float(numstr)

def read_nc_file(fpath):
    # set the tool position to z50mm
    jump_to(0, 0, 50)

    # set a tool diameter of 3mm
    global rad
    rad = 1.5

    # open the nc file
    f = open(fpath)

    global x
    global y
    global z
    nx = x
    ny = y
    nz = z
    
    while (True):
        line = f.readline()
        if (len(line) == 0) : break

        G0_found = (line.find('G0') != -1)
        G1_found = (line.find('G1') != -1)
        G2_found = (line.find('G2') != -1)
        G3_found = (line.find('G3') != -1)
        X_found, tx = get_value(line, 'X')
        Y_found, ty = get_value(line, 'Y')
        Z_found, tz = get_value(line, 'Z')
        move_found = False
        if X_found:
            nx = tx
            move_found = True
        if Y_found:
            ny = ty
            move_found = True
        if Z_found:
            nz = tz
            move_found = True

        if move_found:
            cut_to(nx, ny, nz)            

    f.close()

# clear the volume
setrect(0, 0, 0, 1024, 1024, 256, -1)
repaint()

# make a default block 50mm x 50mm x 10mm
global voxels_per_mm
vwidth = 600
mwidth = 50
voxels_per_mm = vwidth/mwidth
x0 = 512 - 300
y0 = 512 - 300
z0 = 256 - 10 * voxels_per_mm
x1 = 512 + 300
y1 = 512 + 300
z1 = 256
setrect(x0, y0, z0, x1, y1, z1, 0)
repaint()

# ask the user to choose an nc file
app = wx.App()
fd = wx.FileDialog(None, "Open NC File", "", "", "Tap Files (*.tap)|*.tap")
result = fd.ShowModal()

if result == wx.ID_OK:
    read_nc_file(fd.GetPath())

 

