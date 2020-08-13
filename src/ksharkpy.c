// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2019 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
 */

/** Use GNU C Library. */
#define _GNU_SOURCE 1

// C
#include <stdio.h>
#include <dlfcn.h>

// Python
#include <Python.h>

// KernelShark
#include "kernelshark/libkshark.h"
#include "kernelshark/libkshark-plugin.h"
#include "kernelshark/libkshark-model.h"

// trace-cruncher
#include "common.h"

static PyObject *KSHARK_ERROR = NULL;
static PyObject *TRACECRUNCHER_ERROR = NULL;

static PyObject *method_open(PyObject *self, PyObject *args,
					     PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	char *fname;
	int sd;

	static char *kwlist[] = {"file_name", NULL};
	if(!PyArg_ParseTupleAndKeywords(args,
					kwargs,
					"s",
					kwlist,
					&fname)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	sd = kshark_open(kshark_ctx, fname);
	if (sd < 0) {
		PyErr_Format(KSHARK_ERROR, "Failed to open file %s", fname);
		return NULL;
	}

	return PyLong_FromLong(sd);
}

static PyObject* method_close(PyObject* self, PyObject* noarg)
{
	struct kshark_context *kshark_ctx = NULL;

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	kshark_close_all(kshark_ctx);

	Py_RETURN_NONE;
}

static struct kshark_data_stream *get_stream(int stream_id)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_data_stream *stream;

	if (!kshark_instance(&kshark_ctx))
		return NULL;

	stream = kshark_get_data_stream(kshark_ctx, stream_id);
	if (!stream) {
		PyErr_Format(KSHARK_ERROR,
			     "No data stream %i loaded.",
			     stream_id);
		return NULL;
	}

	return stream;
}

static PyObject* method_set_clock_offset(PyObject* self, PyObject* args,
							 PyObject *kwargs)
{
	struct kshark_data_stream *stream;
	int64_t offset;
	int stream_id;

	static char *kwlist[] = {"stream_id", "offset", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "iL",
					 kwlist,
					 &stream_id,
					 &offset)) {
		return NULL;
	}

	stream = get_stream(stream_id);
	if (!stream)
		return NULL;

	if (stream->calib_array)
		free(stream->calib_array);

	stream->calib_array = malloc(sizeof(*stream->calib_array));
	stream->calib_array[0] = offset;
	stream->calib_array_size = 1;

	stream->calib = kshark_offset_calib;

	Py_RETURN_NONE;
}

static int compare(const void *a, const void *b)
{
	int a_i, b_i;

	a_i = *(const int *) a;
	b_i = *(const int *) b;

	if (a_i > b_i)
		return +1;

	if (a_i < b_i)
		return -1;

	return 0;
}

static PyObject* method_get_tasks(PyObject* self, PyObject* args,
						  PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	const char *comm;
	int sd, *pids;
	ssize_t i, n;

	static char *kwlist[] = {"stream_id", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "i",
					 kwlist,
					 &sd)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	n = kshark_get_task_pids(kshark_ctx, sd, &pids);
	if (n == 0) {
		PyErr_SetString(KSHARK_ERROR,
				"Failed to retrieve the PID-s of the tasks");
		return NULL;
	}

	qsort(pids, n, sizeof(*pids), compare);

	PyObject *tasks, *pid_list, *pid_val;

	tasks = PyDict_New();
	for (i = 0; i < n; ++i) {
		comm = kshark_comm_from_pid(sd, pids[i]);
		pid_val = PyLong_FromLong(pids[i]);
		pid_list = PyDict_GetItemString(tasks, comm);
		if (!pid_list) {
			pid_list = PyList_New(1);
			PyList_SET_ITEM(pid_list, 0, pid_val);
			PyDict_SetItemString(tasks, comm, pid_list);
		} else {
			PyList_Append(pid_list, pid_val);
		}
	}

	return tasks;
}

static PyObject *method_event_id(PyObject *self, PyObject *args,
						 PyObject *kwargs)
{
	struct kshark_data_stream *stream;
	int stream_id, event_id;
	const char *name;

	static char *kwlist[] = {"stream_id", "name", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "is",
					 kwlist,
					 &stream_id,
					 &name)) {
		return NULL;
	}

	stream = get_stream(stream_id);
	if (!stream)
		return NULL;

	event_id = stream->interface.find_event_id(stream, name);
	return PyLong_FromLong(event_id);
}

