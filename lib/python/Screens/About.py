# Embedded file name: About.py
from Screen import Screen
from Screens.MessageBox import MessageBox
from Components.config import config
from Components.ActionMap import ActionMap
from Components.Sources.StaticText import StaticText
from Components.Harddisk import harddiskmanager
from Components.NimManager import nimmanager
from Components.About import about
from Components.ScrollLabel import ScrollLabel
from Components.Button import Button
from Components.Label import Label
from Components.ProgressBar import ProgressBar
from Tools.StbHardware import getFPVersion
from Tools.Directories import fileExists
from enigma import eTimer, eLabel, eConsoleAppContainer
from Screens.Console import Console
from Components.HTMLComponent import HTMLComponent
from Components.GUIComponent import GUIComponent
import skin, os
import urllib

class About(Screen):

    def __init__(self, session):
        Screen.__init__(self, session)
        self.setTitle(_('About'))
        hddsplit = skin.parameters.get('AboutHddSplit', 0)
        AboutText = _('Hardware: ') + about.getHardwareTypeString() + '\n'
        AboutText += _('CPU: ') + about.getCPUInfoString() + '\n'
        AboutText += _('Image: ') + about.getImageTypeString() + '\n'
        AboutText += _('Installed: ') + about.getFlashDateString() + '\n'
        AboutText += _('Kernel version: ') + about.getKernelVersionString() + '\n'
        EnigmaVersion = 'Enigma: ' + about.getEnigmaVersionString()
        self['EnigmaVersion'] = StaticText(EnigmaVersion)
        AboutText += EnigmaVersion + '\n'
        AboutText += _('Enigma (re)starts: %d\n') % config.misc.startCounter.value
        GStreamerVersion = 'GStreamer: ' + about.getGStreamerVersionString().replace('GStreamer', '')
        self['GStreamerVersion'] = StaticText(GStreamerVersion)
        AboutText += GStreamerVersion + '\n'
        ImageVersion = _('Last upgrade: ') + about.getImageVersionString()
        self['ImageVersion'] = StaticText(ImageVersion)
        AboutText += ImageVersion + '\n'
        AboutText += _('DVB drivers: ') + about.getDriverInstalledDate() + '\n'
        AboutText += _('Python version: ') + about.getPythonVersionString() + '\n'
        fp_version = getFPVersion()
        if fp_version is None:
            fp_version = ''
        else:
            fp_version = _('Frontprocessor version: %s') % fp_version
            AboutText += fp_version + '\n'
        self['FPVersion'] = StaticText(fp_version)
        self['TunerHeader'] = StaticText(_('Detected NIMs:'))
        AboutText += '\n' + _('Detected NIMs:') + '\n'
        nims = nimmanager.nimList(showFBCTuners=False)
        for count in range(len(nims)):
            if count < 4:
                self['Tuner' + str(count)] = StaticText(nims[count])
            else:
                self['Tuner' + str(count)] = StaticText('')
            AboutText += nims[count] + '\n'

        self['HDDHeader'] = StaticText(_('Detected HDD:'))
        AboutText += '\n' + _('Detected HDD:') + '\n'
        hddlist = harddiskmanager.HDDList()
        hddinfo = ''
        if hddlist:
            formatstring = hddsplit and '%s:%s, %.1f %sB %s' or '%s\n(%s, %.1f %sB %s)'
            for count in range(len(hddlist)):
                if hddinfo:
                    hddinfo += '\n'
                hdd = hddlist[count][1]
                if int(hdd.free()) > 1024:
                    hddinfo += formatstring % (hdd.model(),
                     hdd.capacity(),
                     hdd.free() / 1024.0,
                     'G',
                     _('free'))
                else:
                    hddinfo += formatstring % (hdd.model(),
                     hdd.capacity(),
                     hdd.free(),
                     'M',
                     _('free'))

        else:
            hddinfo = _('none')
        self['hddA'] = StaticText(hddinfo)
        AboutText += hddinfo + '\n\n' + _('Network Info:')
        for x in about.GetIPsFromNetworkInterfaces():
            AboutText += '\n' + x[0] + ': ' + x[1]

        self['AboutScrollLabel'] = ScrollLabel(AboutText)
        self['key_green'] = Button(_('Space on partitions'))
        self['key_red'] = Button(_('System processes'))
        self['key_yellow'] = Button(_('Network interfaces'))
        self['key_blue'] = Button(_('Memory Info'))
        self['actions'] = ActionMap(['ColorActions', 'SetupActions', 'DirectionActions'], {'cancel': self.close,
         'ok': self.close,
         'red': self.runps,
         'green': self.rundf,
         'yellow': self.runifconfig,
         'blue': self.showMemoryInfo,
         'up': self['AboutScrollLabel'].pageUp,
         'down': self['AboutScrollLabel'].pageDown})
        return

    def showTranslationInfo(self):
        self.session.open(TranslationInfo)

    def showCommits(self):
        self.session.open(CommitInfo)

    def showMemoryInfo(self):
        self.session.open(MemoryInfo)

    def showTroubleshoot(self):
        self.session.open(Troubleshoot)

    def runps(self):
        runlist = []
        runlist.append('ps')
        self.session.open(Console, title=_('System processes'), cmdlist=runlist)

    def rundf(self):
        runlist = []
        runlist.append('df -h')
        self.session.open(Console, title=_('Space on partitions'), cmdlist=runlist)

    def runifconfig(self):
        runlist = []
        runlist.append('ifconfig')
        self.session.open(Console, title=_('Network interfaces'), cmdlist=runlist)


