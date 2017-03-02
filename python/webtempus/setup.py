#!/usr/bin/env python
# -*- coding: utf-8 -*-
from setuptools import setup

requirements = (
    'flask==0.12',
)

setup(
    name='webtempus',
    version='1.0.0',
    description="Tempus Web API",
    long_description="Flask application that exposes a web API to use Tempus",
    classifiers=[
        "Programming Language :: Python",
        "Operating System :: OS Independent",
        "Intended Audience :: Developers",
        "Topic :: System :: Software Distribution",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],

    keywords='tempus,web',
    author='Tempus Team',
    author_email='infos@oslandia.com',
    maintainer='Oslandia',
    maintainer_email='infos@oslandia.com',

    license='LGPL',

    include_package_data=True,
    install_requires=requirements,
)
