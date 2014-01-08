/*
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HY_NEVRA_H
#define HY_NEVRA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum _hy_nevra_param_e {
    HY_NEVRA_NAME,
    HY_NEVRA_EPOCH,
    HY_NEVRA_VERSION,
    HY_NEVRA_RELEASE,
    HY_NEVRA_ARCH
};

struct Nevra {
    char *name;
    int epoch;
    char *version;
    char *release;
    char *arch;
};

HyNevra hy_nevra_create();
void hy_nevra_clear(HyNevra nevra);
void hy_nevra_free(HyNevra nevra);
HyNevra hy_nevra_clone(HyNevra nevra);
int hy_nevra_cmp(HyNevra nevra1, HyNevra nevra2);
int hy_nevra_possibility(char *nevra_str, const char *re, HyNevra nevra);
int hy_parse_nevra(char *nevra_str, const char *re, char **n, int *e, char **v,
	char **r, char **a);
void print_nevra(HyNevra nevra);

const char *hy_nevra_get_string(HyNevra nevra, int which);
void hy_nevra_set_string(HyNevra nevra, int which, const char* str_val);

#ifdef __cplusplus
}
#endif

#endif
