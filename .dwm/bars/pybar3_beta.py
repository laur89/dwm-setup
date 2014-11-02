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
interfaces_file="/tmp/connected_interfaces.dat" # Be careful with editing this - another script manages this file !!!

max_mode=2  # Maximum mode value
min_mode=1   # Minimum mode value

sleep_interval = 1    # refresh interval
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
    
def generate_bat_vars():
    # Generate bat_stat and percentage variables that might be used by other classes:
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
        # Color namings have to be like <color_background> and corresponding <background_color>,
        # unless you're bypassing the arrows by calling self.build_module with third parameter (see Time() for example) !!!
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

        self._bar = ''.join(bar)

        #return bar + '\u0002'
        #return bar

    def set_icon(self, icon=None, color='None'):
        if icon != None:
            self._icon = self.colors.get(color) + self.icons.get(icon) + '\u00a0'  # Note there's tailing space entered after the icon (\u00a0); this will be added to the module block with or without using icon;
        else:
            self._icon = ''


    def build_module(self, text=None, color='None', option='True'):
        ''' builds the final output. if 3rd arg is set for self.build_module, then leading and trailing arrows won't be added;
           if first arg (text) is None, then self._module will be empty string '''
        if text == None:
            self._module = ''
        elif option == 'True':
            self._module = str(self.colors.get(color[color.find('_')+1:]+'_black') +
                                    self.icons.get('arrow') +
                                    self._icon +
                                    self.colors.get(color) +
                                    text +
                                    self.colors.get('black_'+color[color.find('_')+1:]) +
                                    self.icons.get('arrow') +
                                    self.colors.get(color[color.find('_')+1:]+'_black')
                                )
        elif option == 'txtonly':
            self._module = text
        else:
            ''' i.e. don't add the trailing and leading arrow '''
            self._module = self.colors.get(color) + self._icon + text


class Music(Segment):
    trim_length = 30    # nr of characters to trim song info to;
    
    def __str__(self):
        return self._module
        
    def __init__(self, color, complete=True):
        Segment.__init__(self)

        self.set_icon('music', color)
        self.build_module('Not playing', color)

        try:
            cmd = run_cmd(['timeout', '1s', 'mpc'])
        except OSError:
            return

        if 'playing' in cmd:
            text = []

            [playing, info, dummy] = cmd.split('\n')
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

            if complete == True:
                self.build_module(' '.join(text), color)
            else:
                self.build_module(' '.join(text), color, 'txtonly')

class Vol(Segment):
    def __str__(self):
        return self._module
        
    def __init__(self, color):
        Segment.__init__(self)

        # Remove bar if battery info present:
        try:
            bat._bar
            no_bar = True
        except:
            no_bar = False
            
            
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
            if no_bar:
                self.build_module('', color)
            else:
                self.get_bar(0)
                self.build_module(self._bar, color)
            return

        if volume < 40:
            self.set_icon('vol_low', color)
        elif volume > 70:
            self.set_icon('vol_high', color)
        else:
            self.set_icon('vol_medium', color)
            
        if no_bar:
            self.build_module(str(volume)+'%', color)
        else:
            self.get_bar(volume)
            self.build_module(self._bar+'\u00a0'+str(volume)+'%', color)

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

        self.build_module(' / '.join(unread))


class Date(Segment):
    def __str__(self):
        return self._module
        
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon()
        self.build_module(datetime.datetime.now().strftime('%a %d'), color)


class Time(Segment):
    def __str__(self):
        return self._module
        
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('None', color)
        self.build_module(datetime.datetime.now().strftime('%R'), color, False) # 'False' bypasses the arrow addition;
        

class Mem(Segment):
    def __str__(self):
        return self._module
    
    def __init__(self, color, complete=True):
        Segment.__init__(self)

        self.set_icon('ram', color)
        
        mem=str(round(psutil.phymem_usage()[3])) + '%'
        
        if complete == True:
            self.build_module(mem, color)
        else:
            self.build_module(mem, color, 'txtonly')
            
class cpu_and_mem(Segment):
    def __str__(self):
        return self._module
    
    def __init__(self, color):
        Segment.__init__(self)

        self.set_icon('ram', color)
        
        mem=Mem(color, False)
        cpu = CPU(color, True, False)
        
        self.build_module(str(mem) + ' | ' + str(cpu), color)
        
        
