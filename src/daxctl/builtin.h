/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2015-2018 Intel Corporation. All rights reserved. */
#ifndef _DAXCTL_BUILTIN_H_
#define _DAXCTL_BUILTIN_H_

struct daxctl_ctx;
int cmd_list(int argc, const char **argv, struct daxctl_ctx *ctx);
int cmd_migrate(int argc, const char **argv, struct daxctl_ctx *ctx);
#endif /* _DAXCTL_BUILTIN_H_ */
