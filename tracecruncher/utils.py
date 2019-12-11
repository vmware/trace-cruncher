"""
SPDX-License-Identifier: LGPL-2.1

Copyright 2019 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
"""

import json

from . import datawrapper as dw
from . import ksharkpy as ks

def size(data):
    """ Get the number of trace records.
    """
    for key in dw.data_column_types:
        if data[key] is not None:
            return data[key].size

    raise Exception('Data size is unknown.')

def save_session(session, s):
    """ Save a KernelShark session description of a JSON file.
    """
    s.seek(0)
    json.dump(session, s, indent=4)
    s.truncate()


def new_gui_session(fname, sname):
    """ Generate and save a default KernelShark session description
        file (JSON).
    """
    ks.new_session_file(fname, sname)

    with open(sname, 'r+') as s:
        session = json.load(s)

        session['Filters']['filter mask'] = 7
        session['CPUPlots'] = []
        session['TaskPlots'] = []
        session['Splitter'] = [1, 1]
        session['MainWindow'] = [1200, 800]
        session['ViewTop'] = 0
        session['ColorScheme'] = 0.75
        session['Model']['bins'] = 1000

        session['Markers']['markA'] = {}
        session['Markers']['markA']['isSet'] = False
        session['Markers']['markB'] = {}
        session['Markers']['markB']['isSet'] = False
        session['Markers']['Active'] = 'A'

        save_session(session, s)

