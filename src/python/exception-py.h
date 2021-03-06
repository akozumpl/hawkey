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

#ifndef EXCEPTION_PY_H
#define EXCEPTION_PY_H

extern PyObject *HyExc_Exception;
extern PyObject *HyExc_Value;
extern PyObject *HyExc_Query;
extern PyObject *HyExc_Arch;
extern PyObject *HyExc_Runtime;
extern PyObject *HyExc_Validation;

int init_exceptions(void);
int ret2e(int ret, const char *msg);

#endif // EXCEPTION_PY_H
