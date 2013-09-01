#include <Python.h>
#include <regex.h>
#include "structmember.h"
#include "scanner.h"

typedef struct {
    PyObject_HEAD
    PyObject *string;
    PyObject *match_history;
    PyObject *match;
    PyObject *pos_history;
    int *pos;
} StringScanner;

static void
StringScanner_dealloc(StringScanner* self)
{
    Py_XDECREF(self->string);
    Py_XDECREF(self->match_history);
    Py_XDECREF(self->match);
    Py_XDECREF(self->pos_history);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
StringScanner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    StringScanner *self;

    self = (StringScanner *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->string = PyString_FromString("");
        if (self->string == NULL)
        {
            Py_DECREF(self);
            return NULL;
        }

        self->match = PyString_FromString("");
        if (self->match == NULL)
        {
            Py_DECREF(self);
            return NULL;
        }

        self->pos = 0;
    }

    return (PyObject *)self;
}

static int
StringScanner_init(StringScanner *self, PyObject *args, PyObject *kwds)
{
    PyObject *string=NULL, *tmp;

    static char *kwlist[] = {"string", NULL};

    if (! PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &string))
        return -1; 

    if (string) {
        tmp = self->string;
        Py_INCREF(string);
        self->string = string;
        Py_XDECREF(tmp);
    }

    return 0;
}

static PyMemberDef StringScanner_members[] = {
    {"string", T_OBJECT_EX, offsetof(StringScanner, string), 0, "string"},
    {"match_history", T_OBJECT_EX, offsetof(StringScanner, match_history), 0, "match_history"},
    {"match", T_OBJECT_EX, offsetof(StringScanner, match), 0, "match"},
    {"pos_history", T_OBJECT_EX, offsetof(StringScanner, pos_history), 0, "pos_history"},
    {"pos", T_INT, offsetof(StringScanner, pos), 0, "StringScanner pos"},
    {NULL}  /* Sentinel */
};

static PyMethodDef StringScanner_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject scanner_StringScannerType = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "scanner.StringScanner",                   /*tp_name*/
    sizeof(StringScanner),                     /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)StringScanner_dealloc,         /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash */
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    0,                                         /*tp_getattro*/
    0,                                         /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    "StringScanner objects",                   /* tp_doc */
    0,                                         /* tp_traverse */
    0,                                         /* tp_clear */
    0,                                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter */
    0,                                         /* tp_iternext */
    StringScanner_methods,                     /* tp_methods */
    StringScanner_members,                     /* tp_members */
    0,                                         /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    (initproc)StringScanner_init,              /* tp_init */
    0,                                         /* tp_alloc */
    StringScanner_new,                         /* tp_new */
};

static PyMethodDef scanner_methods[] = {
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initscanner(void) 
{
    PyObject* m;

    scanner_StringScannerType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&scanner_StringScannerType) < 0)
        return;

    m = Py_InitModule3("scanner", scanner_methods,
                       "scanner module that creates an extension type.");

    Py_INCREF(&scanner_StringScannerType);
    PyModule_AddObject(m, "StringScanner", (PyObject *)&scanner_StringScannerType);
}
