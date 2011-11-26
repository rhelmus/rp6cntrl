#!/usr/bin/env python

import argparse
import errno
import glob
import os
import shutil
import subprocess
import zipfile

SOURCES = [
    'rp6simul/main.cpp',
    'rp6simul/avrtimer.cpp',
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
parser.add_argument('action', choices=['zip', 'src', 'nixstaller', 'winstaller'],
                    help='Action to perform.', action='append')

args = parser.parse_args()

print 'Installing to temporary prefix...'
print subprocess.check_output('cd rp6simul && qmake PREFIX=../release/.tmpdst && make -j4 && make install', shell=True)

if 'zip' in args.action:
    flist = ['release/.tmpdst']
    
    mkdirOpt('release')
    zfile = zipfile.ZipFile('release/rp6simul.zip', 'w')
    
    for dfile in flist:
        print 'dfile: ' + dfile
        if os.path.isdir(dfile):
            stack = [ dfile ]
            while stack:
                directory = stack.pop()
                for sfile in glob.glob(directory + '/*'):
                    if os.path.isdir(sfile):
                        stack.append(sfile)
                    else:
                        zfile.write(sfile, 'src/'+sfile)
        else:
            zfile.write(dfile, 'src/'+dfile)

    zfile.close()
    print "Generated zip archive in release/"

shutil.rmtree('release/.tmpdst')
