#ifndef PY_SCANNER_H
#define PY_SCANNER_H

#include "oniguruma.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* multi-purpose flags */
    unsigned long flags;
#define FLAG_MATCHED (1 << 0)

    /* the string to scan */
    PyObject *str;

    /* scan pointers */
    long prev;   /* legal only when MATCHED_P(s) */
    long curr;   /* always legal */

    /* the regexp register; legal only when MATCHED_P(s) */
    //struct re_registers regs;
    OnigRegion regs;

    /* regexp used for last scan */
    regex_t *regex;

    int unicode;
} strscanner;

typedef struct {
    PyObject_HEAD
    regex_t *regex;
    PyObject *pattern;
    int unicode;
} StringRegexp;

typedef struct {
    PyObject_HEAD
    strscanner *p;
} StringScanner;

int regexp_init(StringRegexp *, PyObject *);
void regexp_delloc(StringRegexp *);
int init_python_syntax(void);

#ifdef __cplusplus
}
#endif

#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
            PyObject_HEAD_INIT(type) size,
#endif

#if PY_MAJOR_VERSION >= 3
#define PyInt_Type PyLong_Type
#define PyInt_AS_LONG PyLong_AS_LONG
#else
#define PyLong_FromLong PyInt_FromLong
#define PyBytes_FromStringAndSizePy String_FromStringAndSize
#define PyBytes_Check PyString_Check
#define PyBytes_GET_SIZE PyString_GET_SIZE
#define PyBytes_AS_STRING PyString_AS_STRING
#endif

#endif  /* !defined(PY_SCANNER_H) */
