/******************************************************************************
 * Project:  PROJ.4
 * Purpose:  Mutex (thread lock) functions.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2009, Frank Warmerdam
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/


/* projects.h and windows.h conflict - avoid this! */

#if defined(MUTEX_pthread) && !defined(_XOPEN_SOURCE)
// For pthread_mutexattr_settype
#define _XOPEN_SOURCE 500
#endif

#ifndef _WIN32
#include <projects.h>
#else
#include <proj_api.h>
#endif

/* on win32 we always use win32 mutexes, even if pthreads are available */
#if defined(_WIN32) && !defined(MUTEX_stub)
#  define MUTEX_win32
#  undef  MUTEX_pthread
#endif

#if !defined(MUTEX_stub) && !defined(MUTEX_pthread) && !defined(MUTEX_win32)
#  define MUTEX_stub
#endif

/************************************************************************/
/* ==================================================================== */
/*                      stub mutex implementation                       */
/* ==================================================================== */
/************************************************************************/

#ifdef MUTEX_stub

/************************************************************************/
/*                            pj_acquire_lock()                         */
/*                                                                      */
/*      Acquire the PROJ.4 lock.                                        */
/************************************************************************/

void pj_acquire_lock()
{
}

/************************************************************************/
/*                            pj_release_lock()                         */
/*                                                                      */
/*      Release the PROJ.4 lock.                                        */
/************************************************************************/

void pj_release_lock()
{
}

/************************************************************************/
/*                          pj_cleanup_lock()                           */
/************************************************************************/
void pj_cleanup_lock()
{
}

/************************************************************************/
/*                     pj_acquire_lock_default_ctxt()                   */
/************************************************************************/

void pj_acquire_lock_default_ctxt()
{
}

/************************************************************************/
/*                     pj_release_lock_default_ctxt()                   */
/************************************************************************/

void pj_release_lock_default_ctxt()
{
}

/************************************************************************/
/*                     pj_acquire_lock_initcache()                      */
/************************************************************************/

void pj_acquire_lock_initcache()
{
}

/************************************************************************/
/*                     pj_release_lock_initcache()                      */
/************************************************************************/

void pj_release_lock_initcache()
{
}

/************************************************************************/
/*                     pj_acquire_lock_gridinfo()                       */
/************************************************************************/

void pj_acquire_lock_gridinfo()
{
}

/************************************************************************/
/*                     pj_release_lock_gridinfo()                       */
/************************************************************************/

void pj_release_lock_gridinfo()
{
}

/************************************************************************/
/*                     pj_acquire_lock_gridlist()                       */
/************************************************************************/

void pj_acquire_lock_gridlist()
{
}

/************************************************************************/
/*                     pj_release_lock_gridlist()                       */
/************************************************************************/

void pj_release_lock_gridlist()
{
}

/************************************************************************/
/*                    pj_acquire_lock_gridcatalog()                     */
/************************************************************************/

void pj_acquire_lock_gridcatalog()
{
}

/************************************************************************/
/*                    pj_release_lock_gridcatalog()                     */
/************************************************************************/

void pj_release_lock_gridcatalog()
{
}

#endif // def MUTEX_stub

/************************************************************************/
/* ==================================================================== */
/*                    pthread mutex implementation                      */
/* ==================================================================== */
/************************************************************************/

#ifdef MUTEX_pthread

#include "pthread.h"

static pthread_mutex_t pj_precreated_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pj_core_lock;
static int pj_core_lock_created = 0;

/************************************************************************/
/*                          pj_acquire_lock()                           */
/*                                                                      */
/*      Acquire the PROJ.4 lock.                                        */
/************************************************************************/

void pj_acquire_lock()
{
    if (!pj_core_lock_created) {
        /*
        ** We need to ensure the core mutex is created in recursive mode
        ** and there is no portable way of doing that using automatic
        ** initialization so we have pj_precreated_lock only for the purpose
        ** of protecting the creation of the core lock.
        */
        pthread_mutexattr_t mutex_attr;

        pthread_mutex_lock( &pj_precreated_lock);

        pthread_mutexattr_init(&mutex_attr);
#ifndef PTHREAD_MUTEX_RECURSIVE
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
#else
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
#endif
        pthread_mutex_init(&pj_core_lock, &mutex_attr);
        pj_core_lock_created = 1;

        pthread_mutex_unlock( &pj_precreated_lock );
    }

    pthread_mutex_lock( &pj_core_lock);
}

