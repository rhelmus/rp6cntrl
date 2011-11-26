#!/usr/bin/env python

import argparse
import errno
import glob
import os
import shutil
import subprocess
import tarfile
import zipfile

distFiles = [
'avrglue/avrglue.config',
'avrglue/avrglue.creator',
'avrglue/avrglue.files',
'avrglue/avrglue.includes',
'avrglue/glue.cpp',
'avrglue/glue.h',
'avrglue/ioregisters.cpp',
'avrglue/ioregisters.h',
'avrglue/itoa.cpp',
'avrglue/SConstruct',
'COPYING',
'lua/lapi.c',
'lua/lapi.h',
'lua/lauxlib.c',
'lua/lauxlib.h',
'lua/lbaselib.c',
'lua/lcode.c',
'lua/lcode.h',
'lua/ldblib.c',
'lua/ldebug.c',
'lua/ldebug.h',
'lua/ldo.c',
'lua/ldo.h',
'lua/ldump.c',
'lua/lfunc.c',
'lua/lfunc.h',
'lua/lgc.c',
'lua/lgc.h',
'lua/linit.c',
'lua/liolib.c',
'lua/llex.c',
'lua/llex.h',
'lua/llimits.h',
'lua/lmathlib.c',
'lua/lmem.c',
'lua/lmem.h',
'lua/loadlib.c',
'lua/lobject.c',
'lua/lobject.h',
'lua/lopcodes.c',
'lua/lopcodes.h',
'lua/loslib.c',
'lua/lparser.c',
'lua/lparser.h',
'lua/lstate.c',
'lua/lstate.h',
'lua/lstring.c',
'lua/lstring.h',
'lua/lstrlib.c',
'lua/ltable.c',
'lua/ltable.h',
'lua/ltablib.c',
'lua/ltm.c',
'lua/ltm.h',
'lua/luaconf.h',
'lua/lua.h',
'lua/lua.hpp',
'lua/lualib.h',
'lua/lundump.c',
'lua/lundump.h',
'lua/lvm.c',
'lua/lvm.h',
'lua/lzio.c',
'lua/lzio.h',
'printfiles.sh',
'qextserialport/qextserialenumerator.cpp',
'qextserialport/qextserialenumerator.h',
'qextserialport/qextserialenumerator_osx.cpp',
'qextserialport/qextserialenumerator_p.h',
'qextserialport/qextserialenumerator_unix.cpp',
'qextserialport/qextserialenumerator_win.cpp',
'qextserialport/qextserialport.cpp',
'qextserialport/qextserialport_global.h',
'qextserialport/qextserialport.h',
'qextserialport/qextserialport_p.h',
'qextserialport/qextserialport.pri',
'qextserialport/qextserialport_unix.cpp',
'qextserialport/qextserialport_win.cpp',
'qextserialport/qextwineventnotifier_p.cpp',
'qextserialport/qextwineventnotifier_p.h',
'qextserialport/README',
'release.py',
'resource/cardboard-box.png',
'resource/cardboard-box.svg',
'resource/clapping-hands.png',
'resource/clapping-hands.wav',
'resource/clear.png',
'resource/configure.png',
'resource/connect-serial.png',
'resource/dataplot.png',
'resource/delete.png',
'resource/edit-map.png',
'resource/filter.png',
'resource/floor.jpg',
'resource/follow.png',
'resource/grid-icon.png',
'resource/grid-icon.svg',
'resource/light-add.png',
'resource/light-add.svg',
'resource/light-mode.png',
'resource/light-mode.svg',
'resource/light-refresh.png',
'resource/light-refresh.svg',
'resource/m32-top.png',
'resource/m32-top.svg',
'resource/mouse-arrow.png',
'resource/reset.png',
'resource/robot-start.png',
'resource/robot-start.svg',
'resource/rotate.png',
'resource/rp6-top.png',
'resource/rp6-top.svg',
'resource/run.png',
'resource/sources.txt',
'resource/stop.png',
'resource/tool.png',
'resource/viewmag_.png',
'resource/viewmag+.png',
'resource/wall.jpg',
'SDL/include/SDL/begin_code.h',
'SDL/include/SDL/close_code.h',
'SDL/include/SDL/doxyfile',
'SDL/include/SDL/SDL_active.h',
'SDL/include/SDL/SDL_audio.h',
'SDL/include/SDL/SDL_byteorder.h',
'SDL/include/SDL/SDL_cdrom.h',
'SDL/include/SDL/SDL_config.h',
'SDL/include/SDL/SDL_copying.h',
'SDL/include/SDL/SDL_cpuinfo.h',
'SDL/include/SDL/SDL_endian.h',
'SDL/include/SDL/SDL_error.h',
'SDL/include/SDL/SDL_events.h',
'SDL/include/SDL/SDL_getenv.h',
'SDL/include/SDL/SDL.h',
'SDL/include/SDL/SDL_joystick.h',
'SDL/include/SDL/SDL_keyboard.h',
'SDL/include/SDL/SDL_keysym.h',
'SDL/include/SDL/SDL_loadso.h',
'SDL/include/SDL/SDL_main.h',
'SDL/include/SDL/SDL_mouse.h',
'SDL/include/SDL/SDL_mutex.h',
'SDL/include/SDL/SDL_name.h',
'SDL/include/SDL/SDL_opengl.h',
'SDL/include/SDL/SDL_platform.h',
'SDL/include/SDL/SDL_quit.h',
'SDL/include/SDL/SDL_rwops.h',
'SDL/include/SDL/SDL_stdinc.h',
'SDL/include/SDL/SDL_syswm.h',
'SDL/include/SDL/SDL_thread.h',
'SDL/include/SDL/SDL_timer.h',
'SDL/include/SDL/SDL_types.h',
'SDL/include/SDL/SDL_version.h',
'SDL/include/SDL/SDL_video.h',
'SDL/lib/SDL.dll',
'SDL/README-SDL.txt',
'shared/shared.h',
'src/avrtimer.cpp',
'src/avrtimer.h',
'src/basegraphicsitem.cpp',
'src/basegraphicsitem.h',
'src/bumper.h',
'src/clock.cpp',
'src/clock.h',
'src/dataplotwidget.cpp',
'src/dataplotwidget.h',
'src/graphicsitemtypes.h',
'src/handlegraphicsitem.cpp',
'src/handlegraphicsitem.h',
'src/irsensor.h',
'src/led.h',
'src/lightgraphicsitem.cpp',
'src/lightgraphicsitem.h',
'src/lightsensor.h',
'src/lua.cpp',
'src/lua.h',
'src/lua/m32/drivers/exteeprom.lua',
'src/lua/m32/drivers/extint1.lua',
'src/lua/m32/drivers/keypad.lua',
'src/lua/m32/drivers/led.lua',
'src/lua/m32/drivers/mic.lua',
'src/lua/m32/drivers/piezo.lua',
'src/lua/m32/drivers/spi.lua',
'src/lua/m32/m32.lua',
'src/lua/m32/properties.lua',
'src/lua/main.lua',
'src/lua/robot/drivers/acs.lua',
'src/lua/robot/drivers/bumper.lua',
'src/lua/robot/drivers/extint1.lua',
'src/lua/robot/drivers/ircomm.lua',
'src/lua/robot/drivers/led.lua',
'src/lua/robot/drivers/light.lua',
'src/lua/robot/drivers/motor.lua',
'src/lua/robot/properties.lua',
'src/lua/robot/robot.lua',
'src/lua/shared_drivers/adc.lua',
'src/lua/shared_drivers/portlog.lua',
'src/lua/shared_drivers/timer0.lua',
'src/lua/shared_drivers/timer1.lua',
'src/lua/shared_drivers/timer2.lua',
'src/lua/shared_drivers/twi.lua',
'src/lua/shared_drivers/uart.lua',
'src/lua/utils.lua',
'src/main.cpp',
'src/mapsettingsdialog.cpp',
'src/mapsettingsdialog.h',
'src/pathinput.cpp',
'src/pathinput.h',
'src/pluginthread.cpp',
'src/pluginthread.h',
'src/preferencesdialog.cpp',
'src/preferencesdialog.h',
'src/progressdialog.cpp',
'src/progressdialog.h',
'src/projectsettings.cpp',
'src/projectsettings.h',
'src/projectwizard.cpp',
'src/projectwizard.h',
'src/resizablepixmapgraphicsitem.cpp',
'src/resizablepixmapgraphicsitem.h',
'src/robotgraphicsitem.cpp',
'src/robotgraphicsitem.h',
'src/robotscene.cpp',
'src/robotscene.h',
'src/robotwidget.cpp',
'src/robotwidget.h',
'src/rotatablepixmapgraphicsitem.cpp',
'src/rotatablepixmapgraphicsitem.h',
'src/rp6simul.cpp',
'src/rp6simul.h',
'src/rp6simul.pro',
'src/simulator.cpp',
'src/simulator.h',
'src/TODO.txt',
'src/utils.cpp',
'src/utils.h',
]

