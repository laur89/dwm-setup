#!/usr/bin/env python3.2
# py3 !
# Requires psutil module for memory, cpu and bandwidth usage retrieval:
#   apt-get install python3-pip
#   pip-3 install psutil
import sys
import datetime
import os
import re
import subprocess
import urllib.request
import xml.dom.minidom
import psutil
import time
import string
import codecs

modefile='/tmp/DWM_statusbar_mode.dat'  # !!! Be careful with editing this - other programs refer to that file !!!
barfile='/tmp/DWM_statusbar_contents.dat'
startmode='1'   # Default startmode

max_mode=2  # Maximum mode value
min_mode=1   # Minimum mode value

sleep_interval = 0.2    # refresh interval
#############################################

def run_cmd(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0].decode("utf-8").strip()

def find_mode():
    # Find which mode to run in:
    f = open(modefile, 'r')
    new_mode=f.read()
    f.close()
        
    if new_mode == 'restart':
        # Reset the status in statusfile:
        #f = open(modefile, 'w')
        #f.write(mode)
        #f.close()
        
        #path=os.path.realpath(__file__) # Path to the current script
        sys.stdout.flush()  # Flushes open file objects etc;
        python = sys.executable
        os.execl(python, python,  * sys.argv) # Resets itself

    return str(new_mode)


# Write the default mode to statusfile:
f = open(modefile, 'w')
f.write(startmode)
f.close()
mode=startmode    # Default starting status
###############################

# Start the loop:
while 1:
    if mode == '1':
        barsource = ['c_a_m', 'bat', 'wx', 'music', 'vol', 'date', 'time']
    elif mode == '2':
        barsource = ['nw', 'wx', 'vol', 'date', 'time']
    else:
        # Script exits on unsupported mode
        sys.exit()
        
    contents = {}
    f = open(barfile, 'r')
    o = f.readlines()
    f.close()
    for line in o:
        #print(line)
        line = line.split()
        key = line[0]
        val = ' '.join(line[1:])
        #(key, val) = line.split()
        contents[key] = val

    
    #print(contents.get('cpu'))
    #sys.exit()
    #status=contents.get('cpu')
    
    
    status=''
    for item in barsource:
        status += contents.get(item)
    #print(status)
    status=bytes(status, 'ascii').decode('unicode-escape')
    
    # Finally, set the root window:
    os.system('xsetroot -name "' + status + '        "')   # trailing spaces are because of the systray;
    time.sleep(sleep_interval)
    mode=find_mode()
    #sys.exit()



