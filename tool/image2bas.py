#!/usr/bin/env python3
import os
from sys import argv
from PIL import Image
#import numpy as np
#import pandas as pd
import operator

imagefile="";



try:
  imagefile = argv[1]

except:
  print('missing image file')
  exit(1)

basic= os.path.splitext(imagefile);
basicfile=f'{basic[0]}.bas'

img = Image.open(imagefile)
rgb_im = img.convert("RGB")
pixdata = rgb_im.load()
width, height = img.size

pixel = 8

# 0 rot
# 1 grün
# 2 gelb
# 3 blau
# 4 magenta
# 5 cyan
# 6 weiß
# 7 weiß

lines = []
count_color_ = 0

cred=0
cgreen=0
cblue=0
cyellow=0
ccyan=0
cmagenta=0
Cblack=0
Cwhite=0

for y in range(0,height,pixel):
    for x in range(0,width,pixel):
        r, g, b = rgb_im.getpixel((x, y))


        print(f'{r} {g} {b}');

        #red
        if r==255 and g<=50 and b<=50:
            #print("USE COLOR RED")
            c=0
            cred += 1
        #green
        elif r<=50 and g==255 and b<=50:
            #print("USE COLOR GREEN")
            c=1
            cgreen += 1
        #blue
        elif r<=50 and g<=50 and b==255:
            #print("USE COLOR BLUE")
            c=3
            cblue += 1
        #yellow
        elif r>128 and g>=128 and b<=50:
            #print("USE COLOR YELLOW")
            c=2
            cyellow += 1
        #cyan
        elif r<=50 and g>128 and b>128:
            #print("USE COLOR CYAN")
            c=5
            ccyan += 1
        #magenta
        elif r>128 and g<=50 and b>128:
            #print("USE COLOR MAGENTA")
            c=4
            cmagenta += 1
        #black
        elif r<=128 and g<=128 and b<=128:
            #print("USE COLOR BLACK")
            c=15
            Cblack += 1
        #white
        else:
            #print("USE COLOR WHITE")
            c=7
            Cwhite += 1

        lines.append([c,c,x,y,x+pixel,y+pixel,pixel])

i=0
f = open(basicfile, "w")

'''make background frame with the most color'''
backgoundcolor = [[cred,0],[cgreen,1],[cblue,3],[cyellow,2],[ccyan,5],[cmagenta,4],[Cblack,15],[Cwhite,7]]
value = max(enumerate(backgoundcolor), key=operator.itemgetter(1))
bg = value[0]

i=i+10
f.write(f'{i} REM BG FRAMECOLOR {bg}\n')

i=i+10
f.write(f'{i} rectangle {bg},{bg},{0},{0},{640},{200},{pixel}\n')


for line in lines:
    if line[0]!=bg:
        i=i+10
        print(f'{i} rectangle {line[0]},{line[1]},{line[2]},{line[3]},{line[4]},{line[5]},{line[6]}')
        f.write(f'{i} rectangle {line[0]},{line[1]},{line[2]},{line[3]},{line[4]},{line[5]},{line[6]}\n')
f.close()
