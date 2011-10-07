/*
    Copyright (C) 2011  Nickolas Fotopoulos

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <stdlib.h>

#include <Python.h>
#include <numpy/arrayobject.h>

#include <lal/Sort.h>

static int rankdata(
    const double * restrict const array,  /* input array */
    const size_t len,            /* length of input and output arrays */
    double * restrict const out_array     /* output array */
    );
static PyObject *pylal_stats_rankdata(PyObject *self, PyObject *args);

const char rankdata_docstring[] = "C implementation of scipy.stats.rankdata\n\n"
"Ranks the data in a, dealing with ties appropriately.\n"
"\n"
"Equal values are assigned a rank that is the average of the ranks that\n"
"would have been otherwise assigned to all of the values within that set.\n"
"Ranks begin at 1, not 0.\n"
"\n"
"Example\n"
"-------\n"
"In [15]: stats.rankdata([0, 2, 2, 3])\n"
"Out[15]: array([ 1. ,  2.5,  2.5,  4. ])\n"
"\n"
"Parameters\n"
"----------\n"
"a : array\n";

/* comparison function for argsort */
static int cmp(void *p, const void *a, const void *b) {
    const double c = *((const double *)a);
    const double d = *((const double *)b);
    return (c < d) ? -1 : (c > d);
}

static int rankdata(
    const double * restrict const array, /* input array */
    const size_t len,                    /* length of input and output arrays */
    double * restrict const out_array    /* output array */
    ) {
    size_t i, j;
    INT4 *ind_array;
    double sumranks = 0.0;
    size_t dupcount = 0;

    /* allocate temporary ind_array */
    ind_array = (INT4 *)malloc(len * sizeof(INT4));
    if (!ind_array) return 1;
    memset(ind_array, (INT4) 0, len);

    /* argsort */
    XLALHeapIndex(ind_array, array, len, sizeof(double), NULL, cmp);

    /* assign rankings, averaging ties */
    for (i = 0; i < len; i++) {
        sumranks += i;
        dupcount += 1;
        if ((i == len - 1) || (array[ind_array[i]] != array[ind_array[i + 1]])) {
            for (j = i - dupcount + 1; j <= i; j++)
                out_array[ind_array[j]] = sumranks / (float) dupcount + 1;
            sumranks = 0.;
            dupcount = 0;
        }
    }

    /* clean up */
    free(ind_array);

    return 0;
}

static PyObject *pylal_stats_rankdata(PyObject *self, PyObject *args) {
    PyObject *in_arr, *out_arr;
    int must_clean_up = 0;
    npy_intp dims[1] = {0};

    if (!PyArg_ParseTuple(args, "O", &in_arr)) return NULL;
    if (!PyArray_Check(in_arr) || (PyArray_TYPE(in_arr) != NPY_DOUBLE)
        || !PyArray_ISCARRAY_RO(in_arr)) {
        in_arr = PyArray_FROMANY(in_arr, NPY_DOUBLE, 1, 1, NPY_CARRAY_RO);
        must_clean_up = 1;
    }
    dims[0] = PyArray_SIZE(in_arr);
    out_arr = PyArray_SimpleNew(1, dims, NPY_DOUBLE);

    if (rankdata((double *) PyArray_DATA(in_arr), dims[0], (double *) PyArray_DATA(out_arr))) {
        PyErr_SetString(PyExc_ValueError, "rankdata failed");
        if (must_clean_up) { Py_DECREF(in_arr); }
        Py_DECREF(out_arr);
        return NULL;
    }

    if (must_clean_up) { Py_DECREF(in_arr); }
    return out_arr;
}

static struct PyMethodDef methods[] = {
    {"rankdata", pylal_stats_rankdata, METH_VARARGS, rankdata_docstring}
};

void init_stats(void) {
    (void) Py_InitModule3("pylal._stats", methods, "Some statistical routines that I'd like to run quickly\n");
    import_array();
}
