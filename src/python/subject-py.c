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

#include <Python.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "src/iutil.h"
#include "src/nevra.h"
#include "src/reldep.h"
#include "src/sack.h"
#include "src/sack_internal.h"
#include "src/subject.h"
#include "src/types.h"

// pyhawkey
#include "nevra-py.h"
#include "pycomp.h"
#include "reldep-py.h"
#include "sack-py.h"
#include "subject-py.h"


// getsetters 

static PyObject *
get_pattern(_SubjectObject *self, void *closure)
{
    if (self->pattern == NULL)
        Py_RETURN_NONE;
    return PyString_FromString(self->pattern);
}

static PyGetSetDef subject_getsetters[] = {
    {"pattern", (getter)get_pattern, NULL, NULL, NULL},
    {NULL}          /* sentinel */
};

static PyObject *
subject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _SubjectObject *self = (_SubjectObject*)type->tp_alloc(type, 0);
    if (self) {
	   self->pattern = NULL;
    }
    return (PyObject*)self;
}

static void
subject_dealloc(_SubjectObject *self)
{
    solv_free(self->pattern);
    Py_TYPE(self)->tp_free(self);
}

static int
subject_init(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    char *pattern = NULL;
    if (!PyArg_ParseTuple(args, "s", &pattern))
        return -1;
    self->pattern = solv_strdup(pattern);
    return 0;
}

/* object methods */

static void
append_nevra(PyObject *list, HyNevra nevra)
{
    HyNevra nevra_copy = hy_nevra_clone(nevra);
    PyObject *no = nevraToPyObject(nevra_copy);
    PyList_Append(list, no);
    Py_DECREF(no);
}

static inline int
filter_all(HyNevra nevra, HySack sack, int glob, int icase)
{
    return 1;
}

static inline int
is_glob_pattern(char *str)
{
    if (str == NULL)
        return 0;
    while (*str != '\0') {
        if (*str == '*' || *str == '[' || *str == '?')
            return 1;
        str++;
    }
    return 0;
}

static inline int
filter_real(HyNevra nevra, HySack sack, int allow_glob, int icase)
{
    int flags = HY_NAME_ONLY;
    char *version = nevra->version;
    const char **existing_arches;
    if (nevra->name != NULL && !(allow_glob && is_glob_pattern(nevra->version))) {
        if (allow_glob && is_glob_pattern(nevra->version))
            version = NULL;
        if (icase)
            flags |= HY_ICASE;
        if (allow_glob && is_glob_pattern(nevra->name))
            flags |= HY_GLOB;
        if (sack_knows(sack, nevra->name, version, flags) == 0)
            return 0;
    }
    if (nevra->arch != NULL && !(allow_glob && is_glob_pattern(nevra->arch))) {
        if (strcmp(nevra->arch, "src") == 0)
            return 1;
        existing_arches = hy_sack_list_arches(sack);
        for (int i = 0; existing_arches[i] != NULL; ++i) {
            if (strcmp(nevra->arch, existing_arches[i]) == 0)
                return 1;
        }
        solv_free(existing_arches);
        return 0;
    }
    return 1;
}

static PyObject *
nevras_from_form(char *pattern, const char **default_form, PyObject *form, HySack sack,
    int glob, int icase, int (*filter)(HyNevra, HySack, int, int))
{
    PyObject *result = PyList_New(0);
    HyNevra nevra;
    char *str = NULL;
    PyObject *tmp_str = NULL;
    if (form == NULL) {
        // use default form
        FOR_NEVRAS_POSSIBILITIES(pattern, nevra, default_form) {
            if ((*filter)(nevra, sack, glob, icase))
                append_nevra(result, nevra);
        }
    } else if (PyString_Check(form) || PyUnicode_Check(form)) {
        str = pycomp_get_string(form, tmp_str);
        if (str == NULL)
            return NULL;
        nevra = hy_nevra_create();
        int ret = hy_nevra_possibility(pattern, str, nevra);
        Py_XDECREF(tmp_str);
        if (ret == 0) {
            if ((*filter)(nevra, sack, glob, icase))
                append_nevra(result, nevra);
            hy_nevra_free(nevra);
        }
    } else if (PyList_Check(form)) {
        for (int i = 0; i < PyList_Size(form); ++i) {
            nevra = hy_nevra_create();
            str = pycomp_get_string(PyList_GetItem(form, i), tmp_str);
            int ret = hy_nevra_possibility(pattern, str, nevra);
            Py_XDECREF(tmp_str);
            if (ret == 0) {
                if ((*filter)(nevra, sack, glob, icase))
                    append_nevra(result, nevra);
                hy_nevra_free(nevra);
            }
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "Expected a string or list of strings as form");
        return NULL;
    }
    return PyObject_GetIter(result);
}