def mkdirOpt(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST:
            pass
        else:
            raise

parser = argparse.ArgumentParser(description='Simple release tool for rp6simul.')
parser.add_argument('action', choices=['zip', 'tar', 'src-zip', 'src-tar', 'nixstaller', 'winstaller'],
                    help='Action to perform.', nargs='*')

args = parser.parse_args()
action = args.action

baseDir = os.path.join('release', '.tmpdst')

print 'Installing to temporary prefix...'

if os.name == 'nt':
    make = 'mingw32-make'
else:
    make = 'make'

print subprocess.check_output('cd src && qmake PREFIX=../{0} && {1} -j4 && {1} install'
    .format(baseDir, make), shell=True)

mkdirOpt('release')

if 'tar' in action or 'zip' in action:
    tfile = None
    zfile = None

    if 'tar' in action:
        tfile = tarfile.open(os.path.join('release', 'rp6simul-bin.tar.gz'), 'w:gz')
    if 'zip' in action:
        zfile = zipfile.ZipFile(os.path.join('release', 'rp6simul-bin.zip'), 'w')

    stack = [ baseDir ]
    while stack:
        directory = stack.pop()
        for sfile in glob.glob(os.path.join(directory, '*')):
            if os.path.isdir(sfile):
                stack.append(sfile)
            else:
                if tfile != None: tfile.add(sfile, sfile.replace(baseDir, ''))
                if zfile != None: zfile.write(sfile, sfile.replace(baseDir, ''))

    if tfile != None: tfile.close()
    if zfile != None: zfile.close()

    print "Generated binary archive(s) in release/"

if 'src-zip' in action or 'src-tar' in action:
    tfile = None
    zfile = None

    if 'src-tar' in action:
        tfile = tarfile.open(os.path.join('release', 'rp6simul-src.tar.gz'), 'w:gz')
    if 'src-zip' in action:
        zfile = zipfile.ZipFile(os.path.join('release', 'rp6simul-src.zip'), 'w')

    for dfile in distFiles:
        if os.path.isdir(dfile):
            stack = [ dfile ]
            while stack:
                directory = stack.pop()
                for sfile in glob.glob(os.path.join(directory, '/*')):
                    if os.path.isdir(sfile):
                        stack.append(sfile)
                    else:
                        if tfile != None: tfile.add(sfile, os.path.join('rp6simul', sfile))
                        if zfile != None: zfile.write(sfile, os.path.join('rp6simul', sfile))
        else:
            if tfile != None: tfile.add(dfile, os.path.join('rp6simul', dfile))
            if zfile != None: zfile.write(dfile, os.path.join('rp6simul', dfile))

    if tfile != None: tfile.close()
    if zfile != None: zfile.close()

    print "Generated source archive(s) in release/"


if os.path.exists(baseDir):
    shutil.rmtree(baseDir)
