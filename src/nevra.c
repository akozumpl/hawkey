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

#include <stdlib.h>
#include <regex.h>
#include "iutil.h"
#include "types.h"
#include "nevra.h"

HyNevra
hy_nevra_create()
{
    HyNevra nevra = solv_calloc(1, sizeof(*nevra));
    hy_nevra_clear(nevra);
    return nevra;
}

void
hy_nevra_clear(HyNevra nevra)
{
    nevra->name = NULL;
    nevra->epoch = -1L;
    nevra->version = NULL;
    nevra->release = NULL;
    nevra->arch = NULL;
}

static inline int
string_cmp(const char *s1, const char *s2)
{
    if (s1 == NULL)
        return -1;
    else if (s2 == NULL)
        return 1;
    return strcmp(s1, s2);
}

int
hy_nevra_cmp(HyNevra nevra1, HyNevra nevra2)
{
    int ret;
    char *nevra1_strs[] = { nevra1->name, nevra1->version, nevra1->release, nevra1->arch };
    char *nevra2_strs[] = { nevra2->name, nevra2->version, nevra2->release, nevra2->arch };
    if (nevra1->epoch < nevra2->epoch)
        return -1;
    else if (nevra1->epoch > nevra2->epoch)
        return 1;
    else
        return 0;
    for (int i = 0; i < 4; ++i) {
        ret = string_cmp(nevra1_strs[i], nevra2_strs[i]);
        if (ret)
            return ret;
    }
    return 0;
}

void
hy_nevra_free(HyNevra nevra)
{
    solv_free(nevra->name);
    solv_free(nevra->version);
    solv_free(nevra->release);
    solv_free(nevra->arch);
    solv_free(nevra);
}

HyNevra
hy_nevra_clone(HyNevra nevra)
{
    HyNevra clone = hy_nevra_create();
    clone->name = solv_strdup(nevra->name);
    clone->epoch = nevra->epoch;
    clone->version = solv_strdup(nevra->version);
    clone->release = solv_strdup(nevra->release);
    clone->arch = solv_strdup(nevra->arch);
    return clone;
}

static inline char **
get_string(HyNevra nevra, int which)
{
    switch (which) {
    case HY_NEVRA_NAME:
    return &(nevra->name);
    case HY_NEVRA_VERSION:
    return &(nevra->version);
    case HY_NEVRA_RELEASE:
    return &(nevra->release);
    case HY_NEVRA_ARCH:
    return &(nevra->arch);
    default:
    return NULL;
    }
}

const char *
hy_nevra_get_string(HyNevra nevra, int which)
{
    return *get_string(nevra, which);
}


void
hy_nevra_set_string(HyNevra nevra, int which, const char* str_val)
{
    char** attr = get_string(nevra, which);
    solv_free(*attr);
    *attr = solv_strdup(str_val);
}

#define MATCH_EMPTY(i) (matches[i].rm_so >= matches[i].rm_eo)

int
hy_parse_nevra(char *nevra_str, const char *re, char **n, int *e, char **v,
    char **r, char **a)
{
    enum { NAME = 1, EPOCH = 3, VERSION = 4, RELEASE = 5, ARCH = 6 };
    regex_t reg;
    char *epoch = NULL;
    *e = -1;
    
    regmatch_t matches[10];

    regcomp(&reg, re, REG_EXTENDED);
    if (regexec(&reg, nevra_str, 10, matches, 0) == 0) {
        if (!MATCH_EMPTY(EPOCH)) {
            copy_str_from_subexpr(&epoch, nevra_str, matches, EPOCH);
            *e = atoi(epoch);
            free(epoch);
        }
        if (copy_str_from_subexpr(n, nevra_str, matches, NAME) == -1)
            return -1;
        copy_str_from_subexpr(v, nevra_str, matches, VERSION);
        copy_str_from_subexpr(r, nevra_str, matches, RELEASE);
        copy_str_from_subexpr(a, nevra_str, matches, ARCH);
        return 0;
    } else {
        return -1;
    }
}

#undef MATCH_EMPTY

int
hy_nevra_possibility(char *nevra_str, const char *re, HyNevra nevra)
{
    if (hy_parse_nevra(nevra_str, re, &(nevra->name), &(nevra->epoch),
        &(nevra->version), &(nevra->release), &(nevra->arch)) == -1) {
        return -1;
    }
    return 0;
}

void
print_nevra(HyNevra nevra) // TODO erase
{
    printf("### ");
    if (nevra->name == NULL)
        printf("name == NULL, ");
    else
        printf("name == %s, ", nevra->name);
    printf("epoch == %d, ", nevra->epoch);
    if (nevra->version == NULL)
        printf("version == NULL, ");
    else
        printf("version == %s, ", nevra->version);
    if (nevra->release == NULL)
        printf("release == NULL, ");
    else
        printf("release == %s, ", nevra->release);
    if (nevra->arch == NULL)
        printf("arch == NULL, ");
    else
        printf("arch == %s", nevra->arch);
    printf("\n");
    fflush(stdout);
}