class CPU(Segment):
    def __str__(self):
        return self._module
        
    def __init__(self, color, all_cpus=False, complete=True):
        Segment.__init__(self)
        
        self.set_icon('cpu', color)

        if all_cpus == True:
        #  CPU percentage per core:
            cpuloads = psutil.cpu_percent(interval=sleep_interval, percpu=True)
            for i, load in enumerate(cpuloads):
                rounded = str(round(load))
                if len(rounded) == 1:
                    cpuloads[i] = ' ' + rounded + '%'   # pad the value if only one digit
                else:
                    cpuloads[i] = rounded + '%'
                    
            # Compile the string that will be printed:
            output = ''
            for i, value in enumerate(cpuloads):
                output = output + 'CPU' + str(i) + ': ' + value + ' | '
            # remove the trailing ' | ' from last item:
            output = output[0:len(output)-2]

        else:
        #  CPU percentage overall:
            output = str(round(psutil.cpu_percent(interval=sleep_interval))) + '%'
            if len(output) == 2:
                output=' ' + output # pad the value if only one digit

        if complete == True:
            self.build_module(output, color)
        else:
            self.build_module(output, color, 'txtonly')

class bat_(Segment):
    # FYI: to simply see whether AC power is connected:   cat /sys/class/power_supply/C1F2/online
    def __str__(self):
        return self._module
        
    def __init__(self, color):
        Segment.__init__(self)

        generate_bat_vars()
        
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
            self.get_bar(bat_percentage, 18, 'round-candycane')
            self.build_module(self._bar+'\u00a0'+str(bat_percentage)+'%', color)
        elif 'Charging,' in bat_stat:
            #color='black_gray'
            self.set_icon('bat_charging', color)
            self.get_bar(bat_percentage, 18)
            self.build_module(self._bar+'\u00a0'+str(bat_percentage)+'%', color)
        #elif 'Unknown,' in bat_stat:
        #    color='urgency_gray'
        #    self.set_icon('bat_error', color)
        #    self.build_module('Battery status unknown!', color)
        else:
            #self.set_icon('AC', color)
            #self.build_module('', color)
            self.build_module(None)
            
class wx(Segment):
    # reads weather info from file
    def __str__(self):
        return self._module
        
    def __init__(self, color):
        Segment.__init__(self)

        try:
            f=open(wx_log, 'r')
            wx_info=f.read()
            f.close()
            
            # Extract temperature:
            index=wx_info.find('Temp:')
            index2=wx_info.find(' °C')
            temp=wx_info[index+7:index2] + '°C'
            #print(temp)


            try:
            # Only display wx, if battery info isn't shown, i.e. battery info essentially replaces wx info;
            # otherwise statusbar gets too long and is places partially behind systray:
                bat._bar
                self.set_icon('None', color)
                self.build_module(None)
            except:
                self.set_icon('AC', color)
                self.build_module(temp, color)
        except:
            self.set_icon('None', color)
            self.build_module(None)
            
            
class network(Segment):
    def __str__(self):
        return self._module
        
    def __init__(self, color):
        Segment.__init__(self)
        # Rx/Tx data also at  /sys/class/net/wlan0/statistics/[tx_bytes | rx_bytes]
        
        
        # WHY ARE THESE GLOBAL VAR DECLARATIONS REQUIRED, globals already exist; i should be able to use them (without editing ofc)????:
        global ntw_startTime
        global sent_old
        global recv_old
        
        nw_data=str(psutil.network_io_counters(pernic=True))
        ntw_endTime=time.time()
        timediff=float(ntw_endTime - ntw_startTime)
        #print(timediff)
        ntw_startTime=time.time()
        
        # Find which interface data to show:
        try:
            f = open(interfaces_file, 'r')
            active_line = f.readlines()[1].split()  # Loads the line containing interface labled as 'default' by network-manager;
            f.close()
            try:
                # If no connected interfaces, then [1] element isn't present.
                active = active_line[1]
                if active == 'wlan0':
                    int_data = nw_data[nw_data.find('wlan0'):nw_data.find('eth0')]
                elif active == 'eth0':
                    int_data = nw_data[nw_data.find('eth0'):]
                else:
                    # No (known) connection:
                    active = None
            except:
                active = None
        except:
            active = None



            
        if active != None:
            sent=float(int_data[int_data.find('bytes_sent=')+11:int_data.find(', bytes_recv')])
            recv=float(int_data[int_data.find('bytes_recv=')+11:int_data.find(', packets_sent')])
            
            sent_diff=float((sent - sent_old) * 8)  # Find the difference between last measure point, convert to bits
            recv_diff=float((recv - recv_old) * 8) 
            
            # Find current Tx:
            sent_current=round( ((sent_diff / 1048576) / timediff), 1) # translates into Mbit
            
            if sent_current < 1.0:
                sent_current=str(round( ((sent_diff / 1024) / timediff), 1)) + ' Kbit/s'
            elif sent_current > 1024.0:
                sent_current=str(round( ((sent_diff / 1073741824) / timediff), 2)) + ' Gbit/s'
            else:
                sent_current=str(sent_current) + ' Mbit/s'
                
            # Find current Rx:
            recv_current=round( ((recv_diff / 1048576) / timediff), 1) # translates into Mbit
            
            if recv_current < 1.0:
                recv_current=str(round( ((recv_diff / 1024) / timediff), 1)) + ' Kbit/s'
            elif recv_current > 1024.0:
                recv_current=str(round( ((recv_diff / 1073741824) / timediff), 2)) + ' Gbit/s'
            else:
                recv_current=str(recv_current) + ' Mbit/s'

            
            
            # Find total sent:
            sent_total=round(sent / 1048576, 1) # translates into MB
            
            if sent_total < 1.0:
                sent_total=str(round(sent / 1024, 1)) + ' KB'
            elif sent_total > 1024.0:
                sent_total=str(round(sent / 1073741824, 2)) + ' GB'
            else:
                sent_total=str(sent_total) + ' MB'
                
            # Find total recv:
            recv_total=round(recv / 1048576, 1) # translates into MB
            
            if recv_total < 1.0:
                recv_total=str(round(recv / 1024, 1)) + ' KB'
            elif recv_total > 1024.0:
                recv_total=str(round(recv / 1073741824, 2)) + ' GB'
            else:
                recv_total=str(recv_total) + ' MB'
                
            # Reset the variables:
            sent_old = sent
            recv_old = recv
        
        
            self.set_icon('AC', color)
            self.build_module(str(active + ':  ' + 'Rx: ' + recv_current + '  ' + recv_total + ' | ' + 'Tx: ' + sent_current + '  ' + sent_total), color)
            
        else:
            self.set_icon('AC', color)
            self.build_module('No connection', color)
   