static PyObject *
nevra_possibilities(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *form = NULL;
    char *kwlist[] = { "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &form)) {
        return NULL;
    }
    return nevras_from_form(self->pattern, forms_most_spec, form, NULL, -1, -1, filter_all);
}

static PyObject *
nevra_possibilities_real(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    HySack sack = NULL;
    int allow_globs = 0;
    int icase = 0;
    PyObject *form = NULL;
    char *kwlist[] = { "sack", "allow_globs", "icase", "form", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|iiO", kwlist,
        sack_converter, &sack, &allow_globs, &icase, &form))
        return NULL;
    
    return nevras_from_form(self->pattern, forms_real, form, sack, allow_globs, icase, filter_real);
}

static PyObject *
reldep_possibilities_real(_SubjectObject *self, PyObject *args, PyObject *kwds)
{
    HySack sack = NULL;
    int icase = 0;
    int flags = 0;
    char *kwlist[] = { "sack", "icase", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|i", kwlist,
        sack_converter, &sack, &icase))
        return NULL;
    if (icase)
        flags |= HY_ICASE;
    
    HyNevra nevra = hy_nevra_create();
    if (hy_nevra_possibility(self->pattern, HY_FORM_NAME, nevra) == 0 &&
        sack_knows(sack, self->pattern, NULL, flags)) {
        HyReldep reldep = hy_reldep_from_str(sack, self->pattern);
        if (reldep == NULL)
            Py_RETURN_NONE;
        return reldepToPyObject(reldep);
    } else {
        hy_nevra_free(nevra);
        Py_RETURN_NONE;
    }
}

static struct PyMethodDef subject_methods[] = {
    {"nevra_possibilities", (PyCFunction) nevra_possibilities,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {"nevra_possibilities_real", (PyCFunction) nevra_possibilities_real,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {"reldep_possibilities_real", (PyCFunction) reldep_possibilities_real,
    METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject subject_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Subject",        /*tp_name*/
    sizeof(_SubjectObject),   /*tp_basicsize*/
    0,              /*tp_itemsize*/
    (destructor) subject_dealloc,  /*tp_dealloc*/
    0,              /*tp_print*/
    0,              /*tp_getattr*/
    0,              /*tp_setattr*/
    0,              /*tp_compare*/
    0,              /*tp_repr*/
    0,              /*tp_as_number*/
    0,              /*tp_as_sequence*/
    0,              /*tp_as_mapping*/
    0,              /*tp_hash */
    0,              /*tp_call*/
    0,              /*tp_str*/
    0,              /*tp_getattro*/
    0,              /*tp_setattro*/
    0,              /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,     /*tp_flags*/
    "Subject object",     /* tp_doc */
    0,              /* tp_traverse */
    0,              /* tp_clear */
    0,              /* tp_richcompare */
    0,              /* tp_weaklistoffset */
    0,/* tp_iter */
    0,                          /* tp_iternext */
    subject_methods,      /* tp_methods */
    0,              /* tp_members */
    subject_getsetters,       /* tp_getset */
    0,              /* tp_base */
    0,              /* tp_dict */
    0,              /* tp_descr_get */
    0,              /* tp_descr_set */
    0,              /* tp_dictoffset */
    (initproc)subject_init,   /* tp_init */
    0,              /* tp_alloc */
    subject_new,          /* tp_new */
    0,              /* tp_free */
    0,              /* tp_is_gc */
};
