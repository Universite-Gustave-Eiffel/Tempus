# -*- coding: utf-8 -*-
import sys
from os.path import join, dirname, abspath

from setuptools import setup, find_packages
from setuptools.command.test import test as _test

__project_name__ = 'tempusloader'
__version__ = '0.9.2'

here = abspath(dirname(__file__))

requirements = (
)

develop_requirements = (
)

setup(
    name=__project_name__,
    version=__version__,
    description="Tempus data loader",
    long_description=(open("README.md").read(),),
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
    package_dir={'': 'src/python'},
    package_data={
        'tempusloader/provider': ['sql/*.sql'],
    },
    exclude_package_data = { '': ['CMakeLists.txt'] },
    include_package_data=True,
    zip_safe=False,
    install_requires=requirements,
    extras_require={
        "develop": develop_requirements,
    },

    entry_points=dict(console_scripts=[
        'loadtempus=tempusloader.load_tempus:main',
    ]),
)

