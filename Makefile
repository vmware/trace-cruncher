#
# SPDX-License-Identifier: GPL-2.0
#
# Copyright 2019 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
#

all:
	./install_third_party.sh
	python3 setup.py build

clean:
	rm -rf third_party build tracecruncher/lib
	rm -f src/datawrapper.c

install:
	python3 setup.py install --record install_manifest.txt

uninstall:
	xargs rm -v < install_manifest.txt
