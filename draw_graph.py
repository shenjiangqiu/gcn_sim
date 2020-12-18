#!/bin/python3
#%%
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import sys

file_name=sys.argv[1]
f=open(file_name)
f_rec=open(sys.argv[2])

line=f.readline()
i=0
fig,ax=plt.subplots()
while len(line)>0:
    array=np.array(line.split(),dtype=int)
    array=np.append(array,i)
    print(array)
    ax.plot([i for _ in range(len(array))],array,'ro')
    line=f.readline()
    i=i+1

line=f_rec.readline()
while len(line)>0:
    a=np.array(line.split(),dtype=int)
    rec=patches.Rectangle((a[0],a[1]),3,a[2]-1,edgecolor='r',facecolor='none')
    ax.add_patch(rec)
    line=f_rec.readline()
plt.savefig("fig")
