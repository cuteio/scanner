#include <Python.h>
#include <structmember.h>
#include "scanner.h"

/**
 * The oniguruma syntax for python
 */
static OnigSyntaxType OnigSyntaxPython;


/**
 * Get the oniguruma encoding for a given integer. Because we don't forward
 * oniguruma encodings to the python space, we have to use something like that.
 */
static OnigEncodingType *
get_onig_encoding(int encoding)
{
    switch (encoding) {
        case  0: return ONIG_ENCODING_ASCII;
        case  1: return ONIG_ENCODING_ISO_8859_1;
        case  2: return ONIG_ENCODING_ISO_8859_2;
        case  3: return ONIG_ENCODING_ISO_8859_3;
        case  4: return ONIG_ENCODING_ISO_8859_4;
        case  5: return ONIG_ENCODING_ISO_8859_5;
        case  6: return ONIG_ENCODING_ISO_8859_6;
        case  7: return ONIG_ENCODING_ISO_8859_7;
        case  8: return ONIG_ENCODING_ISO_8859_8;
        case  9: return ONIG_ENCODING_ISO_8859_9;
        case 10: return ONIG_ENCODING_ISO_8859_10;
        case 11: return ONIG_ENCODING_ISO_8859_11;
        case 12: return ONIG_ENCODING_ISO_8859_11;
        case 13: return ONIG_ENCODING_ISO_8859_13;
        case 14: return ONIG_ENCODING_ISO_8859_14;
        case 15: return ONIG_ENCODING_ISO_8859_15;
        case 16: return ONIG_ENCODING_ISO_8859_16;
        case 17: return ONIG_ENCODING_UTF8;
        case 18: return ONIG_ENCODING_UTF16_BE;
        case 19: return ONIG_ENCODING_UTF16_LE;
        case 20: return ONIG_ENCODING_UTF32_BE;
        case 21: return ONIG_ENCODING_UTF32_LE;
        case 22: return ONIG_ENCODING_EUC_JP;
        case 23: return ONIG_ENCODING_EUC_TW;
        case 24: return ONIG_ENCODING_EUC_KR;
        case 25: return ONIG_ENCODING_EUC_CN;
        case 26: return ONIG_ENCODING_SJIS;
        /* case 27: return ONIG_ENCODING_KOI8; */
        case 28: return ONIG_ENCODING_KOI8_R;
        case 29: return ONIG_ENCODING_CP1251;
        case 30: return ONIG_ENCODING_BIG5;
        case 31: return ONIG_ENCODING_GB18030;
        default: return ONIG_ENCODING_UNDEF;
    }
}

/**
 * initialize the python syntax based on the ruby one
 */
int
init_python_syntax(void)
{
    onig_copy_syntax(&OnigSyntaxPython, ONIG_SYNTAX_RUBY);
    int behavior = onig_get_syntax_behavior(&OnigSyntaxPython);

    /* use the ruby settings but disable the use of the same
       name for multiple groups, disable warnings for stupid
       escapes and capture named and position groups */
    onig_set_syntax_behavior(&OnigSyntaxPython,
            behavior & ~(ONIG_SYN_CAPTURE_ONLY_NAMED_GROUP |
                ONIG_SYN_ALLOW_MULTIPLEX_DEFINITION_NAME |
                ONIG_SYN_WARN_CC_OP_NOT_ESCAPED |
                ONIG_SYN_WARN_REDUNDANT_NESTED_REPEAT)
            );
    /* sre like singleline */
    onig_set_syntax_options(&OnigSyntaxPython, ONIG_OPTION_NEGATE_SINGLELINE);
    return 0;
}

/**
 * Like get_onig_encoding, but for the syntax.
 */
static OnigSyntaxType *
get_onig_syntax(int syntax)
{
    switch (syntax) {
        case  0: return ONIG_SYNTAX_ASIS;
        case  1: return ONIG_SYNTAX_POSIX_BASIC;
        case  2: return ONIG_SYNTAX_POSIX_EXTENDED;
        case  3: return ONIG_SYNTAX_EMACS;
        case  4: return ONIG_SYNTAX_GREP;
        case  5: return ONIG_SYNTAX_GNU_REGEX;
        case  6: return ONIG_SYNTAX_JAVA;
        case  7: return ONIG_SYNTAX_PERL;
        case  8: return ONIG_SYNTAX_PERL_NG;
        case  9: return ONIG_SYNTAX_RUBY;
        case 10:
        default: return &OnigSyntaxPython;
    }
}

