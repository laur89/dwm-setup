#!/usr/bin/env python3.2
# py3 !
# Requires psutil module for memory and cpu usage retrieval:
#   apt-get install python3-pip
#   pip-3 install psutil

import datetime
import os
import re
import subprocess
import urllib.request
import xml.dom.minidom
import psutil
import time
import string

def run_cmd(cmd):
    return subprocess.Popen(cmd, stdout=subprocess.PIPE).communicate()[0].decode("utf-8").strip()


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
    }
    bars = {
        'round-regular': '\uead0\uead1\uead2\uead3\uead4\uead5\uead6',
        'round-candycane': '\uead0\uead1\uead2\uead7\uead8\uead9\ueada',
    }
    colors = {
        'normal': '\x01',
        'urgent': '\x03',
        'error': '\x04',
        'white': '\x07',
        'warning': '\x08',
        'b_txt_y_bg': '\x09',
        'y_txt_b_gb': '\x0C',
        'blu_txt_b_bg': '\x0D',
        'b_txt_blu_bg': '\x0A',
        'gry_txt_b_bg': '\x0E',
        'b_txt_gry_bg': '\x0B',
        'urg_txt_gry_bg': '\x0F',
        'org_txt_gry_bg': '\x10',
        'None': '',
    }

        
    def get_bar(self, percent, length=10, text='', type='round-candycane'):
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

        return bar + '\u0002' + text
        #return bar  #+'\u0002'

    def set_icon(self, icon, color='None'):

        if icon == None:
            self._icon = ''
        else:
            self._icon = self.colors.get(color) + self.icons.get(icon) + '\u00a0'


    def set_text(self, text, color='None'):
        if text == None:
            self._text = ''
        else:
            self._text = self.colors.get(color) + text #+ self.colors.get('normal')



class Music(Segment):
    trim_length = 30
    
    def __str__(self):
        return str(self._icon + self._text)
        
    def __init__(self):
        Segment.__init__(self)

        self.set_icon('music', 'b_txt_y_bg')
        self.set_text('Not playing', 'b_txt_y_bg')

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

            self.set_text(' '.join(text), 'b_txt_y_bg')


class Vol(Segment):
    def __str__(self):
        return str(self._icon + self._text)
        
    def __init__(self):
        Segment.__init__(self)

        try:
            cmd = run_cmd(['amixer', 'get', 'Master'])
        except OSError:
            return

        try:
            volume = int(re.search('\[(\d+)%\]', cmd).group(1))
            muted = re.search('\[(on|off)\]', cmd).group(1) == 'off'
        except AttributeError:
            self.set_icon('vol_mute')
            self.set_text('No sound')

            return

        if muted:
            self.set_icon('vol_mute', 'b_txt_blu_bg')
            self.set_text(self.get_bar(0))
            return

        if volume < 10:
            self.set_icon('vol_low', 'b_txt_blu_bg')
        elif volume > 70:
            self.set_icon('vol_high', 'b_txt_blu_bg')
        else:
            self.set_icon('vol_medium', 'b_txt_blu_bg')

        self.set_text(self.get_bar(volume), 'b_txt_blu_bg')

class MailSegment(Segment):
    def __init__(self):
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
        return str(self._icon + self._text)
    def __init__(self):
        Segment.__init__(self)

        self.set_icon(None, 'b_txt_gry_bg')
        self.set_text(datetime.datetime.now().strftime('%a %d'), 'b_txt_gry_bg')


class Time(Segment):
    def __str__(self):
        return str(self._icon + self._text)
    def __init__(self):
        Segment.__init__(self)

        self.set_icon(None, 'white')
        self.set_text(datetime.datetime.now().strftime('%R'), 'white')
        
        

class Mem(Segment):
    def __str__(self):
        return str(self._icon + self._text)
    
    def __init__(self):
        Segment.__init__(self)

        self.set_icon('ram', 'b_txt_blu_bg')
        
        mem=str(round(psutil.phymem_usage()[3]))
        self.set_text(mem + '%', 'b_txt_blu_bg')
        
class CPU(Segment):
    def __str__(self):
        return self._icon + self._text
        
    def __init__(self):
        Segment.__init__(self)
        self.set_icon('cpu', 'b_txt_y_bg')

        #  CPU percentage per core:
        #cpuloads = psutil.cpu_percent(interval=1, percpu=True)
        #cpu1 = str(round(cpuloads[0])) + '%'
        #cpu2 = str(round(cpuloads[1])) + '%'
        #self.set_text(cpu1 + ' | ' + cpu2, 'b_txt_y_bg')
        
        #  CPU percentage overall:
        cpuloads = str(round(psutil.cpu_percent(interval=1)))
        if len(cpuloads) == 1:
            cpuloads=' ' + cpuloads
        self.set_text(cpuloads+'%', 'b_txt_y_bg')