static PyObject *method_event_name(PyObject *self, PyObject *args,
						 PyObject *kwargs)
{
	struct kshark_data_stream *stream;
	struct kshark_entry entry;
	int stream_id, event_id;
	PyObject *ret;
	char *name;

	static char *kwlist[] = {"stream_id", "event_id", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "ii",
					 kwlist,
					 &stream_id,
					 &event_id)) {
		return NULL;
	}

	stream = get_stream(stream_id);
	if (!stream)
		return NULL;

	entry.event_id = event_id;
	entry.visible = 0xFF;
	name = stream->interface.get_event_name(stream, &entry);

	ret = PyUnicode_FromString(name);
	free(name);

	return ret;
}

static PyObject *method_read_event_field(PyObject *self, PyObject *args,
							 PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_entry entry;
	int event_id, ret, sd;
	const char *field;
	int64_t offset;
	int64_t val;

	static char *kwlist[] = {"stream_id", "offset", "event_id", "field", NULL};
	if(!PyArg_ParseTupleAndKeywords(args,
					kwargs,
					"iLis",
					kwlist,
					&sd,
					&offset,
					&event_id,
					&field)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	entry.event_id = event_id;
	entry.offset = offset;
	entry.stream_id = sd;

	ret = kshark_read_event_field(&entry, field, &val);
	if (ret != 0) {
		PyErr_Format(KSHARK_ERROR,
			     "Failed to read field '%s' of event '%i'",
			     field, event_id);
		return NULL;
	}

	return PyLong_FromLong(val);
}

static PyObject *method_new_session_file(PyObject *self, PyObject *args,
							 PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_config_doc *session;
	struct kshark_config_doc *plugins;
	struct kshark_config_doc *markers;
	struct kshark_config_doc *model;
	struct kshark_trace_histo histo;
	const char *session_file;

	static char *kwlist[] = {"session_file", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "s",
					 kwlist,
					 &session_file)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	session = kshark_config_new("kshark.config.session",
				    KS_CONFIG_JSON);

	kshark_ctx->filter_mask = KS_TEXT_VIEW_FILTER_MASK |
				  KS_GRAPH_VIEW_FILTER_MASK |
				  KS_EVENT_VIEW_FILTER_MASK;

	kshark_export_all_dstreams(kshark_ctx, &session);

	ksmodel_init(&histo);
	model = kshark_export_model(&histo, KS_CONFIG_JSON);
	kshark_config_doc_add(session, "Model", model);

	markers = kshark_config_new("kshark.config.markers", KS_CONFIG_JSON);
	kshark_config_doc_add(session, "Markers", markers);

	plugins = kshark_config_new("kshark.config.plugins", KS_CONFIG_JSON);
	kshark_config_doc_add(session, "User Plugins", plugins);

	kshark_save_config_file(session_file, session);
	kshark_free_config_doc(session);

	Py_RETURN_NONE;
}

static PyMethodDef ksharkpy_methods[] = {
	{"open",
	 (PyCFunction) method_open,
	 METH_VARARGS | METH_KEYWORDS,
	 "Open trace data file"
	},
	{"close",
	 (PyCFunction) method_close,
	 METH_VARARGS | METH_KEYWORDS,
	 "Close trace data file"
	},
	{"set_clock_offset",
	 (PyCFunction) method_set_clock_offset,
	 METH_VARARGS | METH_KEYWORDS,
	 "Set the clock offset of the data stream"
	},
	{"get_tasks",
	 (PyCFunction) method_get_tasks,
	 METH_VARARGS | METH_KEYWORDS,
	 "Get all tasks recorded in a trace file"
	},
	{"event_id",
	 (PyCFunction) method_event_id,
	 METH_VARARGS | METH_KEYWORDS,
	 "Get the Id of the event from its name"
	},
	{"event_name",
	 (PyCFunction) method_event_name,
	 METH_VARARGS | METH_KEYWORDS,
	 "Get the name of the event from its Id number"
	},
	{"read_event_field",
	 (PyCFunction) method_read_event_field,
	 METH_VARARGS | METH_KEYWORDS,
	 "Get the value of an event field having a given name"
	},
	{"new_session_file",
	 (PyCFunction) method_new_session_file,
	 METH_VARARGS | METH_KEYWORDS,
	 "Create new session description file"
	},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef ksharkpy_module = {
	PyModuleDef_HEAD_INIT,
	"ksharkpy",
	"",
	-1,
	ksharkpy_methods
};

PyMODINIT_FUNC PyInit_ksharkpy(void)
{
	PyObject *module = PyModule_Create(&ksharkpy_module);

	KSHARK_ERROR = PyErr_NewException("tracecruncher.ksharkpy.ks_error",
					  NULL, NULL);
	PyModule_AddObject(module, "ks_error", KSHARK_ERROR);

	TRACECRUNCHER_ERROR = PyErr_NewException("tracecruncher.tc_error",
						 NULL, NULL);
	PyModule_AddObject(module, "tc_error", TRACECRUNCHER_ERROR);

	return module;
}
