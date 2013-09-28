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

        memset(p, 0, sizeof (strscanner));
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

/*
 * Reset the scan pointer (index 0) and clear matching data.
 */
static PyObject *
StringScanner_reset(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    p->curr = 0;
    CLEAR_MATCH_STATUS(p);
    return (PyObject *)self;
}

/*
 * call-seq:
 *   terminate
 *   clear
 *
 * Set the scan pointer to the end of the string and clear matching data.
 */
static PyObject *
StringScanner_terminate(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    p->curr = S_LEN(p);
    CLEAR_MATCH_STATUS(p);
    return (PyObject *)self;
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
    } else {
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

/*
 * call-seq: match?(pattern)
 *
 * Tests whether the given +pattern+ is matched from the current scan pointer.
 * Returns the length of the match, or +nil+.  The scan pointer is not advanced.
 *
 *   s = StringScanner.new('test string')
 *   p s.match?(/\w+/)   # -> 4
 *   p s.match?(/\w+/)   # -> 4
 *   p s.match?(/\s+/)   # -> nil
 */
static PyObject *
StringScanner_match_p(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 0, 0, 1);
}

/*
 * call-seq: skip(pattern)
 *
 * Attempts to skip over the given +pattern+ beginning with the scan pointer.
 * If it matches, the scan pointer is advanced to the end of the match, and the
 * length of the match is returned.  Otherwise, +nil+ is returned.
 *
 * It's similar to #scan, but without returning the matched string.
 *
 *   s = StringScanner.new('test string')
 *   p s.skip(/\w+/)   # -> 4
 *   p s.skip(/\w+/)   # -> nil
 *   p s.skip(/\s+/)   # -> 1
 *   p s.skip(/\w+/)   # -> 6
 *   p s.skip(/./)     # -> nil
 *
 */
static PyObject *
StringScanner_skip(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 1, 0, 1);
}

/*
 * call-seq: check(pattern)
 *
 * This returns the value that #scan would return, without advancing the scan
 * pointer.  The match register is affected, though.
 *
 *   s = StringScanner.new("Fri Dec 12 1975 14:39")
 *   s.check /Fri/               # -> "Fri"
 *   s.pos                       # -> 0
 *   s.matched                   # -> "Fri"
 *   s.check /12/                # -> nil
 *   s.matched                   # -> nil
 *
 * Mnemonic: it "checks" to see whether a #scan will return a value.
 */
static PyObject *
StringScanner_check(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 0, 1, 1);
}

/*
 * call-seq: scan_full(pattern, advance_pointer_p, return_string_p)
 *
 * Tests whether the given +pattern+ is matched from the current scan pointer.
 * Advances the scan pointer if +advance_pointer_p+ is true.
 * Returns the matched string if +return_string_p+ is true.
 * The match register is affected.
 *
 * "full" means "#scan with full parameters".
 */
static PyObject *
StringScanner_scan_full(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    PyObject *py_s;
    PyObject *py_f;
    int s = 0;
    int f = 0;
    if (!PyArg_ParseTuple(args, "O!|O!O!", &scanner_StringRegexpType, &regexp, &PyBool_Type, &py_s, &PyBool_Type, &py_f))
        return NULL;
    if (py_s == Py_True)
        s = 1;
    if (py_f == Py_True)
        f = 1;
    return strscan_do_scan(self, (StringRegexp *)regexp, s, f, 1);
}

/*
 * call-seq: scan_until(pattern)
 *
 * Scans the string _until_ the +pattern+ is matched.  Returns the substring up
 * to and including the end of the match, advancing the scan pointer to that
 * location. If there is no match, +nil+ is returned.
 *
 *   s = StringScanner.new("Fri Dec 12 1975 14:39")
 *   s.scan_until(/1/)        # -> "Fri Dec 1"
 *   s.pre_match              # -> "Fri Dec "
 *   s.scan_until(/XYZ/)      # -> nil
 */
static PyObject *
StringScanner_scan_until(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 1, 1, 0);
}

