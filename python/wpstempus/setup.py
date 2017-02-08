#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
from os.path import join, dirname, abspath

from setuptools import setup, find_packages

setup(
    name='wpstempus',
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
    packages=['wpstempus'],
    include_package_data=True,
    zip_safe=False,
    install_requires=(),
    extras_require={
        "develop": ()
    }
)
