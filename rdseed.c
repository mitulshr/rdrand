/*
 * Copyright (c) 2013, Chris Stillson <stillson@gmail.com>
 * All rights reserved.
 *
 * portions of this code
 * Copyright � 2012, Intel Corporation.  All rights reserved.
 * (Namely, the cpuid checking code)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define MAX_RETRY 100
#ifdef _LP64
#define IS64BIT 1
#else
#define IS32BIT 1
#endif

/* Windows Specific */
#ifdef _WIN32
#if (_MSC_VER >= 1800) /* Visual Studio 2013 Onwards */
#include <immintrin.h>
#include <string.h>
#else
#error Microsoft compiler started supporting RDSEED
#error instructions from 2013 version and onwards
#endif
/* Windows */
/* GNUC Specific */
#elif __GNUC__
#ifdef __INTEL_COMPILER
# include <immintrin.h>
#endif
#define USING_GCC 1
#include <stdint.h>
#include <string.h>
/* GNUC */
#else
#error Your compiler is not supported currently
#error if you port to another compiler, please
#error send back the patch to https://github.com/stillson/rdrand
#endif

/*
#if __clang__
#define USING_CLANG 1
#else
#error Only support for gcc or clang currently
#error if you port to another compiler, please
#error send back the patch to https://github.com/stillson/rdrand
#endif
*/

#if PY_MAJOR_VERSION == 2
#define PYTHON2 1
#elif PY_MAJOR_VERSION == 3
#define PYTHON3 1
#else
#error requires python 2 or 3
#endif



#if USING_GCC
# define _rdseed64_step(x) ({ unsigned char err; asm volatile(".byte 0x48; .byte 0x0f; .byte 0xc7; .byte 0xf8; setc %1":"=a"(x), "=qm"(err)); err; })
# define _rdseed32_step(x) ({ unsigned char err; asm volatile(".byte 0x0f; .byte 0xc7; .byte 0xf8; setc %1":"=a"(x), "=qm"(err)); err; })
#endif



uint64_t get_bits(int);
int RdSeed_cpuid(void);

PyDoc_STRVAR(module_doc, "rdseed: Python interface to intel hardware rng\n");

/*! \brief Queries cpuid to see if rdseed is supported
 *
 * rdseed support in a CPU is determined by examining the 30th bit of the ecx
 * register after calling cpuid.
 *
 * \return bool of whether or not rdseed is supported
 */

/*! \def RDRAND_MASK
 *    The bit mask used to examine the ecx register returned by cpuid. The
 *   30th bit is set.
 */
#define RDSEED_MASK   0x40000

#ifdef __GNUC__
# define __cpuid(x,y,z) asm volatile("cpuid":"=a"(x[0]),"=b"(x[1]),"=c"(x[2]),"=d"(x[3]):"a"(y),"c"(z))
#endif

int 
RdSeed_cpuid()
{
	/* Are we on an Intel processor? */
	unsigned int info[4] = { -1, -1, -1, -1 };
#ifdef _WIN32
	__cpuid(info, /*feature bits*/0);
#endif

#ifdef __GNUC__
	__cpuid(info, /*feature bits*/0, 0);
#endif
	if (memcmp((void *)&info[1], (void *) "Genu", 4) != 0 ||
	    memcmp((void *)&info[3], (void *) "ineI", 4) != 0 ||
	    memcmp((void *)&info[2], (void *) "ntel", 4) != 0) {
		return 0;
	}

	/* Do we have RDSEED? */
	unsigned int info_rdseed[4] = { -1, -1, -1, -1 };
#ifdef _WIN32
	__cpuid(info_rdseed, /*feature bits*/7);
#endif

#ifdef __GNUC__
	__cpuid(info_rdseed, /*feature bits*/7, 0);
#endif
	unsigned int ebx = info_rdseed[1];
	if ((ebx & RDSEED_MASK) == RDSEED_MASK)
		return 1;
	else
		return 0;
}


#if IS64BIT
//utility to return 64 random bits
uint64_t
get_bits(int not_force)
{
    unsigned long int seed = 0;
    int i = 0;
    if(not_force == 1) 
    { 
	 while( ! _rdseed64_step(seed) && i < MAX_RETRY)
	 {
		 i++;
	 }
	 if(i >= MAX_RETRY) return -1;
    }
    else 
    {
	while(! _rdseed64_step(seed));
    }
    return seed;
}
#elif IS32BIT
uint64_t
get_bits(int not_force)
{
    int it = 0;
    union{
       uint64_t seed;
       struct {
          uint32_t seed1;
          uint32_t seed2;
       } i;
    } un;

    if(not_force == 1) 
    {
       while( ! _rdseed32_step(un.i.seed1) && it < MAX_RETRY)
       {
	 it++;
       }
       if(it >= MAX_RETRY) return -1;

       it = 0;
       while( ! _rdseed32_step(un.i.seed2) && it < MAX_RETRY)
       {
	 it++;
       }
       if(it >= MAX_RETRY) return -1;

    }
    else
    {
         while(! _rdseed32_step(un.i.seed1));
         while(! _rdseed32_step(un.i.seed2));
    }

    return un.seed;
}
#endif

