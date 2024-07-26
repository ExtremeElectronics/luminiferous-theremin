from tkinter import *
from tkinterdnd2 import *

mfrequency=[
    78,82,87,92,98,104,110,117,123,131,139,147,
    156,165,175,185,196,208,220,233,247,262,277,294,
    311,330,349,370,392,415,440,466,494,523,554,587,
    622,659,698,740,784,831,880,932,988,1047,1109,1175,
    1245,1319,1397,1480,1568,1661,1760,1865,1976,2093,2217,
    2349]

MINDISTANCE=20
MAXDISTANCE=350

fa=MAXDISTANCE-MINDISTANCE

fn=len( mfrequency )
fc=(fa/fn)

fdarray=[]

#zeros for mindistance
for xc in range(MINDISTANCE):
   fdarray.append(0)

for i in range(fn-1):
    f1=mfrequency[i]
    f2=mfrequency[i+1]
    #fdarray.append(f1)
    x=(f2-f1)/fc
    for xc in range(int(fc)):
        fdarray.append(int(f1 + x * xc))

for xc in range(int(fc)-1):
    fdarray.append(int(f2+x*xc))

fdarray.append(0)

print (len(fdarray))

c=0
a="//mapping distance to frequency\n"
a+="//from distarray.py\n"
a+="// with MINDISTANCE "+str(MINDISTANCE)+" and MAXDISTANCE "+str(MAXDISTANCE)+"\n\n"

a+="uint16_t farray["+str(len(fdarray))+"]={\n    "
for z in range(len(fdarray)):
    print (z,fdarray[z])
    a=a+str(fdarray[z])+","
    c=c+1
    if(c==16):
        c=0
        a=a+"\n    "
a=a.strip(',') +"\n};\n"

print (a)


# create TK root
root = TkinterDnD.Tk()

canvas = Canvas(root, width=1500, height=1060)

for z in range(1,len(fdarray)):
    canvas.create_line(fdarray[z-1]/2,(z-1)*2,fdarray[z]/2,z*2, fill="#000000", width=2)
    if(fdarray[z] in mfrequency):
        canvas.create_line(fdarray[z] / 2+10, z*2 , fdarray[z] / 2-10, z*2, fill="#ff0000", width=2)

canvas.pack()
root.mainloop()