/*
 * call-seq: exist?(pattern)
 *
 * Looks _ahead_ to see if the +pattern+ exists _anywhere_ in the string,
 * without advancing the scan pointer.  This predicates whether a #scan_until
 * will return a value.
 *
 *   s = StringScanner.new('test string')
 *   s.exist? /s/            # -> 3
 *   s.scan /test/           # -> "test"
 *   s.exist? /s/            # -> 2
 *   s.exist? /e/            # -> nil
 */
static PyObject *
StringScanner_exist_p(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 0, 0, 0);
}

/*
 * call-seq: skip_until(pattern)
 *
 * Advances the scan pointer until +pattern+ is matched and consumed.  Returns
 * the number of bytes advanced, or +nil+ if no match was found.
 *
 * Look ahead to match +pattern+, and advance the scan pointer to the _end_
 * of the match.  Return the number of characters advanced, or +nil+ if the
 * match was unsuccessful.
 *
 * It's similar to #scan_until, but without returning the intervening string.
 *
 *   s = StringScanner.new("Fri Dec 12 1975 14:39")
 *   s.skip_until /12/           # -> 10
 *   s                           #
 */
static PyObject *
StringScanner_skip_until(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 1, 0, 0);
}


/*
 * call-seq: check_until(pattern)
 *
 * This returns the value that #scan_until would return, without advancing the
 * scan pointer.  The match register is affected, though.
 *
 *   s = StringScanner.new("Fri Dec 12 1975 14:39")
 *   s.check_until /12/          # -> "Fri Dec 12"
 *   s.pos                       # -> 0
 *   s.matched                   # -> 12
 *
 * Mnemonic: it "checks" to see whether a #scan_until will return a value.
 */
static PyObject *
StringScanner_check_until(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    if (!PyArg_ParseTuple(args, "O!", &scanner_StringRegexpType, &regexp))
        return NULL;
    return strscan_do_scan(self, (StringRegexp *)regexp, 0, 1, 0);
}

/*
 * call-seq: search_full(pattern, advance_pointer_p, return_string_p)
 *
 * Scans the string _until_ the +pattern+ is matched.
 * Advances the scan pointer if +advance_pointer_p+, otherwise not.
 * Returns the matched string if +return_string_p+ is true, otherwise
 * returns the number of bytes advanced.
 * This method does affect the match register.
 */
static PyObject *
StringScanner_search_full(StringScanner *self, PyObject *args)
{
    PyObject *regexp;
    PyObject *py_s;
    PyObject *py_f;
    int s = 0;
    int f = 0;
    if (!PyArg_ParseTuple(args, "O!|O!O!", &scanner_StringRegexpType, &regexp, &PyBool_Type, &py_s, &PyBool_Type, &py_f))
        return NULL;
    if (py_s == Py_True)
        s = 1;
    if (py_f == Py_True)
        f = 1;
    return strscan_do_scan(self, (StringRegexp *)regexp, s, f, 0);
}

/*
 * call-seq: peek(len)
 *
 * Extracts a string corresponding to <tt>string[pos,len]</tt>, without
 * advancing the scan pointer.
 *
 *   s = StringScanner.new('test string')
 *   s.peek(7)          # => "test st"
 *   s.peek(7)          # => "test st"
 *
 */
static PyObject *
StringScanner_peek(StringScanner *self, PyObject *args)
{
    strscanner *p;
    PyObject *vlen;
    long len;

    if (!PyArg_ParseTuple(args, "O!", &PyInt_Type, &vlen))
        return NULL;

    p = self->p;
    len = PyInt_AS_LONG(vlen);
    if (EOS_P(p))
        return infect(str_new(p, "", 0), p);

    if (p->curr + len > S_LEN(p))
        len = S_LEN(p) - p->curr;
    return extract_beg_len(p, p->curr, len);
}

