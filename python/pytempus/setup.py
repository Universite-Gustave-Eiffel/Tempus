#!/usr/bin/env python
# -*- coding: utf-8 -*-
from setuptools import setup, Extension
from distutils.command.build_ext import build_ext

# cheap argument parsing
# -L add a library directory
# -I add an include directory
import sys
includes=[]
libs=[]
args=[]
lm=len(sys.argv) - 1
i = 0
while i < len(sys.argv):
    arg=sys.argv[i]
    if arg == '-I' and i < lm:
        includes.append(sys.argv[i+1])
        i += 1
    elif arg == '-L' and i < lm:
        libs.append(sys.argv[i+1])
        i += 1
    else:
        args.append(arg)
    i += 1
sys.argv=args

copt =  {'msvc': ['/EHsc'],
         'gcc' : ['-std=c++11'] }
libs_opt =  {'msvc' : ['tempus', 'boost_python-vc140-mt-1_63', 'libpq'],
             'gcc' : ['tempus', 'boost_python'] }


class build_ext_subclass( build_ext ):
    def build_extensions(self):
        c = self.compiler.compiler_type
        if copt.has_key(c):
           for e in self.extensions:
               e.extra_compile_args = copt[ c ]
        if lopt.has_key(c):
            for e in self.extensions:
                e.libraries = libs_opt[ c ]
        build_ext.build_extensions(self)

setup(
    name='pytempus',
    version='1.0.0',
    description="Tempus Python API",
    long_description="Tempus Python API module",
    classifiers=[
        "Programming Language :: Python",
        "Operating System :: OS Independent",
        "Intended Audience :: Developers",
        "Topic :: System :: Software Distribution",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],

    keywords='',
    author='Tempus Team',
    author_email='infos@oslandia.com',
    maintainer='Oslandia',
    maintainer_email='infos@oslandia.com',

    license='LGPL',
    ext_modules=[Extension('pytempus',
                           include_dirs = includes,
                           library_dirs = libs,
                           sources = ['src/pytempus.cc'])],
    cmdclass = {'build_ext': build_ext_subclass },
    include_package_data=True,
    zip_safe=False,
    install_requires=(),
    extras_require={
        "develop": ()
    }
)
