#!/usr/bin/env python
# -*- coding: utf-8 -*-
from setuptools import setup

requirements = (
    'geomet==0.1.1',
    'flask==0.12',
    'psycopg2==2.7',
    'pytempus'
)

setup(
    name='webtempus',
    version='1.0.0',
    description="Web API for Tempus, multimodal path planning request",
    long_description="Flask application that exposes a web API to configure and use tempus, a C++ framework to develop multimodal path planning requests: https://github.com/Ifsttar/Tempus/",
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
