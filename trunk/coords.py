import math
import voxelcut

class Coords:
    def __init__(self, xmin, ymin, zmin, xmax, ymax, zmax):
        xw = math.fabs(xmax - xmin)
        yw = math.fabs(ymax - ymin)
        zw = math.fabs(zmax - zmin)
        xr = xw/1024
        yr = yw/1024
        zr = zw/256
        
        maxr = xr
        if yr > maxr: maxr = yr
        if zr > maxr: maxr = zr
        
        if maxr < 0.00000000001:
            self.voxels_per_mm = 0.0
            return
        
        self.voxels_per_mm = 1/maxr
        
        self.cx = (xmin + xmax)/2
        self.cy = (ymin + ymax)/2        
        self.cz = zmin
        
    def mm_to_voxels(self, x, y, z):
        if self.voxels_per_mm == 0.0:
            return 0, 0, 0
        vx = 512 + (x - self.cx)*self.voxels_per_mm
        vy = 512 + (y - self.cy)*self.voxels_per_mm
        vz = (z - self.cz)*self.voxels_per_mm
        return int(vx), int(vy), int(vz)
    
    def add_block(self, x, y, z, xw, yw, zw):
        x = x - (xw/2)
        y = y - (yw/2)
        xw = xw * self.voxels_per_mm
        yw = yw * self.voxels_per_mm
        zw = zw * self.voxels_per_mm
        x, y, z = self.mm_to_voxels(x, y, z)
        voxelcut.add_block(x, y, z, int(xw), int(yw), int(zw))