class bat_(Segment):
    # FYI: to simply see whether AC power is connected:   cat /sys/class/power_supply/AC/online
    def __str__(self):
        return self._icon + self._text
        
    def __init__(self):
        Segment.__init__(self)

        if 'Discharging,' in bat_stat:
            if percentage >= 50:
                colr='b_txt_gry_bg'
                self.set_icon('bat_full', colr)
            elif percentage <= 10:
                colr='urg_txt_gry_bg'
                self.set_icon('bat_empty', colr)
            else:
                colr='org_txt_gry_bg'
                self.set_icon('bat_medium', colr)
            self.set_text(self.get_bar(percentage, 18, '\x0B '+str(percentage)+'%'), colr)
        elif 'Charging,' in bat_stat:
            colr='b_txt_gry_bg'
            self.set_icon('bat_charging', colr)
            self.set_text(self.get_bar(percentage, 18, '\x0B '+str(percentage)+'%'), colr)
        #elif 'Unknown,' in bat_stat:
        #    colr='urg_txt_gry_bg'
        #    self.set_icon('bat_error', colr)
        #    self.set_text('Battery status unknown!', colr)
        else:
            self.set_icon('AC', 'b_txt_gry_bg')
            self.set_text('')
            
class network(Segment):
    # FYI: to simply see whether AC power is connected:   cat /sys/class/power_supply/AC/online
    def __str__(self):
        return self._icon + self._text
        
    def __init__(self):
        Segment.__init__(self)
        
        nw_data=str(psutil.network_io_counters(pernic=True))
        nw_wlan=nw_data[nw_data.find('wlan0'):nw_data.find('eth0')]
        nw_eth=nw_data[nw_data.find('eth0'):]
        
        wlan_sent=nw_wlan[nw_wlan.find('bytes_sent=')+11:nw_wlan.find(', bytes_recv')]
        wlan_recv=nw_wlan[nw_wlan.find('bytes_recv=')+11:nw_wlan.find(', packets_sent')]
        
        wlan_sent_display=round(int(wlan_sent) / 1048576, 1) # translates into MB
        
        if wlan_sent_display < 1.0:
            wlan_sent_display=str(round(int(wlan_sent) / 1024, 1)) + ' KB'
        elif wlan_sent_display > 1024.0:
            wlan_sent_display=str(round(int(wlan_sent) / 1073741824, 2)) + ' GB'
        else:
            wlan_sent_display=str(wlan_sent_display) + ' MB'
        
        
        if 'Discharging,' in bat_stat:
            if percentage >= 50:
                colr='b_txt_gry_bg'
                self.set_icon('bat_full', colr)
            elif percentage <= 10:
                colr='urg_txt_gry_bg'
                self.set_icon('bat_empty', colr)
            else:
                colr='org_txt_gry_bg'
                self.set_icon('bat_medium', colr)
            self.set_text(self.get_bar(percentage, 18, '\x0B '+str(percentage)+'%'), colr)
        elif 'Charging,' in bat_stat:
            colr='b_txt_gry_bg'
            self.set_icon('bat_charging', colr)
            self.set_text(self.get_bar(percentage, 18, '\x0B '+str(percentage)+'%'), colr)
        #elif 'Unknown,' in bat_stat:
        #    colr='urg_txt_gry_bg'
        #    self.set_icon('bat_error', colr)
        #    self.set_text('Battery status unknown!', colr)
        else:
            self.set_icon('AC', 'b_txt_gry_bg')
            self.set_text('')

#while 1:
#def bat_stat():
''' Returns status of the battery; first arg is -1 for discharging, 1 for charging
or 0 for AC power, and bat percentage as second arg '''
bat = run_cmd(['acpi', '-b'])

bat=bat.split(' ')
per=bat[3]
bat_stat=bat[2]

percentage=''

for letter in per:
    if letter not in string.punctuation:
        percentage += letter
percentage=int(percentage)



bat = str(bat_())
cpu = str(CPU())
mem = str(Mem())
time = str(Time())
date = str(Date())
vol = str(Vol())
music = str(Music())



# tühik: \u00a0
command=str('xsetroot -name "'+ '\x0C\u1e00' + cpu+ '\x09\u1e00\x0D\u1e00' + mem+ '\x0A\u1e00\x0E\u1e00' + bat +
'\x0B\u1e00\x0C\u1e00' + music+'\x09\u1e00\x0D\u1e00'+ vol+'\x0A\u1e00\x0E\u1e00' + date + '\x0B\u1e00\x07\u00a0'  + time + '        '+'"')
os.system(command)
    #time.sleep(1)