class TranslationInfo(Screen):

    def __init__(self, session):
        Screen.__init__(self, session)
        self.setTitle(_('Translation'))
        info = _('TRANSLATOR_INFO')
        if info == 'TRANSLATOR_INFO':
            info = '(N/A)'
        infolines = _('').split('\n')
        infomap = {}
        for x in infolines:
            l = x.split(': ')
            if len(l) != 2:
                continue
            type, value = l
            infomap[type] = value

        print infomap
        self['key_red'] = Button(_('Cancel'))
        self['TranslationInfo'] = StaticText(info)
        translator_name = infomap.get('Language-Team', 'none')
        if translator_name == 'none':
            translator_name = infomap.get('Last-Translator', '')
        self['TranslatorName'] = StaticText(translator_name)
        self['actions'] = ActionMap(['SetupActions'], {'cancel': self.close,
         'ok': self.close})


class CommitInfo(Screen):

    def __init__(self, session):
        Screen.__init__(self, session)
        self.setTitle(_('Latest Commits'))
        self.skinName = ['Console']
        self['text'] = ScrollLabel('')
        self['actions'] = ActionMap(['WizardActions', 'DirectionActions'], {'ok': self.cancel,
         'back': self.cancel,
         'up': self['text'].pageUp,
         'down': self['text'].pageDown}, -1)
        self.viewtext()

    def cancel(self):
        self.close()

    def viewtext(self):
        try:
            txt = urllib.urlopen('http://graterlia.xunil.pl/changes/lista_zmian').read()
        except:
            txt = _('Cannot get data, the connection to the server (xunil.pl) failed - please try later again')

        self['text'].setText(txt)


class ViewLicense(Screen):

    def __init__(self, session):
        self.session = session
        Screen.__init__(self, session)
        self.setTitle(_('Graterlia OS License'))
        self.skinName = ['Console']
        self['text'] = ScrollLabel('')
        self['actions'] = ActionMap(['WizardActions', 'DirectionActions'], {'ok': self.cancel,
         'back': self.cancel,
         'up': self['text'].pageUp,
         'down': self['text'].pageDown}, -1)
        self.viewtext()

    def cancel(self):
        self.close()

    def viewtext(self):
        list = ''
        for line in open('/etc/license'):
            list += line

        self['text'].setText(list)