/************************************************************************/
/*                          pj_release_lock()                           */
/*                                                                      */
/*      Release the PROJ.4 lock.                                        */
/************************************************************************/

void pj_release_lock()
{
    pthread_mutex_unlock( &pj_core_lock );
}

/************************************************************************/
/*                          pj_cleanup_lock()                           */
/************************************************************************/
void pj_cleanup_lock()
{
}

/************************************************************************/
/*                     pj_acquire_lock_default_ctxt()                   */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock for pj_get_default_ctx()                       */
/************************************************************************/

static pthread_mutex_t pj_default_ctxt_lock = PTHREAD_MUTEX_INITIALIZER;

void pj_acquire_lock_default_ctxt()
{
    pthread_mutex_lock( &pj_default_ctxt_lock);
}

/************************************************************************/
/*                     pj_release_lock_default_ctxt()                   */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock for pj_get_default_ctx()                       */
/************************************************************************/

void pj_release_lock_default_ctxt()
{
    pthread_mutex_unlock( &pj_default_ctxt_lock);
}

/************************************************************************/
/*                     pj_acquire_lock_initcache()                      */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_initcache.c                         */
/************************************************************************/

static pthread_mutex_t pj_lock_initcache = PTHREAD_MUTEX_INITIALIZER;

void pj_acquire_lock_initcache()
{
    pthread_mutex_lock( &pj_lock_initcache);
}

/************************************************************************/
/*                     pj_release_lock_initcache()                      */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_initcache.c                         */
/************************************************************************/

void pj_release_lock_initcache()
{
    pthread_mutex_unlock( &pj_lock_initcache);
}

/************************************************************************/
/*                     pj_acquire_lock_gridinfo()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_gridinfo.c                          */
/************************************************************************/

static pthread_mutex_t pj_lock_gridinfo = PTHREAD_MUTEX_INITIALIZER;

void pj_acquire_lock_gridinfo()
{
    pthread_mutex_lock( &pj_lock_gridinfo);
}

/************************************************************************/
/*                     pj_release_lock_gridinfo()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_gridinfo.c                          */
/************************************************************************/

void pj_release_lock_gridinfo()
{
    pthread_mutex_unlock( &pj_lock_gridinfo);
}

/************************************************************************/
/*                     pj_acquire_lock_gridlist()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_gridlist.c                          */
/************************************************************************/

static pthread_mutex_t pj_lock_gridlist = PTHREAD_MUTEX_INITIALIZER;

void pj_acquire_lock_gridlist()
{
    pthread_mutex_lock( &pj_lock_gridlist);
}

/************************************************************************/
/*                     pj_release_lock_gridlist()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_gridlist.c                          */
/************************************************************************/

void pj_release_lock_gridlist()
{
    pthread_mutex_unlock( &pj_lock_gridlist);
}

/************************************************************************/
/*                     pj_acquire_lock_gridcatalog()                    */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_gridcatalog.c                       */
/************************************************************************/

static pthread_mutex_t pj_lock_gridcatalog = PTHREAD_MUTEX_INITIALIZER;

void pj_acquire_lock_gridcatalog()
{
    pthread_mutex_lock( &pj_lock_gridcatalog);
}

/************************************************************************/
/*                     pj_release_lock_gridcatalog()                    */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_gridcatalog.c                       */
/************************************************************************/

void pj_release_lock_gridcatalog()
{
    pthread_mutex_unlock( &pj_lock_gridcatalog);
}

#endif // def MUTEX_pthread

/************************************************************************/
/* ==================================================================== */
/*                      win32 mutex implementation                      */
/* ==================================================================== */
/************************************************************************/

/* InitializeCriticalSectionAndSpinCount requires _WIN32_WINNT >= 0x403 */
#define _WIN32_WINNT 0x0500

