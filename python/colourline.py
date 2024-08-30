from tkinter import *
from tkinterdnd2 import *

import webcolors

def color(value):
    r=0
    b=0
    g=0

    # y = mx + b
    # m = 4
    # x = value
    # y = RGB._
    if (0 <= value and value <= 1/8) :
        r = 0;
        g = 0;
        b = 4*value + .5; # .5 - 1 // b = 1/2
    elif 1/8 < value and value <= 3/8:
        r = 0;
        g = 4*value - .5; # 0 - 1 // b = - 1/2
        b = 1; # small fix
    elif 3/8 < value and value <= 5/8:
        r = 4*value - 1.5; # 0 - 1 // b = - 3/2
        g = 1;
        b = -4*value + 2.5; # 1 - 0 // b = 5/2
    elif 5/8 < value and value <= 7/8 :
        r = 1;
        g = -4*value + 3.5; # 1 - 0 // b = 7/2
        b = 0;
    elif 7/8 < value and value <= 1 :
        r = -4*value + 4.5; # 1 - .5 // b = 9/2
        g = 0;
        b = 0;
    else:   # should never happen - value > 1
        r = .5;
        g = 0;
        b = 0;


    # scale for hex conversion
    r *= 255;
    g *= 255;
    b *= 255;
  #  print(r,g,b)
    #return Math.round(RGB.R).toString(16)+''+Math.round(RGB.G).toString(16)+''+Math.round(RGB.B).toString(16)
    return (r,g,b)

# create TK root
root = TkinterDnD.Tk()
canvas = Canvas(root, width=1500, height=1060)

MAX=445
a="#define LEDMAX "+ str(MAX)+"\n"
a=a+"uint8_t colline["+str(MAX)+"][3]={\n"

for x in range(0,MAX):
    r,g,b=color(x/MAX)
    hexcol=webcolors.rgb_to_hex((int(r),int(g),int(b)))
    canvas.create_line(x, 0, x, 100, fill="#000000", width=1)
    canvas.create_line(x, 0, x, 100, fill=hexcol, width=1)
    a = a + "    {" + str(int(r)) + ","
    a = a + str(int(g)) + ","
    a = a + str(int(b)) +"},\n";
a = a + "};\n";

print (a)

canvas.pack()
root.mainloop()