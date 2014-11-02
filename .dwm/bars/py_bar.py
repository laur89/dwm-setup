#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# py3 !
# Requires psutil module for memory, cpu and bandwidth usage retrieval:
#   apt-get install python3-pip
#   pip-3 install psutil
#    (if install gives 'fatal error: Python.h: No such file or directory', then apt-get install python3-dev
######################################################
# Can't recall from whom Python base was take, but I suspect it was first developed by Lokaltog (I might be mistaken!)
# Design copied from TrillbyWhite's github page: https://github.com/TrilbyWhite/dwmStatus/blob/master/bar.png
######################################################
import sys
import datetime
import os
import re
import subprocess
from urllib.request import urlopen
import xml.dom.minidom
import psutil
import time
import string

modefile = '/tmp/DWM_statusbar_mode.dat'  # !!! Be careful with editing this - other programs might refer to that file !!!
startmode = '1'   # Default mode to start in
wx_log = "/data/.tmp/tartu_wx_log.txt"    # Be careful with editing this - another script manages this file !!!
interfaces_file = "/tmp/connected_interfaces.dat"  # Be careful with editing this - another script manages this file !!!

cpudata_main = '/proc/stat'     # File containing processor info

default_interface = 'eth0'
max_mode = 3  # Maximum mode value
min_mode = 1   # Minimum mode value

sleep_interval = 1    # refresh interval in seconds
#############################################

def run_cmd(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0].decode("utf-8").strip()

def find_mode():
    # Find which mode to run in:
    with open(modefile, 'r') as f:
        new_mode = f.read()

    if new_mode == 'restart':
        #path=os.path.realpath(__file__) # Path to the current script
        sys.stdout.flush()  # Flushes open file objects etc;
        python = sys.executable
        os.execl(python, python,  * sys.argv) # Resets itself
        sys.exit()

    return new_mode


class Segment:
    icons = {
        #'vol_mute': '\uea30',
        'vol_mute': '\u1e5b',
        #'vol_low': '\uea31',
        'vol_low': '\u1e5c',
        'vol_medium': '\uea32',
        'vol_high': '\uea33',
        #'music': '\uea36',
        'music': '\u1e5a',
        'music_off': '\uea36',
        'mail': '\uea22',
        'mail_inverted': '\ueaf6',
        'date': '\ueaf7',
        'time': '\u0001',
        #'cpu': '\u1e41',
        'cpu': '\u1e04',
        'gpu': '\u1e41',
        #'ram': '\ueae4',
        'ram': '\u1e3e',
        'hdd': '\u1e05',
        'AC': '\uea21',
        'network_eth': '\uea4b',
        'network_wlan': '\ueafd',
        'bat_full': '\uea27',
        'bat_medium': '\uea26',
        'bat_empty': '\uea25',
        'bat_charging': '\uea28',
        'bat_error': '\uea24',
        'thermometer': '\u1e42',
        'up_arrow': '\u1e4b',
        'down_arrow': '\u1e4a',
        'equals': '=',

        'double_arrow': '\u1e2b',
        'arrow': '\u1e00',
        'miniarrow': '\u1e02',
        'None': '',
    }
    bars = {
        #'round-regular': '\uead0\uead1\uead2\uead3\uead4\uead5\uead6',
        #'round-candycane': '\uead0\uead1\uead2\uead7\uead8\uead9\ueada',
        'round-regular': '\uead0\uead1\uead2\uead3\uead4\uead5\uead6',
        'round-candycane': '\uead0\uead1\uead2\uead7\uead8\uead9\ueada',
    }
    colors = {
        # Color namings have to be like <txtcolor_bgcolor> and corresponding <bgcolor_txtcolor>,
        # unless you're bypassing the arrows by calling self.build_module with third parameter (see Time() for example) !!!
        'normal': '\x01',
        'urgent': '\x03',
        'error': '\x04',
        'white': '\x07',
        'warning': '\x08',

        'black_yellow': '\x09',    # black text, yellow bg
        'yellow_black': '\x0A',

        'black_blue': '\x0B',     # black text, blue bg
        'blue_black': '\x0C',
        'white_blue': '\x0D',

        'black_gray': '\x0E',       # black text, gray bg
        'gray_black': '\x0F',
        'white_gray': '\x10',       # gray bg, white txt

        'black_orange': '\x12',     # black text, brght orange bg
        'orange_black': '\x13',
        'white_orange': '\x11',     # white text on brght orange bg

        'black_magenta': '\x14',     # black text, magenta bg
        'magenta_black': '\x15',
        'white_magenta': '\x08',


        # Dummy vars:
        'None': '',
        'None_black': '',
        'black_None': '',
    }


    def get_bar(self, percent, length=10, type='round-regular'):
        bar = []
        fill_length = round(percent / 100 * length)

        chars_empty = self.bars[type][0:3]
        chars_filled = self.bars[type][3:7]

        if not fill_length:
            # Empty bar
            bar.append(chars_empty[0])
            bar.append(chars_empty[1] * (length - 1))
            bar.append(chars_empty[2])
        elif fill_length == length:
            # Full bar
            bar.append(chars_filled[0])
            bar.append(chars_filled[1] * (length - 1))
            bar.append(chars_filled[2])
        else:
            # Partially filled bar
            bar.append(chars_filled[0])
            bar.append(chars_filled[1] * (fill_length - 1))
            bar.append(chars_filled[3])

            bar.append(chars_empty[1] * (length - fill_length - 1))
            bar.append(chars_empty[2])

        self._bar = ''.join(bar)

        #return bar + '\u0002'
        #return bar

    def set_icon(self, icon='None', color='None'):
        if icon == None:
            self._icon = ''
        else:
            self._icon = self.colors.get(color) + self.icons.get(icon) + '\u3000'  # Note there's tailing space entered after the icon (\u3000); this will be added to the module block with or without using icon;


    def build_module(self, text=None, color='None', arrows=True):
        ''' builds the final output. if 3rd arg is set for self.build_module, then leading and trailing arrows won't be added;
           if first arg (text) is None, then self._module will be empty string '''
        if text == None:
            self._module = ''
        elif arrows == True:
            self._module = str(self.colors.get(color[color.find('_')+1:]+'_black') +
                                    self.icons.get('arrow') +
                                    self._icon +
                                    self.colors.get(color) +
                                    text +
                                    self.colors.get('black_'+color[color.find('_')+1:]) +
                                    self.icons.get('arrow')
                                )
        else:
            # i.e. don't add the trailing and leading arrow:
            self._module = self.colors.get(color) + self._icon + text


