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

modefile='/tmp/DWM_statusbar_mode.dat'  # !!! Be careful with editing this - other programs refer to that file !!!
startmode='1'   # Default startmode
wx_log="/data/.tmp/tartu_wx_log.txt"    # Be careful with editing this - another script manages this file !!!

max_mode=2  # Maximum mode value
min_mode=1   # Minimum mode value
#############################################

def run_cmd(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0].decode("utf-8").strip()

def find_status():
    # Find which mode to run in:
    f = open(modefile, 'r')
    new_status=f.read()
    f.close()
        
    if new_status == 'restart':
        # Reset the status in statusfile:
        #f = open(modefile, 'w')
        #f.write(status)
        #f.close()
        
        #path=os.path.realpath(__file__) # Path to the current script
        sys.stdout.flush()  # Flushes open file objects etc;
        python = sys.executable
        os.execl(python, python,  * sys.argv) # Resets itself

    return str(new_status)
    
def generate_bat_vars():
    # Generate bat_stat and percentage variables that are used by other classes:
    global bat_stat
    global bat_percentage

    bat = run_cmd(['acpi', '-b'])
    bat=bat.split(' ')
    per=bat[3]
    bat_stat=bat[2]

    bat_percentage=''
    for letter in per:
        if letter not in string.punctuation:
            bat_percentage += letter
    bat_percentage=int(bat_percentage)
    
    
        
    
