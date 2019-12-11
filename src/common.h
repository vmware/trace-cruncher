/* SPDX-License-Identifier: LGPL-2.1 */

/*
 * Copyright (C) 2017 VMware Inc, Yordan Karadzhov <y.karadz@gmail.com>
 */

#ifndef _TC_COMMON_H
#define _TC_COMMON_H

#define TRACECRUNCHER_ERROR	tracecruncher_error
#define KSHARK_ERROR		kshark_error

#define KS_INIT_ERROR \
	PyErr_SetString(KSHARK_ERROR, "libshark failed to initialize");

#define KS_MEM_ERROR \
	PyErr_SetString(TRACECRUNCHER_ERROR, "failed to allocate memory");

#endif