/*
 * Set the scan pointer to the previous position.  Only one previous position is
 * remembered, and it changes with each scanning operation.
 *
 *   s = StringScanner.new('test string')
 *   s.scan(/\w+/)        # => "test"
 *   s.unscan
 *   s.scan(/../)         # => "te"
 *   s.scan(/\d/)         # => nil
 *   s.unscan             # ScanError: unscan failed: previous match record not exist
 */
static PyObject *
StringScanner_unscan(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    if (!MATCHED_P(p)) {
        PyErr_SetString(ScanError, "unscan failed: previous match record not exist");
        return NULL;
    }
    p->curr = p->prev;
    CLEAR_MATCH_STATUS(p);
    return (PyObject *)self;
}

/*
 * Returns +true+ iff the scan pointer is at the beginning of the line.
 *
 *   s = StringScanner.new("test\ntest\n")
 *   s.bol?           # => true
 *   s.scan(/te/)
 *   s.bol?           # => false
 *   s.scan(/st\n/)
 *   s.bol?           # => true
 *   s.terminate
 *   s.bol?           # => true
 */
static PyObject *
StringScanner_bol_p(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    if (CURPTR(p) > S_PEND(p)) return Py_None;
    if (p->curr == 0) return Py_True;
    return (*(CURPTR(p) - 1) == '\n') ? Py_True : Py_False;
}

/*
 * Returns +true+ if the scan pointer is at the end of the string.
 *
 *   s = StringScanner.new('test string')
 *   p s.eos?          # => false
 *   s.scan(/test/)
 *   p s.eos?          # => false
 *   s.terminate
 *   p s.eos?          # => true
 */
static PyObject *
StringScanner_eos_p(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    return EOS_P(p) ? Py_True : Py_False;
}

/*
 * Returns true iff there is more data in the string.  See #eos?.
 * This method is obsolete; use #eos? instead.
 *
 *   s = StringScanner.new('test string')
 *   s.eos?              # These two
 *   s.rest?             # are opposites.
 */
static PyObject *
StringScanner_rest_p(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    return EOS_P(p) ? Py_False : Py_True;
}

/*
 * Returns +true+ iff the last match was successful.
 *
 *   s = StringScanner.new('test string')
 *   s.match?(/\w+/)     # => 4
 *   s.matched?          # => true
 *   s.match?(/\d+/)     # => nil
 *   s.matched?          # => false
 */
static PyObject *
StringScanner_matched_p(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    return MATCHED_P(p) ? Py_True : Py_False;
}

/*
 * Returns the last matched string.
 *
 *   s = StringScanner.new('test string')
 *   s.match?(/\w+/)     # -> 4
 *   s.matched           # -> "test"
 */
static PyObject *
StringScanner_matched(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    if (!MATCHED_P(p)) return Py_None;
    return extract_range(p, p->prev + p->regs.beg[0],
                            p->prev + p->regs.end[0]);
}

/*
 * Returns the size of the most recent match (see #matched), or +nil+ if there
 * was no recent match.
 *
 *   s = StringScanner.new('test string')
 *   s.check /\w+/           # -> "test"
 *   s.matched_size          # -> 4
 *   s.check /\d+/           # -> nil
 *   s.matched_size          # -> nil
 */
static PyObject *
StringScanner_matched_size(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    if (!MATCHED_P(p)) return Py_None;
    return PyInt_FromLong(p->regs.end[0] - p->regs.beg[0]);
}