class Segment:
    icons = {
        'vol_mute': '\uea30',
        'vol_low': '\uea31',
        'vol_medium': '\uea32',
        'vol_high': '\uea33',
        'music': '\uea36',
        'mail': '\uea22',
        'mail_inverted': '\ueaf6',
        'date': '\ueaf7',
        'time': '\u0001',
        'cpu': '\u1e41',
        'ram': '\ueae4',
        'AC': '\uea21',
        'bat_full': '\uea27',
        'bat_medium': '\uea26',
        'bat_empty': '\uea25',
        'bat_charging': '\uea28',
        'bat_error': '\uea24',
        
        'arrow': '\u1e00',
        'None': '',
    }
    bars = {
        'round-regular': '\uead0\uead1\uead2\uead3\uead4\uead5\uead6',
        'round-candycane': '\uead0\uead1\uead2\uead7\uead8\uead9\ueada',
    }
    colors = {
        # Color namings have to be like color_background and corresponding background_color, unless you're bypassing the arrow usage by calling self.set_text with third parameter !!!
        'normal': '\x01',
        'urgent': '\x03',
        'error': '\x04',
        'white': '\x07',
        'warning': '\x08',
        
        'black_yellow': '\x09',    # black text, yellow bg
        'yellow_black': '\x0C',
        
        'black_blue': '\x0A',     # black text, blue bg
        'blue_black': '\x0D',
        
        'black_gray': '\x0B',       # black text, gray bg
        'gray_black': '\x0E',
        
        'urgency_gray': '\x0F',
        'orage_gray': '\x10',       # find out what that is lol
        
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

        bar = ''.join(bar)

        #return bar + '\u0002'
        return bar

    def set_icon(self, icon='None', color='None'):
        self._icon = self.colors.get(color) + self.icons.get(icon) + '\u00a0'  # Note there's tailing space entered after the icon (\u00a0)


    def set_text(self, text=None, color='None', arrows='True'):
        ''' builds the final output. if 3rd arg is set for self.set_text, then leading and trailing arrows won't be added;
           if first arg (text) is None, then self._text will be empty string '''
        if text == None:
            self._text = ''
        elif arrows == 'True':
            self._text = str(self.colors.get(color[color.find('_')+1:]+'_black') +
                                    self.icons.get('arrow') +
                                    self._icon +
                                    self.colors.get(color) +
                                    text +
                                    self.colors.get('black_'+color[color.find('_')+1:]) +
                                    self.icons.get('arrow')
                                )
        else:
            ''' i.e. don't add the trailing and leading arrow '''
            self._text = self.colors.get(color) + self._icon + text


class Music(Segment):
    trim_length = 30    # nr of characters to trim song info to;
    
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('music', color)
        self.set_text('Not playing', color)

        try:
            cmd = run_cmd(['timeout', '1s', 'mpc'])
        except OSError:
            return

        if 'playing' in cmd:
            text = []

            [playing, info, dummy] = cmd.split('\n')
            percent = int(re.search('(\d+)%', info).group(1))

            if len(playing) > self.trim_length:
                playing = playing[:self.trim_length] + '...'

            (current, total) = re.findall('(\d+:\d+)', info)

            text.extend([
                playing,
                current + str('/') + total,
            ])
            
            if 'Discharging,' not in bat_stat and 'Charging,' not in bat_stat:  # Show progress bar only if there's no bar present for the battery
                text.extend([
                    self.get_bar(percent),
                ])

            self.set_text(' '.join(text), color)


class Vol(Segment):
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        try:
            cmd = run_cmd(['amixer', 'get', 'Master'])
        except OSError:
            return

        try:
            volume = int(re.search('\[(\d+)%\]', cmd).group(1))
            muted = re.search('\[(on|off)\]', cmd).group(1) == 'off'
        except AttributeError:
            self.set_icon('vol_mute', color)
            self.set_text('No sound', color)

            return

        if muted:
            self.set_icon('vol_mute', color)
            self.set_text(self.get_bar(0), color)
            return

        if volume < 40:
            self.set_icon('vol_low', color)
        elif volume > 70:
            self.set_icon('vol_high', color)
        else:
            self.set_icon('vol_medium', color)

        self.set_text(self.get_bar(volume), color)

class MailSegment(Segment):
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('mail')
        self.set_text('N/A')

        unread = []
        hl = False

        try:
            for account in open(os.environ['XDG_CONFIG_HOME'] + '/gmailaccounts', encoding='utf-8'):
                (url, user, passwd) = account.split('|')

                auth_handler = urllib.request.HTTPBasicAuthHandler()
                auth_handler.add_password(realm='New mail feed', uri='https://mail.google.com/', user=user, passwd=passwd)
                opener = urllib.request.build_opener(auth_handler)
                urllib.request.install_opener(opener)

                request = urllib.request.urlopen(url)
                dom = xml.dom.minidom.parseString(request.read())
                count = dom.getElementsByTagName('fullcount')[0].childNodes[0].data

                if int(count) > 0:
                    hl = True

                unread.append(count)
        except (IOError, ValueError, KeyError):
            return

        if hl:
            self.set_icon('mail')

        self.set_text(' / '.join(unread))


class Date(Segment):
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('None', color)
        self.set_text(datetime.datetime.now().strftime('%a %d'), color)


class Time(Segment):
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('None', color)
        self.set_text(datetime.datetime.now().strftime('%R'), color, False) # 'False' bypasses the arrow addition;
        

class Mem(Segment):
    def __str__(self):
        return self._text
    
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('ram', color)
        
        mem=str(round(psutil.phymem_usage()[3])) + '%'
        self.set_text(mem, color)
        
class CPU(Segment):
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)
        self.set_icon('cpu', color)

        #  CPU percentage per core:
        #cpuloads = psutil.cpu_percent(interval=1, percpu=True)
        #cpu1 = str(round(cpuloads[0])) + '%'
        #if cpu1 == 2:
        #   cpu1=' ' + cpu1
        #cpu2 = str(round(cpuloads[1])) + '%'
        #if cpu2 == 2:
        #   cpu2=' ' + cpu2
        #self.set_text(cpu1 + ' | ' + cpu2, 'black_yellow')
        
        #  CPU percentage overall:
        cpuloads = str(round(psutil.cpu_percent(interval=1))) + '%'
        if len(cpuloads) == 2:
            cpuloads=' ' + cpuloads
        self.set_text(cpuloads, color)

