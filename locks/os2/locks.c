/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apr_general.h"
#include "apr_lib.h"
#include "locks.h"
#include "fileio.h"
#include <string.h>
#define INCL_DOS
#include <os2.h>


ap_status_t lock_cleanup(void *thelock)
{
    struct lock_t *lock = thelock;
    return ap_destroy_lock(lock);
}



ap_status_t ap_create_lock(ap_context_t *cont, ap_locktype_e type, char *fname, struct lock_t **lock)
{
    struct lock_t *new;
    ULONG rc;
    char *semname;

    new = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));
    new->cntxt = cont;
    new->type = type;
    new->curr_locked = 0;
    new->fname = ap_pstrdup(cont, fname);
    semname = ap_pstrcat(cont, "/SEM32/", fname, NULL);
    rc = DosCreateMutexSem(semname, &(new->hMutex), type == APR_CROSS_PROCESS ? DC_SEM_SHARED : 0, FALSE);
    *lock = new;
    return os2errno(rc);
}



ap_status_t ap_lock(struct lock_t *lock)
{
    ULONG rc;
    
    if (!lock->curr_locked) {
        rc = DosRequestMutexSem(lock->hMutex, SEM_INDEFINITE_WAIT);

        if (rc == 0)
            lock->curr_locked = TRUE;

        return os2errno(rc);
    }
    
    return APR_SUCCESS;
}



ap_status_t ap_unlock(struct lock_t *lock)
{
    ULONG rc;
    
    if (lock->curr_locked) {
        rc = DosReleaseMutexSem(lock->hMutex);

        if (rc == 0)
            lock->curr_locked = FALSE;

        return os2errno(rc);
    }
    
    return APR_SUCCESS;
}



ap_status_t ap_destroy_lock(struct lock_t *lock)
{
    ULONG rc;
    ap_status_t stat;

    stat = ap_unlock(lock);
    
    if (stat != APR_SUCCESS)
        return stat;
        
    if (lock->hMutex == 0)
        return APR_SUCCESS;

    rc = DosCloseMutexSem(lock->hMutex);
    
    if (!rc)
        lock->hMutex = 0;
        
    return os2errno(rc);
}