class ViewInformation(Screen):

    def __init__(self, session):
        self.session = session
        Screen.__init__(self, session)
        self.setTitle(_('Graterlia OS Information'))
        self.skinName = ['Console']
        self['text'] = ScrollLabel('')
        self['actions'] = ActionMap(['WizardActions', 'DirectionActions'], {'ok': self.cancel,
         'back': self.cancel,
         'up': self['text'].pageUp,
         'down': self['text'].pageDown}, -1)
        self.viewtext()

    def cancel(self):
        self.close()

    def viewtext(self):
        try:
            txt = urllib.urlopen('http://graterlia.xunil.pl/changes/informacje').read()
        except:
            txt = _('Cannot get data, the connection to the server (xunil.pl) failed - please try later again')

        self['text'].setText(txt)


class ViewDonate(Screen):

    def __init__(self, session):
        self.session = session
        Screen.__init__(self, session)
        self.setTitle(_('Graterlia OS Donate'))
        self.skinName = ['Console']
        self['text'] = ScrollLabel('')
        self['actions'] = ActionMap(['WizardActions', 'DirectionActions'], {'ok': self.cancel,
         'back': self.cancel,
         'up': self['text'].pageUp,
         'down': self['text'].pageDown}, -1)
        self.viewtext()

    def cancel(self):
        self.close()

    def viewtext(self):
        txt = _("Graterlia - jak pom\xc3\xb3c w jej rozwoju?\n-------------------------------------------------------------------------------------------------------------------------\n\nJeszcze przed 2013 rokiem ma\xc5\x82o kto s\xc4\x85dzi\xc5\x82, i\xc5\xbc jest mo\xc5\xbcliwe stworzenie takiego softu na tunery.\nTo, co uwa\xc5\xbcano za niemo\xc5\xbcliwe, dzi\xc5\x9b jest faktem.\n\n\n,Zawsze wydaje si\xc4\x99, \xc5\xbce co\xc5\x9b jest niemo\xc5\xbcliwe, dop\xc3\xb3ki nie zostanie to zrobione' Nelson Mandela.\n\n\nGraterlia powsta\xc5\x82a w spos\xc3\xb3b spontaniczny.\nZaj\xc4\x99li si\xc4\x99 ni\xc4\x85 ludzie bezinteresowni, kt\xc3\xb3rych g\xc5\x82\xc3\xb3wnym celem by\xc5\x82o i jest tworzenie oraz utrzymanie dobrego \nsystemu operacyjnego na dekodery.\n\n\nJednak wiadomo, \xc5\xbce aby co\xc5\x9b si\xc4\x99 utrzyma\xc5\x82o i rozwija\xc5\x82o dalej, potrzebne jest systematyczne wsparcie, \nzw\xc5\x82aszcza \xc5\xbce ekipa od Graterlii nie czerpie \xc5\xbcadnego zysku (i nawet nie ma zamiaru!) z istnienia Graterlii,\nnawet nie my\xc5\x9bli o wprowadzaniu reklam na stron\xc4\x99 czy forum, gdy\xc5\xbc wychodzi z za\xc5\x82o\xc5\xbcenia, i\xc5\xbc u\xc5\xbcytkownika nale\xc5\xbcy ceni\xc4\x87.\nJe\xc5\x9bli spodoba\xc5\x82a Ci si\xc4\x99 Graterlia, istnieje spos\xc3\xb3b jej wspierania poprzez PayPal.\n\nZaloguj si\xc4\x99 na swoje konto PayPal, wpisz nasz adres (nbox@xunil.pl) a potem kwot\xc4\x99 jak\xc4\x85 chcesz udzieli\xc4\x87 wsparcia.\n\nDzi\xc4\x99kujemy, \xc5\xbce jeste\xc5\x9bcie z nami,\n- zesp\xc3\xb3\xc5\x82 od Graterlii.")
        self['text'].setText(txt)


