/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005,2007 Red Hat, Inc.
 * Copyright © 2007 Mathias Hasselmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Mathias Hasselmann <mathias.hasselmann@gmx.de>
 *	Behdad Esfahbod <behdad@behdad.org>
 */

#ifndef CAIRO_MUTEX_PRIVATE_H
#define CAIRO_MUTEX_PRIVATE_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <cairo-features.h>

CAIRO_BEGIN_DECLS


/* A fully qualified no-operation statement */
#define CAIRO_MUTEX_NOOP	do {/*no-op*/} while (0)



#if CAIRO_NO_MUTEX

typedef int cairo_mutex_t;
# define CAIRO_MUTEX_INITIALIZE()	CAIRO_MUTEX_NOOP
# define CAIRO_MUTEX_LOCK(name)		CAIRO_MUTEX_NOOP
# define CAIRO_MUTEX_UNLOCK(name)	CAIRO_MUTEX_NOOP
# define CAIRO_MUTEX_NIL_INITIALIZER	0

#elif HAVE_PTHREAD_H /*******************************************************/

# include <pthread.h>

  typedef pthread_mutex_t cairo_mutex_t;

# define CAIRO_MUTEX_INITIALIZE() CAIRO_MUTEX_NOOP
# define CAIRO_MUTEX_LOCK(name) pthread_mutex_lock (&name)
# define CAIRO_MUTEX_UNLOCK(name) pthread_mutex_unlock (&name)
# define CAIRO_MUTEX_FINI(mutex) pthread_mutex_destroy (mutex)
# define CAIRO_MUTEX_NIL_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#elif HAVE_WINDOWS_H /*******************************************************/

/* We require Windows 2000 features. Although we don't use them here, things
 * should still work if this header file ends up being the one to include
 * windows.h into a source file, so: */
# if !defined(WINVER) || (WINVER < 0x0500)
#  define WINVER 0x0500
# endif

# if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
#  define _WIN32_WINNT 0x0500
# endif

# define WIN32_LEAN_AND_MEAN
# include <windows.h>

  typedef CRITICAL_SECTION cairo_mutex_t;

# define CAIRO_MUTEX_LOCK(name) EnterCriticalSection (&name)
# define CAIRO_MUTEX_UNLOCK(name) LeaveCriticalSection (&name)
# define CAIRO_MUTEX_INIT(mutex) InitializeCriticalSection (mutex)
# define CAIRO_MUTEX_FINI(mutex) DeleteCriticalSection (mutex)
# define CAIRO_MUTEX_NIL_INITIALIZER { NULL, 0, 0, NULL, NULL, 0 }

#elif defined __OS2__ /******************************************************/

# define INCL_BASE
# define INCL_PM
# include <os2.h>

  typedef HMTX cairo_mutex_t;

# define CAIRO_MUTEX_LOCK(name) DosRequestMutexSem(name, SEM_INDEFINITE_WAIT)
# define CAIRO_MUTEX_UNLOCK(name) DosReleaseMutexSem(name)
# define CAIRO_MUTEX_INIT(mutex) DosCreateMutexSem (NULL, mutex, 0L, FALSE)
# define CAIRO_MUTEX_FINI(mutex) do {				\
    if (0 != (mutex)) {						\
        DosCloseMutexSem (*(mutex));				\
        (mutex) = 0;						\
    }								\
} while (0)
# define CAIRO_MUTEX_NIL_INITIALIZER 0

#elif CAIRO_HAS_BEOS_SURFACE /***********************************************/

  typedef void* cairo_mutex_t;

  cairo_private void _cairo_beos_lock(cairo_mutex_t*);
  cairo_private void _cairo_beos_unlock(cairo_mutex_t*);

/* the real initialization takes place in a global constructor */
# define CAIRO_MUTEX_LOCK(name) _cairo_beos_lock (&name)
# define CAIRO_MUTEX_UNLOCK(name) _cairo_beos_unlock (&name)

# warning "XXX: Someone who understands BeOS needs to add definitions for" \
          "     cairo_mutex_t, CAIRO_MUTEX_INIT, and CAIRO_MUTEX_FINI," \
          "     and CAIRO_MUTEX_NIL_INITIALIZER to cairo-mutex-private.h"

#else /**********************************************************************/

# error "XXX: No mutex implementation found.  Define CAIRO_NO_MUTEX to 1" \
        "     to compile cairo without thread-safety support."

#endif



#ifndef CAIRO_MUTEX_DECLARE
#define CAIRO_MUTEX_DECLARE(name) extern cairo_mutex_t name;
#endif
#include "cairo-mutex-list-private.h"
#undef CAIRO_MUTEX_DECLARE


#ifndef CAIRO_MUTEX_INIT
# define CAIRO_MUTEX_INIT(_mutex) do {				\
    cairo_mutex_t _tmp_mutex = CAIRO_MUTEX_NIL_INITIALIZER;     \
    memcpy ((_mutex), &_tmp_mutex, sizeof (_tmp_mutex));        \
} while (0)
#endif

#ifndef CAIRO_MUTEX_FINI
# define CAIRO_MUTEX_FINI(mutex)	CAIRO_MUTEX_NOOP
#endif


#ifndef CAIRO_MUTEX_INITIALIZE
# define CAIRO_MUTEX_USE_GENERIC_INITIALIZATION 1
#else
# undef CAIRO_MUTEX_USE_GENERIC_INITIALIZATION
# ifndef CAIRO_MUTEX_FINALIZE
#  define CAIRO_MUTEX_FINALIZE CAIRO_MUTEX_NOOP
# endif
#endif

#if CAIRO_MUTEX_USE_GENERIC_INITIALIZATION

#define CAIRO_MUTEX_INITIALIZE() do { \
    if (!_cairo_mutex_initialized) \
        _cairo_mutex_initialize (); \
} while(0)

#define CAIRO_MUTEX_FINALIZE() do { \
    if (_cairo_mutex_initialized) \
        _cairo_mutex_finalize (); \
} while(0)

cairo_private extern cairo_bool_t _cairo_mutex_initialized;
cairo_private void _cairo_mutex_initialize(void);
cairo_private void _cairo_mutex_finalize(void);

#endif

CAIRO_END_DECLS

/* Make sure everything we want is defined */
#ifndef CAIRO_MUTEX_INITIALIZE
# error "CAIRO_MUTEX_INITIALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_FINALIZE
# error "CAIRO_MUTEX_FINALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_LOCK
# error "CAIRO_MUTEX_LOCK not defined"
#endif
#ifndef CAIRO_MUTEX_UNLOCK
# error "CAIRO_MUTEX_UNLOCK not defined"
#endif
#ifndef CAIRO_MUTEX_INIT
# error "CAIRO_MUTEX_INIT not defined"
#endif
#ifndef CAIRO_MUTEX_FINI
# error "CAIRO_MUTEX_FINI not defined"
#endif
#ifndef CAIRO_MUTEX_NIL_INITIALIZER
# error "CAIRO_MUTEX_NIL_INITIALIZER not defined"
#endif

#endif