############ START ##############
# Find usage, if cml arguments were passed:
if len(sys.argv) > 1:
    # Find current mode:
    f = open(modefile, 'r')
    mode=int(f.read())
    f.close()
    
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
    f = open(modefile, 'w')
    f.write(str(mode))
    f.close()
    
    sys.exit()
###############################

# initialize vars:
nw_data=str(psutil.network_io_counters(pernic=True))
ntw_startTime=int(time.time())

# Find which interface data to show:
try:
    f = open(interfaces_file, 'r')
    active_line = f.readlines()[1].split()
    f.close()
    if len(active_line) > 1:
        active = active_line[1]
    else:
        active = None

    if active == 'eth0':
        int_data = nw_data[nw_data.find('eth0'):]
    else:
        # For wlan0 and not connected at all (because vars have to be initialized nevertheless):
        int_data = nw_data[nw_data.find('wlan0'):nw_data.find('eth0')]
except: # meaning interfaces_file has not yet been created
    int_data = nw_data[nw_data.find('wlan0'):nw_data.find('eth0')]

sent_old=float(int_data[int_data.find('bytes_sent=')+11:int_data.find(', bytes_recv')])
recv_old=float(int_data[int_data.find('bytes_recv=')+11:int_data.find(', packets_sent')])

# Write the default mode to statusfile:
f = open(modefile, 'w')
f.write(startmode)
f.close()
mode=startmode    # Default starting status
###############################

# Start the loop:
while 1:
    if mode == '1':
        cpu = CPU('black_yellow', False) # if second parameter is True, then CPU() returns recource usage for all cores
        bat = bat_('black_gray')
        mem = Mem('black_blue')
        time_ = Time('white')
        date = Date('black_gray')
        vol = Vol('black_blue')
        music = Music('black_yellow')
        weather = wx('black_gray')

        # space: \u00a0
        #run_cmd(['xsetroot', '-name'+' "', cpu, mem, bat, music, vol, date, time_ +'        '+'"'])

        status=str(
                                str(cpu) +
                                str(mem) +
                                str(bat) +
                                str(weather) +
                                str(music) +
                                str(vol) +
                                str(date) +
                                str(time_)
                        )
                        
        # Remove bat object, as many formatting rules depend on its _bar:
        del bat
        #time.sleep(sleep_interval)  # sleep provided by the CPU class
        
    elif mode == '2':
        # Try to remove bat object, as many formatting rules depend on its _bar:
        #try:
        #   del bat
        #except:
        #   pass
            
        time_ = Time('white')
        date = Date('black_gray')
        vol = Vol('black_blue')
        ntw = network('black_yellow')
        weather = wx('black_gray')
        cam = cpu_and_mem('black_yellow')
        
        status=str(
                                str(cam)
                        )
                        
        time.sleep(sleep_interval)
    else:
        # Script exits on unsupported mode
        sys.exit()
    
    # Finally, set the root window:
    os.system('xsetroot -name "' + status + '        "')   # trailing spaces are because of the systray;
    mode=find_mode()



