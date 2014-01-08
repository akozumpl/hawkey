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

#ifndef NEVRA_PY_H
#define NEVRA_PY_H

#include "src/nevra.h"

extern PyTypeObject nevra_Type;

HyNevra nevraFromPyObject(PyObject *o);
PyObject *nevraToPyObject(HyNevra nevra);
int nevra_converter(PyObject *o, HyNevra *nevra_ptr);

typedef struct {
    PyObject_HEAD
    HyNevra nevra;
} _NevraObject;

#endif
