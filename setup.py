#!/usr/bin/env python3

"""
SPDX-License-Identifier: LGPL-2.1

Copyright 2019 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
"""

from setuptools import setup, find_packages
from distutils.core import Extension
from Cython.Build import cythonize
import numpy as np

import sys

def main():
    third_party = './third_party'

    cythonize('src/datawrapper.pyx')

    third_party_libdirs = [third_party+'/lib/kernelshark',
                           third_party+'/lib/traceevent',
                           third_party+'/lib/trace-cmd']

    runtime_library_dirs=['$ORIGIN/lib']

    module_data = Extension('tracecruncher.datawrapper',
                            sources=['src/datawrapper.c'],
                            include_dirs=[np.get_include(), third_party+'/include'],
                            library_dirs=third_party_libdirs,
                            runtime_library_dirs=runtime_library_dirs,
                            libraries=['kshark', 'traceevent', 'tracecmd']
                            )

    module_ks = Extension('tracecruncher.ksharkpy',
                          sources=['src/ksharkpy.c'],
                          include_dirs=[third_party+'/include'],
                          library_dirs=third_party_libdirs,
                          runtime_library_dirs=runtime_library_dirs,
                          libraries=['kshark'],
                          )

    module_ft = Extension('tracecruncher.ftracepy',
                          sources=['src/ftracepy.c'],
                          include_dirs=[third_party+'/include'],
                          library_dirs=third_party_libdirs,
                          runtime_library_dirs=runtime_library_dirs,
                          libraries=['kshark', 'traceevent', 'tracecmd'],
                          )

    setup(name='tracecruncher',
          version='0.1.0',
          description='NumPy based interface for accessing tracing data in Python.',
          author='Yordan Karadzhov (VMware)',
          author_email='y.karadz@gmail.com',
          url='https://github.com/vmware/trace-cruncher',
          license='LGPL-2.1',
          packages=find_packages(),
          ext_modules=[module_data, module_ks, module_ft],
          package_data={'tracecruncher': ['lib/*.so*']},
          classifiers=[
              'Development Status :: 3 - Alpha',
              'Programming Language :: Python :: 3',
              ]
          )

if __name__ == '__main__':
    main()
