#!/usr/bin/env python
# -*- coding: utf-8 -*-
from setuptools import setup, Extension

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
                           extra_compile_args=['-std=c++11'],
                           libraries = ['tempus', 'boost_python'],
                           sources = ['src/pytempus.cc'])],
    include_package_data=True,
    zip_safe=False,
    install_requires=(),
    extras_require={
        "develop": ()
    }
)
