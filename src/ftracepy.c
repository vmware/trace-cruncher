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

// trace-cruncher
#include "common.h"

static PyObject *KSHARK_ERROR = NULL;
static PyObject *FTRACE_ERROR = NULL;

static PyObject *method_event_id(PyObject *self, PyObject *args,
						 PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct tep_event *event;
	const char *system, *name;

	static char *kwlist[] = {"system", "event", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "ss",
					 kwlist,
					 &system,
					 &name)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	event = tep_find_event_by_name(kshark_ctx->pevent, system, name);
	if (!event) {
		PyErr_Format(FTRACE_ERROR,
			     "Failed to find event '%s/%s'",
			     system, name);
	}

	return PyLong_FromLong(event->id);
}

static PyObject *method_read_event_field(PyObject *self, PyObject *args,
							 PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	struct tep_format_field *evt_field;
	struct tep_record *record;
	struct tep_event *event;
	unsigned long long val;
	const char *field;
	uint64_t offset;
	int event_id, ret;

	static char *kwlist[] = {"offset", "event_id", "field", NULL};
	if(!PyArg_ParseTupleAndKeywords(args,
					kwargs,
					"Lis",
					kwlist,
					&offset,
					&event_id,
					&field)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	event = tep_find_event(kshark_ctx->pevent, event_id);
	if (!event) {
		PyErr_Format(FTRACE_ERROR,
			     "Failed to find event '%i'",
			     event_id);
		return NULL;
	}

	evt_field = tep_find_any_field(event, field);
	if (!evt_field) {
		PyErr_Format(FTRACE_ERROR,
			     "Failed to find field '%s' of event '%i'",
			     field, event_id);
		return NULL;
	}

	record = tracecmd_read_at(kshark_ctx->handle, offset, NULL);
	if (!record) {
		PyErr_Format(FTRACE_ERROR,
			     "Failed to read record at offset '%i'",
			     offset);
		return NULL;
	}

	ret = tep_read_number_field(evt_field, record->data, &val);
	free_record(record);

	if (ret != 0) {
		PyErr_Format(FTRACE_ERROR,
			     "Failed to read field '%s' of event '%i'",
			     field, event_id);
		return NULL;
	}

	return PyLong_FromLong(val);
}

static PyObject *method_get_function(PyObject *self, PyObject *args,
						     PyObject *kwargs)
{
	struct kshark_context *kshark_ctx = NULL;
	unsigned long long address;
	const char *func;

	static char *kwlist[] = {"address", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "L",
					 kwlist,
					 &address)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	func = tep_find_function(kshark_ctx->pevent, address);
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
	int pid;
	PyObject *ret;

	static char *kwlist[] = {"pid", "proc_addr", NULL};
	if (!PyArg_ParseTupleAndKeywords(args,
					 kwargs,
					 "iL",
					 kwlist,
					 &pid,
					 &proc_addr)) {
		return NULL;
	}

	if (!kshark_instance(&kshark_ctx)) {
		KS_INIT_ERROR
		return NULL;
	}

	mem_map = tracecmd_search_task_map(kshark_ctx->handle,
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
	{"event_id",
	 (PyCFunction) method_event_id,
	 METH_VARARGS | METH_KEYWORDS,
	 "Get the Id of the event from its name"
	},
	{"read_event_field",
	 (PyCFunction) method_read_event_field,
	 METH_VARARGS | METH_KEYWORDS,
	 "Get the value of an event field having a given name"
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
