#include <Python.h>
#include <structmember.h>
#include "scanner.h"

#define MATCHED_P(s)          ((s)->flags & FLAG_MATCHED)
#define MATCHED(s)             (s)->flags |= FLAG_MATCHED
#define CLEAR_MATCH_STATUS(s)  (s)->flags &= ~FLAG_MATCHED

#define S_PBEG(s)  (PyString_AS_STRING((s)->str))
#define S_LEN(s)  (PyString_GET_SIZE((s)->str))
#define S_PEND(s)  (S_PBEG(s) + S_LEN(s))
#define CURPTR(s) (S_PBEG(s) + (s)->curr)
#define S_RESTLEN(s) (S_LEN(s) - (s)->curr)

#define EOS_P(s) ((s)->curr >= PyString_GET_SIZE(p->str))

extern PyTypeObject scanner_StringRegexpType;
static PyObject *ScanError;

static PyObject *
infect(PyObject *str, strscanner *p)
{
    // TODO: infect
    //OBJ_INFECT(str, p->str);
    return str;
}

static PyObject *
str_new(strscanner *p, const char *ptr, long len)
{
    // TODO: check p->str encode
    PyObject *str = PyString_FromStringAndSize(ptr, len);
    //rb_enc_copy(str, p->str);
    return str;
}

static PyObject *
extract_range(strscanner *p, long beg_i, long end_i)
{
    if (beg_i > S_LEN(p)) return Py_None;
    if (end_i > S_LEN(p))
        end_i = S_LEN(p);
    return infect(str_new(p, S_PBEG(p) + beg_i, end_i - beg_i), p);
}

static PyObject *
extract_beg_len(strscanner *p, long beg_i, long len)
{
    if (beg_i > S_LEN(p)) return Py_None;
    if (beg_i + len > S_LEN(p))
        len = S_LEN(p) - beg_i;
    return infect(str_new(p, S_PBEG(p) + beg_i, len), p);
}

static void
StringScanner_dealloc(StringScanner* self)
{
    strscanner *p = self->p;
    onig_region_free(&(p->regs), 0);
    PyMem_Free(p);
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
StringScanner_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    strscanner *p;
    StringScanner *self;

    self = (StringScanner *)type->tp_alloc(type, 0);
    if (self != NULL) {
        p = (strscanner *)PyMem_Malloc(sizeof (strscanner));
        if (p == NULL)
            return PyErr_NoMemory();

        CLEAR_MATCH_STATUS(p);
        onig_region_init(&(p->regs));
        p->str = Py_None;
        self->p = p;
    }

    return (PyObject *)self;
}

static int
StringScanner_init(StringScanner *self, PyObject *args, PyObject *kwds)
{
    strscanner *p;
    PyObject *string=NULL;
    PyObject *dup=NULL;
    static char *kwlist[] = {"string", "dup", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &string, &dup))
        return -1;

    p = self->p;
    if (string) {
        Py_INCREF(string);
        p->str = string;
    }

    return 0;
}

static PyObject *
strscan_do_scan(StringScanner *self, StringRegexp *regexp, int cuccptr, int getstr, int headonly)
{
    strscanner *p;
    regex_t *re;
    long ret;
    int tmpreg;

    //Check_Type(regex, T_REGEXP);
    //GET_SCANNER(self, p);
    p = self->p;

    CLEAR_MATCH_STATUS(p);
    if (S_RESTLEN(p) < 0) {
        return Py_None;
    }

    p->regex = regexp->regex;
    re = regexp->regex;

    if (headonly) {
        ret = onig_match(re, (UChar* )CURPTR(p),
                         (UChar* )(CURPTR(p) + S_RESTLEN(p)),
                         (UChar* )CURPTR(p), &(p->regs), ONIG_OPTION_NONE);
    } else {
        ret = onig_search(re,
                          (UChar* )CURPTR(p), (UChar* )(CURPTR(p) + S_RESTLEN(p)),
                          (UChar* )CURPTR(p), (UChar* )(CURPTR(p) + S_RESTLEN(p)),
                          &(p->regs), ONIG_OPTION_NONE);
    }
    //if (!tmpreg) RREGEXP(regex)->usecnt--;
    //if (tmpreg) {
    //    if (RREGEXP(regex)->usecnt) {
    //        onig_free(re);
    //    } else {
    //        onig_free(RREGEXP(regex)->ptr);
    //        RREGEXP(regex)->ptr = re;
    //    }
    //}

    if (ret == -2) {
        PyErr_SetString(ScanError, "regexp buffer overflow");
        return NULL;
    }

    if (ret < 0) {
        /* not matched */
        return Py_None;
    }

    MATCHED(p);
    p->prev = p->curr;
    if (cuccptr) {
        p->curr += p->regs.end[0];
    }
    if (getstr) {
        return extract_beg_len(p, p->prev, p->regs.end[0]);
    }
    else {
        return PyInt_FromLong(p->regs.end[0]);
    }
}

static PyObject *
StringScanner_scan(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 1, 1, 1);
}

static PyMemberDef StringScanner_members[] = {
    {NULL}  /* Sentinel */
};

static PyMethodDef StringScanner_methods[] = {
    {"scan", (PyCFunction)StringScanner_scan, METH_VARARGS, "scan"},
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

    if (PyType_Ready(&scanner_StringScannerType) < 0)
        return;

    if (PyType_Ready(&scanner_StringRegexpType) < 0)
        return;

    m = Py_InitModule3("scanner", scanner_methods,
                       "scanner module that creates an extension type.");
    Py_INCREF(&scanner_StringScannerType);
    Py_INCREF(&scanner_StringRegexpType);

    ScanError = PyErr_NewException("scanner.error", NULL, NULL);
    Py_INCREF(ScanError);

    PyModule_AddObject(m, "Error", ScanError);
    PyModule_AddObject(m, "StringScanner", (PyObject *)&scanner_StringScannerType);
    PyModule_AddObject(m, "StringRegexp", (PyObject *)&scanner_StringRegexpType);
}
