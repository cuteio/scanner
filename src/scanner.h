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

#ifdef __cplusplus
}
#endif
#endif  /* !defined(PY_SCANNER_H) */
