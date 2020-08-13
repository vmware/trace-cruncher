// SPDX-License-Identifier: LGPL-2.1

/*
 * Copyright 2019 VMware Inc, Yordan Karadzhov <ykaradzhov@vmware.com>
 */

// KernelShark
#include "kernelshark/libkshark.h"

ssize_t trace2matrix(int sd,
		     int16_t **cpu_array,
		     int32_t **pid_array,
		     int32_t **event_array,
		     int64_t **offset_array,
		     uint64_t **ts_array)
{
	struct kshark_context *kshark_ctx = NULL;
	struct kshark_data_stream *stream;
	ssize_t total = 0;

	if (!kshark_instance(&kshark_ctx))
		return -1;

	stream = kshark_get_data_stream(kshark_ctx, sd);
	if (!stream)
		return -1;

	total = stream->interface.load_matrix(stream, kshark_ctx, cpu_array,
								  pid_array,
								  event_array,
								  offset_array,
								  ts_array);

	return total;
}