class Music(Segment):
    trim_length = 30    # nr of characters to trim song info to;

    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('music', color)
        self.build_module('Not playing', color)

        try:
            cmd = run_cmd(['timeout', '1s', 'mpc'])
        except OSError:
            return

        if 'playing' in cmd:
            text = []

            [playing, info] = cmd.split('\n')[:2]
            percent = int(re.search('(\d+)%', info).group(1))   # extracts % value from second, i.e. 'info' line

            if len(playing) > self.trim_length:
                playing = playing[:self.trim_length] + '...'

            (current, total) = re.findall('(\d+:\d+)', info)

            text.extend([
                playing,
                current + str('/') + total,
            ])

            try:
                bat._bar  # Show progress bar only if there's no bar present for the battery
            except:
                self.get_bar(percent),
                text.extend([
                    self._bar,
                ])


            self.build_module(' '.join(text), color)


class Vol(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        # Remove volume bar if battery bar is present:
        try:
            bat._bar
            no_bat_bar = False
        except:
            no_bat_bar = True


        try:
            cmd = run_cmd(['amixer', 'get', 'Master'])
        except OSError:
            return

        try:
            volume = int(re.search('\[(\d+)%\]', cmd).group(1))
            muted = re.search('\[(on|off)\]', cmd).group(1) == 'off'
        except AttributeError:
            self.set_icon('vol_mute', color)
            self.build_module('No sound', color)
            return

        if muted:
            self.set_icon('vol_mute', color)
            if no_bat_bar:
                self.get_bar(0)
                self.build_module(self._bar, color)
            else:
                self.build_module('', color)
            return

        if volume < 40:
            self.set_icon('vol_low', color)
        elif volume > 69:
            self.set_icon('vol_high', color)
        else:
            self.set_icon('vol_medium', color)

        if no_bat_bar:
            self.get_bar(volume)
            self.build_module(self._bar+'\u3000'+str(volume)+'%', color)
        else:
            self.build_module(str(volume)+'%', color)

class MailSegment(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('mail')
        self.build_module('N/A')

        unread = []
        hl = False

        try:
            for account in open(os.environ['XDG_CONFIG_HOME'] + '/gmailaccounts', encoding='utf-8'):
                (url, user, passwd) = account.split('|')

                auth_handler = urllib.request.HTTPBasicAuthHandler()
                auth_handler.add_password(realm='New mail feed', uri='https://mail.google.com/', user=user, passwd=passwd)
                opener = urllib.request.build_opener(auth_handler)
                urllib.request.install_opener(opener)

                request = urlopen(url)
                dom = xml.dom.minidom.parseString(request.read())
                count = dom.getElementsByTagName('fullcount')[0].childNodes[0].data

                if int(count) > 0:
                    hl = True

                unread.append(count)
        except (IOError, ValueError, KeyError):
            return

        if hl:
            self.set_icon('mail')

        self.build_module(' / '.join(unread))


class Date(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon(None)
        self.build_module(datetime.datetime.now().strftime('%a %d'), color)


class Time(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('None', color)
        self.build_module(datetime.datetime.now().strftime('%R'), color, False) # 'False' bypasses the arrow addition, allowing also calling this class with only 1 color


class Mem(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('ram', color)

        mem = str(round(psutil.phymem_usage()[3])) + '%'
        self.build_module(mem, color)

class CPU(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color, all_cpus=False):
        Segment.__init__(self)

        self.set_icon('cpu', color)

        if all_cpus == True:
        #  CPU percentage per core:
            cpuloads = psutil.cpu_percent(interval=sleep_interval, percpu=True)

            output = ''
            for i, load in enumerate(cpuloads):
                rounded = str(round(load)) + '%'
                if len(rounded) == 2:
                    rounded = ' ' + rounded  # pad the value if only one digit
                output = output + 'CPU' + str(i) + ': ' + rounded + ' ' + self.icons.get('miniarrow') + ' '

            # remove the trailing ' | ' from last item:
            output = output[0:len(output)-3]

        else:
        #  CPU percentage overall:
            output = str(round(psutil.cpu_percent(interval=sleep_interval))) + '%'
            if len(output) == 2:
                output = ' ' + output # pad the value if only one digit

        self.build_module(output, color)

class CPU_manual(Segment):
    ''' Calculates cpu percentage without the use of psutil lib '''

    def __str__(self):
        return self._module

    def __init__(self, color, all_cpus=False):
        Segment.__init__(self)

        self.set_icon('cpu', color)

        global cpu_dict

        # Read in CPU data:
        with open(cpudata_main, 'r') as f:
            cpu_datalines = f.readlines()

        if all_cpus == True:
        #  CPU percentage per core:
            output = ''

            for i, line in enumerate(cpu_datalines[1:]):
                if line.startswith('cpu'):
                    # Load the old values from the dictionary:
                    idle_old = cpu_dict.get(i)[0]
                    cpu_old_total = cpu_dict.get(i)[1]

                    line = line.split()
                    idle = int(line[4])
                    cpu_total = int(line[1]) + int(line[2]) + int(line[3]) + idle
                    cpu_diff = cpu_total - cpu_old_total

                    cpu_perc = str(round(100 * (cpu_diff - (idle - idle_old)) / cpu_diff)) + '%'

                    if len(cpu_perc) == 2:
                        cpu_perc = ' ' + cpu_perc  # pad the value if only one digit


                    output = output + 'CPU' + str(i) + ': ' + cpu_perc + ' ' + self.icons.get('miniarrow') + ' '

                    # add old data to dictionary:
                    cpu_dict[i] = [idle, cpu_total]
                else:
                    break

            # remove the trailing ' | ' from last item:
            output = output[0:len(output)-3]

        else:
        #  CPU percentage overall:

            cpu_data = cpu_datalines[0].split()

            idle = int(cpu_data[4])
            cpu_total = int(cpu_data[1]) + int(cpu_data[2]) + int(cpu_data[3]) + idle
            cpu_diff = cpu_total - cpu_old_total

            output = str(round(100 * (cpu_diff - (idle - idle_old)) / cpu_diff)) + '%'

            idle_old = idle
            cpu_old_total = cpu_total

            if len(output) == 2:
                output = ' ' + output  # pad the value if only one digit

        self.build_module(output, color)


class cpu_and_mem(Segment):
    ''' Prints total CPU and Mem percentage within single module '''
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)
        self.set_icon('cpu', color)

        cpu = str(round(psutil.cpu_percent(interval=sleep_interval))) + '%'
        if len(cpu) == 2:
            cpu=' ' + cpu # pad the value if only one digit

        mem = str(round(psutil.phymem_usage()[2])) + '%'

        self.build_module(cpu + '\u3000' + self.icons.get('miniarrow')  + '\u3000' +
                                                            self.icons.get('ram') + '\u3000' + mem
                                                                                            , color)

class cpu_and_temp(Segment):
    ''' Prints recource usage and temperatures per cpu core '''

    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('cpu', color)

        # Find and store the temps:
        temps = []
        try:
            systemps = os.popen('sensors').readlines()
            for line in systemps:
                if line.startswith('Core '):
                    temps.append(line.split()[2][1:])
        except:
            self.build_module("temps can't be read from sensors", color)
            return

        # CPU percentages & module assembly:
        cpuloads = psutil.cpu_percent(interval=sleep_interval, percpu=True)

        output = ''
        for i, load in enumerate(cpuloads):
            rounded = str(round(load)) + '%'
            if len(rounded) == 2:
                rounded = ' ' + rounded  # pad the value if only one digit
            output = output + 'CPU' + str(i) + ': ' + rounded + ' / ' + temps[i] + ' ' + self.icons.get('miniarrow') + ' '

        # remove the trailing ' | ' from last item:
        output = output[0:len(output)-3]


        self.build_module(output, color)


class bat_(Segment):
    # FYI: to simply see whether AC power is connected:   cat /sys/class/power_supply/C1F2/online
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        bat = run_cmd(['acpi', '-b']).split()
        if len(bat) == 0:
            bat_stat = ""
        else:
            per = bat[3]
            bat_stat = bat[2]
            bat_percentage = int(per.rstrip('%,'))    # Removes punctuation

        if 'Discharging,' in bat_stat:
            if bat_percentage >= 50:
                self.set_icon('bat_full', color)
            elif bat_percentage <= 10:
                color = 'white_orange'
                self.set_icon('bat_empty', color)
            else:
                color = 'white_magenta'
                self.set_icon('bat_medium', color)
            self.get_bar(bat_percentage, 18, 'round-candycane')
            self.build_module(self._bar + '\u3000' + str(bat_percentage) + '%', color)
        elif 'Charging,' in bat_stat:
            self.set_icon('bat_charging', color)
            self.get_bar(bat_percentage, 18)
            self.build_module(self._bar + '\u3000' + str(bat_percentage) + '%', color)
        #elif 'Unknown,' in bat_stat:
        #    color='urgency_gray'
        #    self.set_icon('bat_error', color)
        #    self.build_module('Battery status unknown!', color)
        else:
            #self.set_icon('AC', color)
            #self.build_module('', color)
            self.build_module(None)

class wx(Segment):
    ''' Reads temperature from custom WX file that's managed by different script '''

    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        try:
            with open(wx_log, 'r') as f:
                wx_info = f.readlines()

            # Extract temperature (implying temp is on the second line, i.e. indx 1):
            index = wx_info[1].find('Temp:')
            index2 = wx_info[1].find(' °C')
            temp = wx_info[1][index+7:index2] + '°C'

            # extract the trend:
            trend = wx_info[1][index2+4:].rstrip()

            if trend == "(=)":
                self.set_icon('equals', color)
            elif trend == "(up)":
                self.set_icon('up_arrow', color)
            elif trend == "(down)":
                self.set_icon('down_arrow', color)
            else:
                # this basically means some kind of unexpected behaviour here:
                self.set_icon('thermometer', color)


            #try:
            # Only display wx, if battery info isn't shown, i.e. battery info essentially replaces wx info;
            # otherwise statusbar gets too long and is placed partially behind systray:
                #bat._bar
                #self.build_module(None)
                #return
            #except:
                #pass
        except:
            self.build_module("wx file can't be read", color)
            return
        self.build_module(temp, color)

class cpu_temp(Segment):
    ''' Prints sys temps '''

    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('cpu', color)

        try:
            systemps = os.popen('sensors').readlines()
            cpu_output = ''
            for line in systemps:
                if line.startswith('Core '):
                    cpu_output = cpu_output + line.split()[2][1:] + ' ' + self.icons.get('miniarrow') + ' '

            # remove the trailing ' | ' from last item:
            cpu_output = cpu_output[0:len(cpu_output)-3]
            self.build_module(cpu_output, color)
        except:
            self.build_module("temps can't be read from sensors", color)

class hdd_temp(Segment):
    ''' Prints sys temps '''

    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('hdd', color)

        try:
            hdd_temp = run_cmd(['sudo', 'hddtemp', '/dev/sda', '-n']) + '°C'
            self.build_module(hdd_temp, color)
        except:
            self.build_module("temp can't be read from hddtemp", color)


class gpu_temp(Segment):
    ''' Prints sys temps '''

    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('gpu', color)

        try:
            gpu_temp = run_cmd(['nvidia-settings', '-q', 'GPUCoreTemp', '-t']) + '°C'
            self.build_module(gpu_temp, color)
        except:
            self.build_module("temp can't be read from nvidia-settings", color)

class network(Segment):
    def __str__(self):
        return self._module

    def __init__(self, color):
        Segment.__init__(self)
        # Rx/Tx data also at  /sys/class/net/wlan0/statistics/[tx_bytes | rx_bytes]

        global ntw_startTime
        global recv_old
        global sent_old

        self.set_icon('network_eth', color)

        nw_data = psutil.network_io_counters(pernic=True) # a dict datatype;
        ntw_endTime = time.time()
        timediff = float(ntw_endTime - ntw_startTime)
        ntw_startTime = time.time()   # As timediff got calculated, log the new startTime

        # Find which interface data to show:
        try:
            with open(interfaces_file, 'r') as f:
                nw_local_data = f.readlines()
            active_line = nw_local_data[1].split()  # Reads the line containing interface labled as 'default' by network-manager;
            active = active_line[1]
            if active == 'wlan0':
                int_data = nw_data.get(active)
                self.set_icon('network_wlan', color)

                # Try to find SSID as well:
                if len(nw_local_data) >= 3:
                    ssid_line = nw_local_data[2].split()
                    active = active + '/' + ssid_line[1]
            elif active == 'eth0' or active == 'eth1':
                int_data = nw_data.get(active)
            else:
                # No (known) connection:
                active = None
        except:
            active = None


        if active != None:
            sent = float(int_data[0])
            recv = float(int_data[1])

            sent_diff = float((sent - sent_old) * 8)  # Find the difference between last measure point, convert to bits
            recv_diff = float((recv - recv_old) * 8)

            # Find current Tx:
            sent_current = round( ((sent_diff / 1048576) / timediff), 1) # translates into Mbit

            if sent_current < 1.0:
                sent_current = str(round( ((sent_diff / 1024) / timediff))) + ' Kbit/s'
            elif sent_current > 1024.0:
                sent_current = str(round( ((sent_diff / 1073741824) / timediff), 2)) + ' Gbit/s'
            else:
                sent_current = str(sent_current) + ' Mbit/s'

            # Find current Rx:
            recv_current = round( ((recv_diff / 1048576) / timediff), 1) # translates into Mbit

            if recv_current < 1.0:
                recv_current = str(round( ((recv_diff / 1024) / timediff))) + ' Kbit/s'
            elif recv_current > 1024.0:
                recv_current = str(round( ((recv_diff / 1073741824) / timediff), 2)) + ' Gbit/s'
            else:
                recv_current = str(recv_current) + ' Mbit/s'



            # Find total sent:
            sent_total = round(sent / 1048576, 1) # translates into MB

            if sent_total < 1.0:
                sent_total = str(round(sent / 1024)) + ' KB'
            elif sent_total > 1024.0:
                sent_total = str(round(sent / 1073741824, 2)) + ' GB'
            else:
                sent_total = str(sent_total) + ' MB'

            # Find total recv:
            recv_total = round(recv / 1048576, 1) # translates into MB

            if recv_total < 1.0:
                recv_total = str(round(recv / 1024)) + ' KB'
            elif recv_total > 1024.0:
                recv_total = str(round(recv / 1073741824, 2)) + ' GB'
            else:
                recv_total = str(recv_total) + ' MB'

            # Reset the variables:
            sent_old = sent
            recv_old = recv


            self.build_module(
                str(active + self.icons.get('miniarrow') + '\u3000' + self.icons.get('down_arrow')  + '\u3000' + recv_current +
                '\u3000' + self.icons.get('double_arrow') + '\u3000' + recv_total + self.icons.get('miniarrow') + '\u3000' +
                self.icons.get('up_arrow')  + '\u3000' + sent_current + '\u3000' + self.icons.get('double_arrow') + '\u3000' + sent_total)
                            , color)

        else:   # No (known) interfaces connected:
            self.build_module('No connection', color)



############ START ##############
###############################

# Find usage, if cml arguments were passed:
if len(sys.argv) > 1:
    # Find current mode:
    with open(modefile, 'r') as f:
        mode = int(f.read())

    arg = sys.argv[1]

    if arg == 'next':
        # Switch to next mode:
        mode += 1
        if mode > max_mode:
            mode = min_mode # Start from beginning
    elif arg == 'prev':
        # Switch to previous mode:
        mode -= 1
        if mode < min_mode:
            mode = max_mode # Go to last mode
    else:
        print('unknown usage option. abort.')
        sys.exit(1)

    # Write new mode to statusfile:
    with open(modefile, 'w') as f:
        f.write(str(mode))

    sys.exit()




############################### VAR INITIALIZATION ##########################
# network vars initialization:
nw_data = psutil.network_io_counters(pernic=True)
ntw_startTime = int(time.time())

# Find which interface data to fetch:
try:
    with open(interfaces_file, 'r') as f:
        active_line = f.readlines()[1].split()

    if len(active_line) > 1:
        active = active_line[1]
        int_data = nw_data.get(active)
    else:   # Meaning no 'default' interface was listed
        # because vars have to be initialized nevertheless:
        int_data = nw_data.get(default_interface)

except: # meaning interfaces_file has not yet been created, so initialize vars with wlan data:
    int_data = nw_data.get(default_interface)

sent_old = float(int_data[0])
recv_old = float(int_data[1])
###################################
# CPU vars initialization (for manual calculation class):

with open(cpudata_main, 'r') as f:
    cpu_datalines = f.readlines()

cpu_data = cpu_datalines[0].split()
idle_old = int(cpu_data[4])
cpu_old_total = int(cpu_data[1]) + int(cpu_data[2]) + int(cpu_data[3]) + idle_old

# Per core var initialization:
cpu_dict = {}

for i, line in enumerate(cpu_datalines[1:]):
    if line.startswith('cpu'):
        line = line.split()
        idle = int(line[4])
        cpu_total = int(line[1]) + int(line[2]) + int(line[3]) + idle

        # add old data to dictionary:
        cpu_dict[i] = [idle, cpu_total]
    else:
        break

#################### END OF INITIALIZATION  ###########################

# Write the default mode to statusfile:
with open(modefile, 'w') as f:
    f.write(startmode)

mode = startmode    # Default starting status
###############################

# Start the loop:
while 1:
    try:
        if mode == '1':
            #cpu = CPU('black_yellow', False) # if second parameter is True, then CPU() returns recource usage for all cores
            #mem = Mem('black_blue')
#            bat = bat_('white_gray')
            time_ = Time('white')
            date = Date('white_gray')
            vol = Vol('white_blue')
            music = Music('black_yellow')
#            weather = wx('white_gray')
            cpu = cpu_and_mem('white_gray')

            status=str(
                                    str(cpu) +
#                                    str(bat) +
                                    str(music) +
#                                    str(weather) +
                                    str(vol) +
                                    str(date) +
                                    str(time_)
                            )


            #del bat        # Remove bat object, as many formatting rules depend on its _bar existence;
            #time.sleep(sleep_interval)  # sleep provided by the CPU class (psutils)

        elif mode == '2':
            bat = bat_('white_gray')
            time_ = Time('white')
            date = Date('white_gray')
            vol = Vol('white_blue')
            ntw = network('black_yellow')
            weather = wx('white_gray')

            status=str(
                                    str(ntw) +
                                    str(bat) +
                                    str(weather) +
                                    str(vol) +
                                    str(date) +
                                    str(time_)
                            )

            time.sleep(sleep_interval)

        elif mode == '3':
            #cpu = CPU_manual('white_gray', True)
            cpu = cpu_and_temp('white_gray')
#            bat = bat_('white_gray')
            time_ = Time('white')
            date = Date('white_gray')
            vol = Vol('white_blue')
            weather = wx('white_gray')
            #cpu_temp_ = cpu_temp('black_yellow')
            hdd_temp_ = hdd_temp('black_yellow')
            gpu_temp_ = gpu_temp('black_yellow')
            mem = Mem('black_yellow')

            status=str(
                                    str(mem) +
#                                    str(bat) +
                                    str(cpu) +
                                    #str(cpu_temp_) +
                                    str(gpu_temp_) +
                                    str(hdd_temp_) +
                                    str(weather) +
                                    str(vol) +
                                    str(date) +
                                    str(time_)
                            )

            #time.sleep(sleep_interval)

        else:
            # Script exits on unrecongnised mode:
            sys.exit()

        # Finally, set the root window:
        os.system('xsetroot -name "' + status + '        "')   # trailing spaces are because of the systray; try without them, you'll see;

        # Find operating mode:
        mode = find_mode()
    except:
        os.system('xsetroot -name "\x04Exception at the py_bar main loop. Abort.         "')
        sys.exit(1)



