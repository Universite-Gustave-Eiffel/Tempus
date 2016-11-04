#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
from os.path import join, dirname, abspath

from setuptools import setup, find_packages
from setuptools.command.test import test as _test

setup(
    name='tempusloader',
    version='0.9.2',
    description="Tempus data loader",
    long_description="Tempus data loader",
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
    packages=['tempusloader', 'tempusloader/provider'],
    package_data={
        'tempusloader/provider': ['sql/*.sql'],
    },
    include_package_data=True,
    zip_safe=False,
    install_requires=(),
    extras_require={
        "develop": ()
    },

    entry_points=dict(console_scripts=[
        'loadtempus=tempusloader.load_tempus:main',
    ]),
)

setup(
    name='pytempus',
    version='2.0.0',
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
    packages=['pytempus'],
    include_package_data=True,
    zip_safe=False,
    install_requires=(),
    extras_require={
        "develop": ()
    }
)