static PyObject *
rdseed_get_bits(PyObject *self, PyObject *args)
{
    int num_bits, num_bytes, i;
    int num_quads, num_chars;
    int not_force = 0;
    unsigned char * data = NULL;
    uint64_t seed;
    unsigned char last_mask, lm_shift;
    PyObject *result;

    if ( !PyArg_ParseTuple(args, "i|i", &num_bits, &not_force) )
        return NULL;

    
    if (num_bits <= 0)
    {
        PyErr_SetString(PyExc_ValueError, "number of bits must be greater than zero");
        return NULL;
    }
    
    if (not_force != 0 && not_force != 1)
    {
        PyErr_SetString(PyExc_ValueError, "force flag can take only value of 1. By deafult it will force, value of 1 will not force\n");
        return NULL;
    }

    
    num_bytes   = num_bits / 8;
    lm_shift    = num_bits % 8;
    last_mask   = 0xff >> (8 - lm_shift);

    if (lm_shift)
        num_bytes++;

    num_quads   = num_bytes / 8;
    num_chars   = num_bytes % 8;
    data        = (unsigned char *)PyMem_Malloc(num_bytes);

    if (data == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }

    for(i = 0; i < num_quads; i++)
    {	
	if(not_force == 0)
	   seed = get_bits(0);
	else
	   seed = get_bits(1);

        bcopy((char*)&seed, &data[i * 8], 8);
    }

    if(num_chars)
    {
        if(not_force == 0)
	   seed = get_bits(0);
	else
	   seed = get_bits(1);
        
	bcopy((char*)&seed, &data[num_quads * 8], num_chars);
    }

    if (lm_shift != 0)
        data[num_bytes -1] &= last_mask;

    /* Probably hosing byte order. big deal it's hardware random, has no meaning til we assign it */
    result = _PyLong_FromByteArray(data, num_bytes, 1, 0);
    PyMem_Free(data);
    return result;
}

static PyObject *
rdseed_get_bytes(PyObject *self, PyObject *args)
{
    int num_bytes, i;
    int not_force = 0;
    int num_quads, num_chars;
    unsigned char * data = NULL;
    uint64_t seed;
    PyObject *result;
    
    if ( !PyArg_ParseTuple(args, "i|i", &num_bytes, &not_force) )
        return NULL;

    if (num_bytes <= 0)
    {
        PyErr_SetString(PyExc_ValueError, "number of bytes must be greater than zero");
        return NULL;
    }

    if(not_force != 0 && not_force != 1)
    {
        PyErr_SetString(PyExc_ValueError, "force flag can take only value of 1. By deafult it will force, value of 1 will not force\n");
	return NULL;
    }

    data        = (unsigned char *)PyMem_Malloc(num_bytes);
    num_quads   = num_bytes / 8;
    num_chars   = num_bytes % 8;

    if (data == NULL)
    {
        PyErr_NoMemory();
        return NULL;
    }

    for(i = 0; i < num_quads; i++)
    {
        if(not_force == 0)
	   seed = get_bits(0);
	else
	   seed = get_bits(1);
        bcopy((char*)&seed, &data[i * 8], 8);
    }

    if(num_chars)
    {
	if(not_force == 0)
	   seed = get_bits(0);
	else
	   seed = get_bits(1);
        
	bcopy((char*)&seed, &data[num_quads * 8], num_chars);
    }

    /* Probably hosing byte order. big deal it's hardware random, has no meaning til we assign it */
#if PYTHON2 == 1
    result = Py_BuildValue("s#", data, num_bytes);
#else
    result = Py_BuildValue("y#", data, num_bytes);
#endif
    PyMem_Free(data);
    return result;
}

static PyMethodDef rdseed_functions[] = {
        {"rdseed_get_bits",       rdseed_get_bits,        METH_VARARGS, "rdseed_get_bits()"},
        {"rdseed_get_bytes",      rdseed_get_bytes,       METH_VARARGS, "rdseed_get_bytes()"},
        {NULL, NULL, 0, NULL}   /* Sentinel */
};

#if PYTHON2 == 1
PyMODINIT_FUNC
init_rdseed(void)
{
        PyObject *m;

        // I need to verify that cpu type can do rdseed
        if (RdSeed_cpuid() != 1)
        {
            PyErr_SetString(PyExc_SystemError,
                        "CPU doesn't have random seed generator");
            return;
        }

        m = Py_InitModule3("_rdseed", rdseed_functions, module_doc);
        if (m == NULL)
            return;
}
#else
static struct PyModuleDef rdseedmodule = {
   PyModuleDef_HEAD_INIT, "_rdseed", module_doc, -1, rdseed_functions
};

PyMODINIT_FUNC
PyInit__rdseed(void)
{
        PyObject *m;

        // I need to verify that cpu type can do rdseed
        if (RdSeed_cpuid() != 1)
        {
            PyErr_SetString(PyExc_SystemError,
                        "CPU doesn't have random seed generator");
            return NULL;
        }

        m = PyModule_Create(&rdseedmodule);
        if (m == NULL)
            return NULL;

        return m;
}
#endif
