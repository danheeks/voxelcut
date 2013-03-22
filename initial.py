toolpath.coords = Coords(-100, -100, -5, 100, 100, 5)
voxelcut.set_current_color(12566512)
toolpath.coords.add_block(0.150768, 0, -5, 9.69846, 9.84808, 10)
GRAY = 0x505050
RED = 0x600000
BLUE = 0x000050
toolpath.tools[2] = [[Span(Point(3, 0), Vertex(0, Point(3, 20), Point(0, 0)), False), GRAY], [Span(Point(20, 20), Vertex(0, Point(20, 40), Point(0, 0)), False), RED]]
#toolpath.load('C:/Users/Dan/AppData/Local/Temp/test.tap')
toolpath.load('test.tap')