class bat_(Segment):
    # FYI: to simply see whether AC power is connected:   cat /sys/class/power_supply/C1F2/online
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        if 'Discharging,' in bat_stat:
            if bat_percentage >= 50:
                #color='black_gray'
                self.set_icon('bat_full', color)
            elif bat_percentage <= 10:
                color='urgency_gray'
                self.set_icon('bat_empty', color)
            else:
                color='orage_gray'
                self.set_icon('bat_medium', color)
            self.set_text(self.get_bar(bat_percentage, 18, 'round-candycane')+'\u00a0'+str(bat_percentage)+'%', color)
        elif 'Charging,' in bat_stat:
            #color='black_gray'
            self.set_icon('bat_charging', color)
            self.set_text(self.get_bar(bat_percentage, 18)+'\u00a0'+str(bat_percentage)+'%', color)
        #elif 'Unknown,' in bat_stat:
        #    color='urgency_gray'
        #    self.set_icon('bat_error', color)
        #    self.set_text('Battery status unknown!', color)
        else:
            self.set_icon('AC', color)
            self.set_text('', color)
            
class wx(Segment):
    # reads weather info from file
    def __str__(self):
        return self._text
        
    def __init__(self, color):
        Segment.__init__(self)

        if (os.path.exists(wx_log)):
            f=open(wx_log, 'r')
            wx_info=f.read()
            f.close()
            
            # Extract temperature:
            index=wx_info.find('Temp:')
            index2=wx_info.find(' °C')
            temp=wx_info[index+7:index2] + '°C'
            #print(temp)

            self.set_icon('AC', color)
            self.set_text(temp, color)
        else:
            self.set_icon('None', color)
            self.set_text(None)
            
            
class network(Segment):
    def __str__(self):
        return self._text
        
    def __init__(self, color):        
        Segment.__init__(self)
        # WHY ARE THESE GLOBAL VAR DECLARATIONS REQUIRED, globals already exist; i should be able to use them (without editing ofc)????:
        global ntw_startTime
        global wlan_sent_old
        global wlan_recv_old
        
        nw_data=str(psutil.network_io_counters(pernic=True))
        
        ntw_endTime=time.time()
        timediff=float(ntw_endTime - ntw_startTime)
        ntw_startTime=time.time()
        #print(timediff)
        
        wlan_data=nw_data[nw_data.find('wlan0'):nw_data.find('eth0')]
        eth_data=nw_data[nw_data.find('eth0'):]
        
        wlan_sent=float(wlan_data[wlan_data.find('bytes_sent=')+11:wlan_data.find(', bytes_recv')])
        wlan_recv=float(wlan_data[wlan_data.find('bytes_recv=')+11:wlan_data.find(', packets_sent')])
        
        wlan_sent_diff=float((wlan_sent - wlan_sent_old) * 8)  # Find the difference between last measure point, convert to bits
        wlan_recv_diff=float((wlan_recv - wlan_recv_old) * 8) 
        
        # Find current Tx/wlan:
        wlan_sent_current=round( ((wlan_sent_diff / 1048576) / timediff), 1) # translates into Mbit
        
        if wlan_sent_current < 1.0:
            wlan_sent_current=str(round( ((wlan_sent_diff / 1024) / timediff), 1)) + ' Kbit/s'
        elif wlan_sent_current > 1024.0:
            wlan_sent_current=str(round( ((wlan_sent_diff / 1073741824) / timediff), 2)) + ' Gbit/s'
        else:
            wlan_sent_current=str(wlan_sent_current) + ' Mbit/s'
            
        # Find current Rx/wlan:
        wlan_recv_current=round( ((wlan_recv_diff / 1048576) / timediff), 1) # translates into Mbit
        
        if wlan_recv_current < 1.0:
            wlan_recv_current=str(round( ((wlan_recv_diff / 1024) / timediff), 1)) + ' Kbit/s'
        elif wlan_recv_current > 1024.0:
            wlan_recv_current=str(round( ((wlan_recv_diff / 1073741824) / timediff), 2)) + ' Gbit/s'
        else:
            wlan_recv_current=str(wlan_recv_current) + ' Mbit/s'

        
        
        # Find total sent/wlan:
        wlan_sent_total=round(wlan_sent / 1048576, 1) # translates into MB
        
        if wlan_sent_total < 1.0:
            wlan_sent_total=str(round(wlan_sent / 1024, 1)) + ' KB'
        elif wlan_sent_total > 1024.0:
            wlan_sent_total=str(round(wlan_sent / 1073741824, 2)) + ' GB'
        else:
            wlan_sent_total=str(wlan_sent_total) + ' MB'
            
        # Find total recv/wlan:
        wlan_recv_total=round(wlan_recv / 1048576, 1) # translates into MB
        
        if wlan_recv_total < 1.0:
            wlan_recv_total=str(round(wlan_recv / 1024, 1)) + ' KB'
        elif wlan_recv_total > 1024.0:
            wlan_recv_total=str(round(wlan_recv / 1073741824, 2)) + ' GB'
        else:
            wlan_recv_total=str(wlan_recv_total) + ' MB'
            
        # Reset the variables:
        wlan_sent_old=wlan_sent
        wlan_recv_old=wlan_recv
        
        
        self.set_icon('AC', color)
        self.set_text(str('Rx: ' + wlan_recv_current + '  ' + wlan_recv_total + ' | ' + 'Tx: ' + wlan_sent_current + '  ' + wlan_sent_total), color)