static void
StringRegexp_dealloc(StringRegexp *self)
{
    regexp_delloc(self);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

void
regexp_delloc(StringRegexp *self)
{
    if (self->regex)
        onig_free(self->regex);
    Py_XDECREF(self->pattern);
}

static PyObject *
StringRegexp_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    StringRegexp *self;

    self = (StringRegexp *)type->tp_alloc(type, 0);

    if (self != NULL) {
        self->pattern = NULL;
        self->regex = NULL;
    }

    return (PyObject *)self;
}

int
regexp_init(StringRegexp *self, PyObject *pattern)
{
    int ienc = -1, isyn = 10, rv;
    OnigOptionType options = ONIG_OPTION_NONE;
    OnigSyntaxType *syn;
    OnigEncodingType *enc;
    OnigErrorInfo einfo;
    UChar *pstr, *pend;

    if (PyUnicode_Check(pattern)) {
        //enc = UNICODE_ENCODING;
        enc = ONIG_ENCODING_UTF8;
        pstr = (UChar *) PyUnicode_AS_UNICODE(pattern);
        pend = pstr + (PyUnicode_GET_SIZE(pattern) * sizeof(PY_UNICODE_TYPE));
        self->unicode = 1;
    } else if (PyBytes_Check(pattern)) {
        /* FIXME: to unicode */
        if (ienc == -1) ienc = 0;
        enc = get_onig_encoding(ienc);
        pstr = (UChar *) PyBytes_AS_STRING(pattern);
        pend = pstr + PyBytes_GET_SIZE(pattern);
        self->unicode = 0;
    } else {
        PyErr_SetString(PyExc_TypeError, "pattern must be string or unicode");
        Py_DECREF(self);
        return -1;
    }

    /* Got to keep a reference to the pattern string */
    Py_INCREF(pattern);
    self->pattern = pattern;

    /* XXX: check for invalid values? */
    syn = get_onig_syntax(isyn);

    rv = onig_new(&(self->regex), pstr, pend, options, enc, syn, &einfo);

    if (rv != ONIG_NORMAL) {
        UChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str(s, rv, &einfo);
        //PyErr_SetString(RegexpError, (char *)s);
        Py_DECREF(self);
        return -1;
    }

    return 0;
}

static int
StringRegexp_init(StringRegexp *self, PyObject *args, PyObject *kwds)
{
    PyObject *pattern=NULL;
    static char *kwlist[] = {"pattern", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &pattern))
        return -1;

    return regexp_init(self, pattern);
}

static PyMemberDef StringRegexp_members[] = {
    {"pattern", T_OBJECT_EX, offsetof(StringRegexp, pattern), 0, "pattern"},
    {NULL}  /* Sentinel */
};

static PyMethodDef StringRegexp_methods[] = {
    {NULL}  /* Sentinel */
};

PyTypeObject scanner_StringRegexpType = {
    PyVarObject_HEAD_INIT(NULL, 0)             /*ob_size*/
    "scanner.StringRegexp",                     /*tp_name*/
    sizeof(StringRegexp),                       /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)StringRegexp_dealloc,           /*tp_dealloc*/
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
    "StringRegexp objects",                     /* tp_doc */
    0,                                         /* tp_traverse */
    0,                                         /* tp_clear */
    0,                                         /* tp_richcompare */
    0,                                         /* tp_weaklistoffset */
    0,                                         /* tp_iter */
    0,                                         /* tp_iternext */
    StringRegexp_methods,                       /* tp_methods */
    StringRegexp_members,                       /* tp_members */
    0,                                         /* tp_getset */
    0,                                         /* tp_base */
    0,                                         /* tp_dict */
    0,                                         /* tp_descr_get */
    0,                                         /* tp_descr_set */
    0,                                         /* tp_dictoffset */
    (initproc)StringRegexp_init,                /* tp_init */
    0,                                         /* tp_alloc */
    StringRegexp_new,                           /* tp_new */
};
