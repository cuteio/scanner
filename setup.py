from distutils.core import setup, Extension

NAME = 'scanner'
VERSION = '0.01'

scanner_module = Extension(NAME, sources=['scanner.c'])

setup(name=NAME,
      version=VERSION,
      keywords='scanner StringScanner',
      description="Scanner is like Ruby's StringScanner",

      license='MIT License',
      url='http://github.com/liluo/scanner',
      author='liluo',
      author_email='i@liluo.org',

      ext_modules=[scanner_module])