#ifdef MUTEX_win32

#include <windows.h>

static void pj_cleanup_lock_default_ctxt();
static void pj_cleanup_lock_initcache();
static void pj_cleanup_lock_gridinfo();
static void pj_cleanup_lock_gridlist();
static void pj_cleanup_lock_gridcatalog();

/************************************************************************/
/*                         pj_spin_lock()                               */
/************************************************************************/

static volatile long pj_spin_flag = 0;
static void pj_spin_lock()
{
    while( InterlockedExchange(&pj_spin_flag, 1) )
    {
        while( pj_spin_flag )
        {
#ifdef YieldProcessor
            YieldProcessor();
#else
            Sleep(0);
#endif
        }
    }
}

/************************************************************************/
/*                         pj_spin_unlock()                             */
/************************************************************************/

static void pj_spin_unlock()
{
    pj_spin_flag = 0;
}

/************************************************************************/
/*                          pj_acquire_lock()                           */
/*                                                                      */
/*      Acquire the PROJ.4 lock.                                        */
/************************************************************************/

static HANDLE pj_mutex_lock = NULL;

void pj_acquire_lock()
{
    pj_spin_lock();
    if( pj_mutex_lock == NULL )
        pj_mutex_lock = CreateMutex( NULL, FALSE, NULL );
    pj_spin_unlock();

    WaitForSingleObject( pj_mutex_lock, INFINITE );
}

/************************************************************************/
/*                          pj_release_lock()                           */
/*                                                                      */
/*      Release the PROJ.4 lock.                                        */
/************************************************************************/

void pj_release_lock()
{
    ReleaseMutex( pj_mutex_lock );
}

/************************************************************************/
/*                          pj_cleanup_lock()                           */
/************************************************************************/

void pj_cleanup_lock()
{
    if( pj_mutex_lock != NULL )
    {
        CloseHandle( pj_mutex_lock );
        pj_mutex_lock = NULL;
    }
    pj_cleanup_lock_default_ctxt();
    pj_cleanup_lock_initcache();
}

/************************************************************************/
/*                     pj_acquire_lock_default_ctxt()                   */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock for pj_get_default_ctx()                       */
/************************************************************************/

static CRITICAL_SECTION* pj_cs_default_ctxt = NULL;

void pj_acquire_lock_default_ctxt()
{
    pj_spin_lock();
    if( pj_cs_default_ctxt == NULL )
    {
        pj_cs_default_ctxt = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionAndSpinCount(pj_cs_default_ctxt, 4000);
    }
    pj_spin_unlock();

    EnterCriticalSection(pj_cs_default_ctxt);
}

/************************************************************************/
/*                     pj_release_lock_default_ctxt()                   */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock for pj_get_default_ctx()                       */
/************************************************************************/

void pj_release_lock_default_ctxt()
{
    LeaveCriticalSection(pj_cs_default_ctxt);
}

/************************************************************************/
/*                     pj_cleanup_lock_default_ctxt()                   */
/************************************************************************/

static void pj_cleanup_lock_default_ctxt()
{
    if( pj_cs_default_ctxt != NULL )
    {
        DeleteCriticalSection( pj_cs_default_ctxt );
        free( pj_cs_default_ctxt );
        pj_cs_default_ctxt = NULL;
    }
}

/************************************************************************/
/*                     pj_acquire_lock_initcache()                      */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_initcache.c                         */
/************************************************************************/

static CRITICAL_SECTION* pj_cs_initcache = NULL;

void pj_acquire_lock_initcache()
{
    pj_spin_lock();
    if( pj_cs_initcache == NULL )
    {
        pj_cs_initcache = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionAndSpinCount(pj_cs_initcache, 4000);
    }
    pj_spin_unlock();

    EnterCriticalSection(pj_cs_initcache);
}

/************************************************************************/
/*                     pj_release_lock_initcache()                      */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_initcache.c                         */
/************************************************************************/

void pj_release_lock_initcache()
{
    LeaveCriticalSection(pj_cs_initcache);
}

