toolpath.coords = Coords(-80, -40, -25, 80, 40, 10)
voxelcut.set_current_color(0x202020)
#toolpath.coords.add_block(0, 0, -8, 87, 64, 8.2)
toolpath.coords.add_block(0, 0, -21.5, 120, 45, 25.5)
voxelcut.set_current_color(0x404000)
toolpath.coords.add_block(-90, 0, 4, 70, 60, 5)

GRAY = 0x505050
RED = 0x600000
BLUE = 0x000050

toolpath.tools[1] = [[16, 55, GRAY], [40, 40, RED]]
#toolpath.tools[1] = [[3, 8, GRAY], [3, 7, RED], [40, 40, RED]]
#toolpath.tools[1] = [[5, 12, GRAY], [6, 13, RED], [40, 40, RED]]
#toolpath.tools[2] = [[2, 4, GRAY], [3, 8, BLUE], [15, 30, RED], [40, 40, RED]]
toolpath.tools[2] = [[3, 6, GRAY], [3, 8, BLUE], [15, 30, RED], [40, 40, RED]]
toolpath.tools[3] = [[2.5, 25, GRAY], [15, 30, RED], [40, 40, RED]]
#toolpath.tools[4] = [[6, 30, GRAY], [40, 40, RED]]
#toolpath.tools[3] = [[5, 30, GRAY], [40, 40, RED]]
toolpath.load('gr105.tap')
