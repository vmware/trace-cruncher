// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright 2019 VMware Inc, Yordan Karadzhov <ykaradzhov@vmware.com>
 */

// KernelShark
#include "kernelshark/libkshark.h"

ssize_t trace2matrix(uint64_t **offset_array,
		     uint16_t **cpu_array,
		     uint64_t **ts_array,
		     uint16_t **pid_array,
		     int **event_array)
{
	struct kshark_context *kshark_ctx = NULL;
	ssize_t total = 0;

	if (!kshark_instance(&kshark_ctx))
		return -1;

	total = kshark_load_data_matrix(kshark_ctx, offset_array,
						    cpu_array,
						    ts_array,
						    pid_array,
						    event_array);

	return total;
}