############ START ##############
# Find usage:
if len(sys.argv) > 1:
    # Find current mode:
    f = open(modefile, 'r')
    mode=f.read()
    f.close()
    
    if sys.argv[1] == 'next':
        # Switch to next mode:
        mode += 1
        if mode > max_mode:
            mode = min_mode # Start from beginning
    elif sys.argv[1] == 'prev':
        # Switch to previous mode:
        mode -= 1
        if mode < min_mode:
            mode = max_mode # Go to last mode
    else:
        print('unknown usage option. abort.')
        sys.exit(1)
        
    # Write new mode to statusfile:
    f = open(modefile, 'w')
    f.write(mode)
    f.close()
    
    sys.exit()
    
# initialize vars:
nw_data=str(psutil.network_io_counters(pernic=True))
ntw_startTime=int(time.time())

wlan_data=nw_data[nw_data.find('wlan0'):nw_data.find('eth0')]
eth_data=nw_data[nw_data.find('eth0'):]

wlan_sent_old=float(wlan_data[wlan_data.find('bytes_sent=')+11:wlan_data.find(', bytes_recv')])
wlan_recv_old=float(wlan_data[wlan_data.find('bytes_recv=')+11:wlan_data.find(', packets_sent')])

# Write the default status to statusfile:
f = open(modefile, 'w')
f.write(startmode)
f.close()
status=find_status()    # Default starting status

# Start the loop:
while 1:
    if status == '1':
        generate_bat_vars()
        
        cpu = str(CPU('black_yellow'))
        bat = str(bat_('black_gray'))
        mem = str(Mem('black_blue'))
        time_ = str(Time('white'))
        date = str(Date('black_gray'))
        vol = str(Vol('black_blue'))
        music = str(Music('black_yellow'))

        # space: \u00a0
        #run_cmd(['xsetroot', '-name'+' "', cpu, mem, bat, music, vol, date, time_ +'        '+'"'])

        command=str(
                                'xsetroot -name "'+
                                cpu +
                                mem +
                                bat +
                                music +
                                vol +
                                date +
                                time_ +
                                '        ' +
                        '"')
        #time.sleep(1)  # Currently is sleep provided by the CPU class
    elif status == '2':
        time_ = str(Time('white'))
        date = str(Date('black_gray'))
        vol = str(Vol('black_blue'))
        ntw = str(network('black_yellow'))
        weather = str(wx('black_gray'))
        
        command=str(
                                'xsetroot -name "'+
                                ntw +
                                weather +
                                vol +
                                date +
                                time_ +
                                '        ' +
                        '"')
        time.sleep(1)
    else:
        sys.exit()
    os.system(command)
    status=find_status()