class MemoryInfo(Screen):

    def __init__(self, session):
        Screen.__init__(self, session)
        self['actions'] = ActionMap(['SetupActions', 'ColorActions'], {'cancel': self.close,
         'ok': self.getMemoryInfo,
         'green': self.getMemoryInfo,
         'blue': self.clearMemory})
        self['key_red'] = Label(_('Cancel'))
        self['key_green'] = Label(_('Refresh'))
        self['key_blue'] = Label(_('Clear'))
        self['lmemtext'] = Label()
        self['lmemvalue'] = Label()
        self['rmemtext'] = Label()
        self['rmemvalue'] = Label()
        self['pfree'] = Label()
        self['pused'] = Label()
        self['slide'] = ProgressBar()
        self['slide'].setValue(100)
        self['params'] = MemoryInfoSkinParams()
        self['info'] = Label(_("This info is for developers only.\nFor a normal users it is not relevant.\nDon't panic please when you see values being displayed that you think look suspicious!"))
        self.setTitle(_('Memory Info'))
        self.onLayoutFinish.append(self.getMemoryInfo)

    def getMemoryInfo(self):
        try:
            ltext = rtext = ''
            lvalue = rvalue = ''
            mem = 1
            free = 0
            rows_in_column = self['params'].rows_in_column
            for i, line in enumerate(open('/proc/meminfo', 'r')):
                s = line.strip().split(None, 2)
                if len(s) == 3:
                    name, size, units = s
                elif len(s) == 2:
                    name, size = s
                    units = ''
                else:
                    continue
                if name.startswith('MemTotal'):
                    mem = int(size)
                if name.startswith('MemFree') or name.startswith('Buffers') or name.startswith('Cached'):
                    free += int(size)
                if i < rows_in_column:
                    ltext += ''.join((name, '\n'))
                    lvalue += ''.join((size,
                     ' ',
                     units,
                     '\n'))
                else:
                    rtext += ''.join((name, '\n'))
                    rvalue += ''.join((size,
                     ' ',
                     units,
                     '\n'))

            self['lmemtext'].setText(ltext)
            self['lmemvalue'].setText(lvalue)
            self['rmemtext'].setText(rtext)
            self['rmemvalue'].setText(rvalue)
            self['slide'].setValue(int(100.0 * (mem - free) / mem + 0.25))
            self['pfree'].setText('%.1f %s' % (100.0 * free / mem, '%'))
            self['pused'].setText('%.1f %s' % (100.0 * (mem - free) / mem, '%'))
        except Exception as e:
            print '[About] getMemoryInfo FAIL:', e

        return

    def clearMemory(self):
        eConsoleAppContainer().execute('sync')
        open('/proc/sys/vm/drop_caches', 'w').write('3')
        self.getMemoryInfo()


class MemoryInfoSkinParams(HTMLComponent, GUIComponent):

    def __init__(self):
        GUIComponent.__init__(self)
        self.rows_in_column = 25

    def applySkin(self, desktop, screen):
        if self.skinAttributes is not None:
            attribs = []
            for attrib, value in self.skinAttributes:
                if attrib == 'rowsincolumn':
                    self.rows_in_column = int(value)

            self.skinAttributes = attribs
        return GUIComponent.applySkin(self, desktop, screen)

    GUI_WIDGET = eLabel


