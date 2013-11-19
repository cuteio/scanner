# -*- coding: utf-8 -*-
import os
import sys
import shutil

from distutils.core import setup, Extension
from subprocess import Popen, PIPE

NAME = 'scanner'
VERSION = '0.0.4'

cwd = os.path.dirname(os.path.realpath(__file__))
libonig_dir = os.path.join(cwd, 'vendor')
libonig_include = os.path.join(cwd, 'vendor', 'onig-5.9.4')
libonig_lib = os.path.join(cwd, 'vendor')
os.chdir(libonig_dir)
popen = Popen(['tar', 'zxvf', libonig_dir + '/onig-5.9.4.tar.gz'], stdout=PIPE, stderr=PIPE)
stdoutdata, stderrdata = popen.communicate()
if popen.returncode != 0:
    print(stderrdata)
    sys.exit()

if sys.platform.startswith('linux'):
    configure_options = ['./configure', '--disable-shared', '--with-pic']
else:
    configure_options = ['./configure', '--disable-shared']

os.chdir(libonig_dir + '/onig-5.9.4')
popen = Popen(configure_options, stdout=PIPE, stderr=PIPE)
stdoutdata, stderrdata = popen.communicate()
if popen.returncode != 0:
    print(stderrdata)
    sys.exit()
print(stdoutdata)

popen = Popen(['make'], stdout=PIPE, stderr=PIPE)
stdoutdata, stderrdata = popen.communicate()
if popen.returncode != 0:
    print(stderrdata)
    sys.exit()
print(stdoutdata)

shutil.copy('.libs/libonig.a', libonig_dir)
os.chdir(cwd)

scanner_module = Extension(NAME,
                           sources=['src/scanner.c', 'src/regexp.c'],
                           include_dirs=[libonig_include, 'include'],
                           library_dirs=[libonig_lib],
                           libraries=['onig'])

setup(name=NAME,
      version=VERSION,
      keywords='scanner StringScanner',
      description="Scanner is like Ruby's StringScanner",

      license='MIT License',
      url='http://github.com/liluo/scanner',
      author='liluo',
      author_email='i@liluo.org',

      maintainer='xtao',
      maintainer_email='xutao881001@gmail.com',

      ext_modules=[scanner_module])
