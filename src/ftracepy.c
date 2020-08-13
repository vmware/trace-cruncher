// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright (C) 2019 VMware Inc, Yordan Karadzhov (VMware) <y.karadz@gmail.com>
 */

// Python
#include <Python.h>

// trace-cmd
#include "trace-cmd/trace-cmd.h"

// KernelShark
#include "kernelshark/libkshark.h"
#include "kernelshark/libkshark-tepdata.h"

// trace-cruncher
#include "common.h"

static PyObject *KSHARK_ERROR = NULL;
static PyObject *FTRACE_ERROR = NULL;

static PyObject *method_open_buffer(PyObject *self, PyObject *args,
						    PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	char *file_name, *buffer_name;
	int sd, sd_top;

	static char *kwlist[] = {"file_name", "buffer_name", NULL};
	if(!PyArg_ParseTupleAndKeywords(args,
					kwargs,
					"ss",
					kwlist,
					&file_name,
					&buffer_name)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	sd_top = kshark_tep_find_top_stream(kshark_ctx, file_name);
	if (sd_top < 0) {
		/* The "top" steam has to be initialized first. */
		sd_top = kshark_open(kshark_ctx, file_name);
	}

	if (sd_top < 0)
		return NULL;

	sd = kshark_tep_open_buffer(kshark_ctx, sd_top, buffer_name);
	if (sd < 0) {
		PyErr_Format(KSHARK_ERROR,
			     "Failed to open 'buffer' %s in file %s",
			     buffer_name, file_name);
		return NULL;
	}

	return PyLong_FromLong(sd);
}

static PyObject *method_get_function(PyObject *self, PyObject *args,
						     PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_data_stream *stream;
	unsigned long long address;
	const char *func;
	int stream_id;

	static char *kwlist[] = {"stream_id", "address", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "iL",
					 kwlist,
					 &stream_id,
					 &address)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	stream = kshark_get_data_stream(kshark_ctx, stream_id);
	if (!stream) {
		PyErr_Format(KSHARK_ERROR,
			     "No data stream %i loaded.",
			     stream_id);
		return NULL;
	}

	func = tep_find_function(kshark_get_tep(stream), address);
	if (!func)
		Py_RETURN_NONE;

	return PyUnicode_FromString(func);
}

static PyObject *method_map_instruction_address(PyObject *self, PyObject *args,
								PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct tracecmd_proc_addr_map *mem_map;
	unsigned long long proc_addr, obj_addr;
	struct kshark_data_stream *stream;
	int stream_id, pid;
	PyObject *ret;

	static char *kwlist[] = {"stream_id", "pid", "proc_addr", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "iiL",
					 kwlist,
					 &stream_id,
					 &pid,
					 &proc_addr)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	stream = kshark_get_data_stream(kshark_ctx, stream_id);
	if (!stream) {
		PyErr_Format(KSHARK_ERROR,
			     "No data stream %i loaded.",
			     stream_id);
		return NULL;
	}

	mem_map = tracecmd_search_task_map(kshark_get_tep_input(stream),
					   pid, proc_addr);

	if (!mem_map)
		Py_RETURN_NONE;

	ret = PyDict_New();

	PyDict_SetItemString(ret, "obj_file",
			     PyUnicode_FromString(mem_map->lib_name));

	obj_addr = proc_addr - mem_map->start;
	PyDict_SetItemString(ret, "address", PyLong_FromLong(obj_addr));

	return ret;
}

static PyMethodDef ftracepy_methods[] = {
	{"open_buffer",
	 (PyCFunction) method_open_buffer,
	 METH_VARARGS | METH_KEYWORDS,
	 ""
	},
	{"get_function",
	 (PyCFunction) method_get_function,
	 METH_VARARGS | METH_KEYWORDS,
	 ""
	},
	{"map_instruction_address",
	 (PyCFunction) method_map_instruction_address,
	 METH_VARARGS | METH_KEYWORDS,
	 ""
	},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef ftracepy_module = {
	PyModuleDef_HEAD_INIT,
	"ftracepy",
	"",
	-1,
	ftracepy_methods
};

PyMODINIT_FUNC PyInit_ftracepy(void)
{
	PyObject *module =  PyModule_Create(&ftracepy_module);

	KSHARK_ERROR = PyErr_NewException("tracecruncher.ftracepy.ks_error",
					  NULL, NULL);
	PyModule_AddObject(module, "ks_error", KSHARK_ERROR);

	FTRACE_ERROR = PyErr_NewException("tracecruncher.ftracepy.ft_error",
					  NULL, NULL);
	PyModule_AddObject(module, "ft_error", FTRACE_ERROR);

	return module;
}