static PyMethodDef StringScanner_methods[] = {
    {"reset", (PyCFunction)StringScanner_scan, METH_NOARGS, "reset"},
    {"terminate", (PyCFunction)StringScanner_terminate, METH_NOARGS, "terminate"},
    {"scan", (PyCFunction)StringScanner_scan, METH_VARARGS, "scan"},
    {"match_count", (PyCFunction)StringScanner_match_p, METH_VARARGS, "match_count"},
    {"skip", (PyCFunction)StringScanner_skip, METH_VARARGS, "skip"},
    {"check", (PyCFunction)StringScanner_check, METH_VARARGS, "check"},
    {"scan_full", (PyCFunction)StringScanner_scan_full, METH_VARARGS, "scan_full"},
    {"scan_until", (PyCFunction)StringScanner_scan_until, METH_VARARGS, "scan_until"},
    {"skip_until", (PyCFunction)StringScanner_skip_until, METH_VARARGS, "skip_until"},
    {"check_until", (PyCFunction)StringScanner_check_until, METH_VARARGS, "check_until"},
    {"search_full", (PyCFunction)StringScanner_search_full, METH_VARARGS, "search_full"},
    {"peek", (PyCFunction)StringScanner_peek, METH_VARARGS, "peek"},
    {"unscan", (PyCFunction)StringScanner_unscan, METH_NOARGS, "scan"},
    {NULL}  /* Sentinel */
};

static PyObject *
StringScanner_pos__get__(StringScanner *self)
{
    strscanner *p = self->p;
    return PyInt_FromLong(p->curr);
}

static PyObject *
StringScanner_pos__set__(StringScanner *self, PyObject *v)
{
    strscanner *p;
    long i;

    p = self->p;
    i = PyInt_AS_LONG(v);
    if (i < 0) i += S_LEN(p);
    if (i < 0) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }
    if (i > S_LEN(p)) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }
    p->curr = i;
    return PyInt_FromLong(i);
}

/*
 * Returns the string being scanned.
 */
static PyObject *
StringScanner_string__get__(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    return p->str;
}

/*
 * call-seq: string=(str)
 *
 * Changes the string being scanned to +str+ and resets the scanner.
 * Returns +str+.
 */
static PyObject *
StringScanner_string__set__(StringScanner *self, PyObject *str)
{
    // TODO: string setter
    //struct strscanner *p = check_strscan(self);

    //StringValue(str);
    //p->str = str;
    //p->curr = 0;
    //CLEAR_MATCH_STATUS(p);
    return str;
}

/*
 * Returns the "rest" of the string (i.e. everything after the scan pointer).
 * If there is no more data (eos? = true), it returns <tt>""</tt>.
 */
static PyObject *
StringScanner_rest__get__(StringScanner *self)
{
    strscanner *p;

    p = self->p;
    if (EOS_P(p)) {
        return infect(str_new(p, "", 0), p);
    }
    return extract_range(p, p->curr, S_LEN(p));
}

/*
 * <tt>s.rest_size</tt> is equivalent to <tt>s.rest.size</tt>.
 */
static PyObject *
StringScanner_rest_size__get__(StringScanner *self)
{
    strscanner *p;
    long i;

    p = self->p;
    if (EOS_P(p)) {
        return PyInt_FromLong(0);
    }
    i = S_LEN(p) - p->curr;
    return PyInt_FromLong(i);
}

static PyGetSetDef StringScanner_getsetter[] = {
    {"pos", (getter) StringScanner_pos__get__, (setter) StringScanner_pos__set__, "pos", NULL},
    {"bol", (getter) StringScanner_bol_p, NULL, "bol", NULL},
    {"eos", (getter) StringScanner_eos_p, NULL, "eos", NULL},
    {"is_matched", (getter) StringScanner_matched_p, NULL, "is_matched", NULL},
    {"matched", (getter) StringScanner_matched, NULL, "matched", NULL},
    {"matched_size", (getter) StringScanner_matched_size, NULL, "matched_size", NULL},
    {"exist", (getter) StringScanner_exist_p, NULL, "exist", NULL},
    {"string", (getter) StringScanner_string__get__, NULL, "string", NULL},
    {"is_rest", (getter) StringScanner_rest_p, NULL, "is_rest", NULL},
    {"rest", (getter) StringScanner_rest__get__, NULL, "rest", NULL},
    {"rest_size", (getter) StringScanner_rest_size__get__, NULL, "rest_size", NULL},
    {NULL}
};

static PyMemberDef StringScanner_members[] = {
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
    StringScanner_getsetter,                   /* tp_getset */
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
