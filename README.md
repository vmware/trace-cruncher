

# trace-cruncher

## Overview

The Trace-Cruncher project aims to provide an interface between the existing instrumentation for collection and visualization of tracing data from the Linux kernel and the broad and very well developed ecosystem of instruments for data analysis available in Python. The interface is based on NumPy.

NumPy implements an efficient multi-dimensional container of generic data and uses strong typing in order to provide fast data processing in Python. The  Trace-Cruncher allows for sophisticated analysis of kernel tracing data via scripts, but it also opens the door for exposing the kernel tracing data to the instruments provided by the scientific toolkit of Python like MatPlotLib, Stats, Scikit-Learn and even to the nowadays most popular frameworks for Machine Learning like TensorFlow and PyTorch. The Trace-Cruncher is strongly coupled to the KernelShark project and is build on top of the C API of libkshark.

## Try it out

### Prerequisites

Trace-Cruncher has the following external dependencies:
  trace-cmd / KernelShark, Json-C, Cython, NumPy, MatPlotLib.

1.1 In order to install the packages on Ubuntu do the following:

    > sudo apt-get install libjson-c-dev libpython3-dev cython3 -y

    > sudo apt-get install python3-numpy python3-matplotlib -y

1.2 In order to install the packages on Fedora, as root do the following:

    > dnf install json-c-devel python3-devel python3-Cython -y

    > dnf install python3-numpy python3-matplotlib -y

### Build & Run

Installing trace-cruncher is very simple. After downloading the source code, you just have to run:

     > cd trace-cruncher

     > make

     > sudo make install

Note that this will automatically download, patch and build the appropriate versions of "trace-cmd / KernelShark" libraries from kernel.org. These third-party libraries will be installed as part of trace-cruncher itself and will not interfere with any existing system-wide installations of trace-cmd and KernelShark.

## Documentation

For bug reports and issues, please file it here:

https://bugzilla.kernel.org/buglist.cgi?component=Trace-cmd%2FKernelshark&product=Tools&resolution=---

## Contributing

The trace-cruncher project team welcomes contributions from the community. If you wish to contribute code and you have not signed our contributor license agreement (CLA), our bot will update the issue when you open a Pull Request. For any questions about the CLA process, please refer to our [FAQ](https://cla.vmware.com/faq).

## License

This is available under the [LGPLv2.1 license](COPYING-LGPLv2.1.txt).
