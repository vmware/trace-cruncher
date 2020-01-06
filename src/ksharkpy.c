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
	char *fname = NULL;

	static char *kwlist[] = {"fname", NULL};
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

	if (!kshark_open(kshark_ctx, fname))
		return Py_False;

	return Py_True;
}

static PyObject* method_close(PyObject* self, PyObject* noarg)
{
	struct kshark_context *kshark_ctx = NULL;

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	kshark_close(kshark_ctx);

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

static PyObject* method_get_tasks(PyObject* self, PyObject* noarg)
{
	struct kshark_context *kshark_ctx = NULL;
	const char *comm;
	int *pids;
	ssize_t i, n;

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	n = kshark_get_task_pids(kshark_ctx, &pids);
	if (n == 0) {
		PyErr_SetString(KSHARK_ERROR,
				"Failed to retrieve the PID-s of the tasks");
		return NULL;
	}

	qsort(pids, n, sizeof(*pids), compare);

	PyObject *tasks, *pid_list, *pid_val;

	tasks = PyDict_New();
	for (i = 0; i < n; ++i) {
		comm = tep_data_comm_from_pid(kshark_ctx->pevent, pids[i]);
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

static PyObject *method_register_plugin(PyObject *self, PyObject *args,
							PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	char *plugin, *lib_file;
	int ret;

	static char *kwlist[] = {"plugin", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "s",
					 kwlist,
					 &plugin)) {
		return NULL;
	}

	if (asprintf(&lib_file, "%s/lib/plugin-%s.so",
			        getenv("TRACE_CRUNCHER_PATH"),
				plugin) < 0) {
		KS_MEM_ERROR
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	ret = kshark_register_plugin(kshark_ctx, lib_file);
	free(lib_file);
	if (ret < 0) {
		PyErr_Format(KSHARK_ERROR,
			     "libshark failed to load plugin '%s'",
			     plugin);
		return NULL;
	}

	if (kshark_handle_plugins(kshark_ctx, KSHARK_PLUGIN_INIT) < 0) {
		PyErr_SetString(KSHARK_ERROR,
				"libshark failed to handle its plugins");
		return NULL;
	}

	Py_RETURN_NONE;
}

static PyObject *method_new_session_file(PyObject *self, PyObject *args,
							 PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_config_doc *session;
	struct kshark_config_doc *filters;
	struct kshark_config_doc *markers;
	struct kshark_config_doc *model;
	struct kshark_config_doc *file;
	struct kshark_trace_histo histo;
	const char *session_file, *data_file;

	static char *kwlist[] = {"data_file", "session_file", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "ss",
					 kwlist,
					 &data_file,
					 &session_file)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	session = kshark_config_new("kshark.config.session",
				    KS_CONFIG_JSON);

	file = kshark_export_trace_file(data_file, KS_CONFIG_JSON);
	kshark_config_doc_add(session, "Data", file);

	filters = kshark_export_all_filters(kshark_ctx, KS_CONFIG_JSON);
	kshark_config_doc_add(session, "Filters", filters);

	ksmodel_init(&histo);
	model = kshark_export_model(&histo, KS_CONFIG_JSON);
	kshark_config_doc_add(session, "Model", model);

	markers = kshark_config_new("kshark.config.markers", KS_CONFIG_JSON);
	kshark_config_doc_add(session, "Markers", markers);

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
	 METH_NOARGS,
	 "Close trace data file"
	},
	{"get_tasks",
	 (PyCFunction) method_get_tasks,
	 METH_NOARGS,
	 "Get all tasks recorded in a trace file"
	},
	{"register_plugin",
	 (PyCFunction) method_register_plugin,
	 METH_VARARGS | METH_KEYWORDS,
	 "Load a plugin"
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