/************************************************************************/
/*                     pj_cleanup_lock_initcache()                      */
/************************************************************************/

static void pj_cleanup_lock_initcache()
{
    if( pj_cs_initcache != NULL )
    {
        DeleteCriticalSection( pj_cs_initcache );
        free( pj_cs_initcache );
        pj_cs_initcache = NULL;
    }
}

/************************************************************************/
/*                     pj_acquire_lock_gridinfo()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_gridinfo.c                          */
/************************************************************************/

static CRITICAL_SECTION* pj_cs_gridinfo = NULL;

void pj_acquire_lock_gridinfo()
{
    pj_spin_lock();
    if( pj_cs_gridinfo == NULL )
    {
        pj_cs_gridinfo = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionAndSpinCount(pj_cs_gridinfo, 4000);
    }
    pj_spin_unlock();

    EnterCriticalSection(pj_cs_gridinfo);
}

/************************************************************************/
/*                     pj_release_lock_gridinfo()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_gridinfo.c                          */
/************************************************************************/

void pj_release_lock_gridinfo()
{
    LeaveCriticalSection(pj_cs_gridinfo);
}

/************************************************************************/
/*                     pj_cleanup_lock_gridinfo()                       */
/************************************************************************/

static void pj_cleanup_lock_gridinfo()
{
    if( pj_cs_gridinfo != NULL )
    {
        DeleteCriticalSection( pj_cs_gridinfo );
        free( pj_cs_gridinfo );
        pj_cs_gridinfo = NULL;
    }
}

/************************************************************************/
/*                     pj_acquire_lock_gridlist()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_gridlist.c                          */
/************************************************************************/

static CRITICAL_SECTION* pj_cs_gridlist = NULL;

void pj_acquire_lock_gridlist()
{
    pj_spin_lock();
    if( pj_cs_gridlist == NULL )
    {
        pj_cs_gridlist = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionAndSpinCount(pj_cs_gridlist, 4000);
    }
    pj_spin_unlock();

    EnterCriticalSection(pj_cs_gridlist);
}

/************************************************************************/
/*                     pj_release_lock_gridlist()                       */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_gridlist.c                          */
/************************************************************************/

void pj_release_lock_gridlist()
{
    LeaveCriticalSection(pj_cs_gridlist);
}

/************************************************************************/
/*                     pj_cleanup_lock_gridlist()                       */
/************************************************************************/

static void pj_cleanup_lock_gridlist()
{
    if( pj_cs_gridlist != NULL )
    {
        DeleteCriticalSection( pj_cs_gridlist );
        free( pj_cs_gridlist );
        pj_cs_gridlist = NULL;
    }
}

/************************************************************************/
/*                    pj_acquire_lock_gridcatalog()                     */
/*                                                                      */
/*      Internal function.                                              */
/*      Acquire the lock used in pj_gridcatalog.c                       */
/************************************************************************/

static CRITICAL_SECTION* pj_cs_gridcatalog = NULL;

void pj_acquire_lock_gridcatalog()
{
    pj_spin_lock();
    if( pj_cs_gridcatalog == NULL )
    {
        pj_cs_gridcatalog = (CRITICAL_SECTION *)malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSectionAndSpinCount(pj_cs_gridcatalog, 4000);
    }
    pj_spin_unlock();

    EnterCriticalSection(pj_cs_gridcatalog);
}

/************************************************************************/
/*                    pj_release_lock_gridcatalog()                     */
/*                                                                      */
/*      Internal function.                                              */
/*      Release the lock used in pj_gridcatalog.c                        */
/************************************************************************/

void pj_release_lock_gridcatalog()
{
    LeaveCriticalSection(pj_cs_gridcatalog);
}

/************************************************************************/
/*                   pj_cleanup_lock_gridcatalog()                      */
/************************************************************************/

static void pj_cleanup_lock_gridcatalog()
{
    if( pj_cs_gridcatalog != NULL )
    {
        DeleteCriticalSection( pj_cs_gridcatalog );
        free( pj_cs_gridcatalog );
        pj_cs_gridcatalog = NULL;
    }
}

#endif // def MUTEX_win32
