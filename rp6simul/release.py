#!/usr/bin/env python

import argparse
import errno
import glob
import os
import shutil
import subprocess
import tarfile
import zipfile

sourceFiles = [
    'src/main.cpp',
    'src/avrtimer.cpp',
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
                    help='Action to perform.', action='append')

args = parser.parse_args()

baseDir = 'release/.tmpdst'

print 'Installing to temporary prefix...'
print subprocess.check_output("""
    cd src && \
    qmake PREFIX=../%s && \
    make -j4 && \
    make install
    """ % baseDir, shell=True)

mkdirOpt('release')

if 'zip' in args.action:  
    zfile = zipfile.ZipFile('release/rp6simul-bin.zip', 'w')

    stack = [ baseDir ]
    while stack:
        directory = stack.pop()
        for sfile in glob.glob(directory + '/*'):
            print 'sfile: ' + sfile + ', ' + sfile.replace(baseDir, '')
            if os.path.isdir(sfile):
                stack.append(sfile)
            else:
                zfile.write(sfile, 'base/'+sfile.replace(baseDir, ''))

    stack = [ sourceFiles ]
    while stack:
        directory = stack.pop()
        for sfile in glob.glob(directory + '/*'):
            if os.path.isdir(sfile):
                stack.append(sfile)
            else:
                zfile.write(sfile)

    zfile.close()
    print "Generated zip archive in release/"

if 'tar' in args.action:  
    tarfile = tarfile.open('release/rp6simul-bin.tar.gz', 'w:gz')

    stack = [ baseDir ]
    while stack:
        directory = stack.pop()
        for sfile in glob.glob(directory + '/*'):
            if os.path.isdir(sfile):
                stack.append(sfile)
            else:
                tarfile.add(sfile, sfile.replace(baseDir, ''))

    tarfile.close()
    print "Generated gzipped binary tar archive in release/"

if 'src-zip' in args.action or 'src-tar' in args.action:


shutil.rmtree(baseDir)
