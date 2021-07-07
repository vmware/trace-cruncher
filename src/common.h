/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

#ifndef _TC_COMMON_H
#define _TC_COMMON_H

// C
#include <stdbool.h>
#include <string.h>

#define TRACECRUNCHER_ERROR	tracecruncher_error
#define KSHARK_ERROR		kshark_error
#define TEP_ERROR		tep_error
#define TFS_ERROR		tfs_error

#define KS_INIT_ERROR \
	PyErr_SetString(KSHARK_ERROR, "libshark failed to initialize");

#define MEM_ERROR \
	PyErr_SetString(TRACECRUNCHER_ERROR, "failed to allocate memory");

static const char *NO_ARG = "/NONE/";

static inline bool is_all(const char *arg)
{
	const char all[] = "all";
	const char *p = &all[0];

	for (; *arg; arg++, p++) {
		if (tolower(*arg) != *p)
			return false;
	}
	return !(*p);
}

static inline bool is_no_arg(const char *arg)
{
	return arg[0] == '\0' || arg == NO_ARG;
}

static inline bool is_set(const char *arg)
{
	return !(is_all(arg) || is_no_arg(arg));
}

static inline void no_free()
{
}

#define NO_FREE		no_free

#define STR(x) #x

#define MAKE_TYPE_STR(x) STR(traceevent.x)

#define MAKE_DIC_STR(x) STR(libtraceevent x object)

#define C_OBJECT_WRAPPER_DECLARE(c_type, py_type)				\
	typedef struct {							\
	PyObject_HEAD								\
	struct c_type *ptrObj;							\
} py_type;									\
PyObject *py_type##_New(struct c_type *evt_ptr);				\
bool py_type##TypeInit();							\

#define  C_OBJECT_WRAPPER(c_type, py_type, ptr_free)				\
static PyTypeObject py_type##Type = {						\
	PyVarObject_HEAD_INIT(NULL, 0) MAKE_TYPE_STR(c_type)			\
};										\
PyObject *py_type##_New(struct c_type *evt_ptr)					\
{										\
	py_type *newObject;							\
	newObject = PyObject_New(py_type, &py_type##Type);			\
	newObject->ptrObj = evt_ptr;						\
	return (PyObject *) newObject;						\
}										\
static int py_type##_init(py_type *self, PyObject *args, PyObject *kwargs)	\
{										\
	self->ptrObj = NULL;							\
	return 0;								\
}										\
static void py_type##_dealloc(py_type *self)					\
{										\
	ptr_free(self->ptrObj);							\
	Py_TYPE(self)->tp_free(self);						\
}										\
bool py_type##TypeInit()							\
{										\
	py_type##Type.tp_new = PyType_GenericNew;				\
	py_type##Type.tp_basicsize = sizeof(py_type);				\
	py_type##Type.tp_init = (initproc) py_type##_init;			\
	py_type##Type.tp_dealloc = (destructor) py_type##_dealloc;		\
	py_type##Type.tp_flags = Py_TPFLAGS_DEFAULT;				\
	py_type##Type.tp_doc = MAKE_DIC_STR(c_type);				\
	py_type##Type.tp_methods = py_type##_methods;				\
	if (PyType_Ready(&py_type##Type) < 0)					\
		return false;							\
	Py_INCREF(&py_type##Type);						\
	return true;								\
}										\

#endif