/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#include <regex.h>
#include <string.h>
#include <solv/util.h>
#include "types.h"

int
hy_copy_str_from_subexpr(char** target, const char* source,
    regmatch_t* matches, int i)
{
    int subexpr_len = matches[i].rm_eo - matches[i].rm_so;
    if (subexpr_len == 0)
        return -1;
    *target = solv_malloc(sizeof(char*) * (subexpr_len + 1));
    strncpy(*target, &(source[matches[i].rm_so]), subexpr_len);
    (*target)[subexpr_len] = '\0';
    return 0;
}

int
hy_get_cmp_flags_from_subexpr(int *cmp_type, const char* source,
    regmatch_t* matches, int i)
{
    int subexpr_len = matches[i].rm_eo - matches[i].rm_so;
    const char *match_start = &(source[matches[i].rm_so]);
    if (subexpr_len == 2) {
        if (strncmp(match_start, "!=", 2) == 0)
            *cmp_type |= HY_NEQ;
        else if (strncmp(match_start, "<=", 2) == 0) {
            *cmp_type |= HY_LT;
            *cmp_type |= HY_EQ;
        }
        else if (strncmp(match_start, ">=", 2) == 0) {
            *cmp_type |= HY_GT;
            *cmp_type |= HY_EQ;
        }
        else
            return -1;
    } else if (subexpr_len == 1) {
        if (*match_start == '<')
            *cmp_type |= HY_LT;
        else if (*match_start == '>')
            *cmp_type |= HY_GT;
        else if (*match_start == '=')
            *cmp_type |= HY_EQ;
        else
            return -1;
    } else
        return -1;
    return 0;
}

int
hy_parse_nevra(const char *nevra, char **name, char **evr, int *cmp_type)
{
    regex_t reg;
    const char *regex="^([-a-zA-Z]*)\\s*(<=|>=|!=|<|>|=)?\\s*([-:0-9.]*)$";
    regmatch_t matches[4];
    *cmp_type = 0;

    regcomp(&reg, regex, REG_EXTENDED);

    if(regexec(&reg, nevra, 4, matches, 0) == 0) {
        if (hy_copy_str_from_subexpr(name, nevra, matches, 1) == -1)
            return -1;
        // without comparator and evr
        if ((matches[2].rm_eo - matches[2].rm_so) == 0 &&
            (matches[3].rm_eo - matches[3].rm_so) == 0)
            return 0;
        if (hy_copy_str_from_subexpr(evr, nevra, matches, 3) == -1)
            return -1;
        if (hy_get_cmp_flags_from_subexpr(cmp_type, nevra, matches, 2) == -1)
            return -1;
        return 0;
    }
    else
        return -1;
}
