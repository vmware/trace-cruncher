#
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2019 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
#

all:
	python3 setup.py build

clean:
	rm -rf build src/datawrapper.c

install:
	python3 setup.py install --record install_manifest.txt

uninstall:
	xargs rm -v < install_manifest.txt