class Troubleshoot(Screen):

    def __init__(self, session):
        Screen.__init__(self, session)
        self.setTitle(_('Troubleshoot'))
        self['AboutScrollLabel'] = ScrollLabel(_('Please wait'))
        self['key_red'] = Button()
        self['key_green'] = Button()
        self['actions'] = ActionMap(['OkCancelActions', 'DirectionActions', 'ColorActions'], {'cancel': self.close,
         'up': self['AboutScrollLabel'].pageUp,
         'down': self['AboutScrollLabel'].pageDown,
         'left': self.left,
         'right': self.right,
         'red': self.red,
         'green': self.green})
        self.container = eConsoleAppContainer()
        self.container.appClosed.append(self.appClosed)
        self.container.dataAvail.append(self.dataAvail)
        self.commandIndex = 0
        self.updateOptions()
        self.onLayoutFinish.append(self.run_console)

    def left(self):
        self.commandIndex = (self.commandIndex - 1) % len(self.commands)
        self.updateKeys()
        self.run_console()

    def right(self):
        self.commandIndex = (self.commandIndex + 1) % len(self.commands)
        self.updateKeys()
        self.run_console()

    def red(self):
        if self.commandIndex >= self.numberOfCommands:
            self.session.openWithCallback(self.removeAllLogfiles, MessageBox, _('Do you want to remove all the crahs logfiles'), default=False)
        else:
            self.close()

    def green(self):
        if self.commandIndex >= self.numberOfCommands:
            try:
                os.remove(self.commands[self.commandIndex][4:])
            except:
                pass

            self.updateOptions()
        self.run_console()

    def removeAllLogfiles(self, answer):
        if answer:
            for fileName in self.getLogFilesList():
                try:
                    os.remove(fileName)
                except:
                    pass

            self.updateOptions()
            self.run_console()

    def appClosed(self, retval):
        if retval:
            self['AboutScrollLabel'].setText(_('Some error occured - Please try later'))

    def dataAvail(self, data):
        self['AboutScrollLabel'].appendText(data)

    def run_console(self):
        self['AboutScrollLabel'].setText('')
        self.setTitle('%s - %s' % (_('Troubleshoot'), self.titles[self.commandIndex]))
        command = self.commands[self.commandIndex]
        if command.startswith('cat '):
            try:
                self['AboutScrollLabel'].setText(open(command[4:], 'r').read())
            except:
                self['AboutScrollLabel'].setText(_('Logfile does not exist anymore - Use the arrows to the left or right to select another log'))

        else:
            try:
                if self.container.execute(command):
                    raise Exception, 'failed to execute: ', command
            except Exception as e:
                self['AboutScrollLabel'].setText('%s\n%s' % (_('Some error occured - Please try later'), e))

    def cancel(self):
        self.container.appClosed.remove(self.appClosed)
        self.container.dataAvail.remove(self.dataAvail)
        self.container = None
        self.close()
        return

    def getLogFilesList(self):
        import glob
        home_root = '/hdd/enigma2_crash.log'
        tmp = '/tmp/enigma2_crash.log'
        return [ x for x in sorted(glob.glob('/mnt/hdd/*.log'), key=lambda x: os.path.isfile(x) and os.path.getmtime(x)) ] + (os.path.isfile(home_root) and [home_root] or []) + (os.path.isfile(tmp) and [tmp] or [])

    def updateOptions(self):
        self.titles = ['cat ']
        self.commands = ['cat ']
        install_log = '/var/log/autoinstall.log'
        if os.path.isfile(install_log):
            self.titles.append('%s' % install_log)
            self.commands.append('cat %s' % install_log)
        self.numberOfCommands = len(self.commands)
        fileNames = self.getLogFilesList()
        if fileNames:
            totalNumberOfLogfiles = len(fileNames)
            logfileCounter = 1
            for fileName in reversed(fileNames):
                self.titles.append('logfile %s (%s/%s)' % (fileName, logfileCounter, totalNumberOfLogfiles))
                self.commands.append('cat %s' % fileName)
                logfileCounter += 1

        self.commandIndex = min(len(self.commands) - 1, self.commandIndex)
        self.updateKeys()

    def updateKeys(self):
        self['key_red'].setText(_('Cancel') if self.commandIndex > self.numberOfCommands else _('Remove all logfiles'))
        self['key_green'].setText(_('Refresh') if self.commandIndex > self.numberOfCommands else _('Remove this logfile'))