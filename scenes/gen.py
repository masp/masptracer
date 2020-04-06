import random
import math
import colorsys
pi = math.pi

cylinder_center = (0, 0, 0)
cylinder_radius = 10
cylinder_length = 50
no_subcylinders = 32


def points_in_circum(r, n=100, center_x=0, center_y=0):
    return [(math.cos(2*pi/n*x)*r + center_x,math.sin(2*pi/n*x)*r + center_y) for x in range(0,n+1)]

points = points_in_circum(cylinder_radius, no_subcylinders)
# arc length / 2 is radius of subcylinder
size = (pi * cylinder_radius) / no_subcylinders
h = 0.0
h_step = 1.0 / no_subcylinders
with open("gen_scene.scene", "w+") as out_scene:
    for (x, y) in points:
        rand_color = colorsys.hsv_to_rgb(h, 1.0, 1.0)
        out_scene.write("mtlcolor  {:.2f} {:.2f} {:.2f}  1 1 1  0.3 0.5 0.2 30 1 1.7\n".format(
            rand_color[0], rand_color[1], rand_color[2]))
        out_scene.write("cylinder  {:.2f} {:.2f} -10  0 0 1 {:.2f} 50\n".format(x, y, size/2))
        h += h_step

