/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)queue.h	8.3 (Berkeley) 12/13/93
 *
 * For Jos, extra comments have been added to this file, and the original
 * TAILQ and CIRCLEQ definitions have been removed.   - August 9, 2005
 */

#ifndef LWIP_ARCH_QUEUE_H
#define LWIP_ARCH_QUEUE_H

/*
 * A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked
 * so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before
 * or after an existing element or at the head of the list. A list
 * may only be traversed in the forward direction.
 */

/*
 * An example using the below functions.
 */
#if 0

struct Frob
{
	int frobozz;
	LIST_ENTRY(Frob) frob_link;	/* this contains the list element pointers */
};

LIST_HEAD(Frob_list, Frob)		/* defines struct Frob_list as a list of Frob */

struct Frob_list flist;			/* declare a Frob list */

LIST_INIT(&flist);			/* clear flist (globals are cleared anyway) */
flist = LIST_HEAD_INITIALIZER(&flist);	/* alternate way to clear flist */

if(LIST_EMPTY(&flist))			/* check whether list is empty */
	printf("list is empty\n");

struct Frob *f = LIST_FIRST(&flist);	/* f is first element in list */
f = LIST_NEXT(f, frob_link);		/* now f is next (second) element in list */
f = LIST_NEXT(f, frob_link);		/* now f is next (third) element in list */

for(f=LIST_FIRST(&flist); f != 0; 	/* iterate over elements in flist */
    f = LIST_NEXT(f, frob_link))
	printf("f %d\n", f->frobozz);

LIST_FOREACH(f, &flist, frob_link)	/* alternate way to say that */
	printf("f %d\n", f->frobozz);

f = LIST_NEXT(LIST_FIRST(&flist));	/* f is second element in list */
LIST_INSERT_AFTER(f, g, frob_link);	/* add g right after f in list */
LIST_REMOVE(g, frob_link);		/* remove g from list (can't insert twice!) */
LIST_INSERT_BEFORE(f, g, frob_link);	/* add g right before f */
LIST_REMOVE(g, frob_link);		/* remove g again */
LIST_INSERT_HEAD(&flist, g, frob_link);	/* add g as first element in list */

#endif

/*
 * List declarations.
 */

/*
 * A list is headed by a structure defined by the LIST_HEAD macro.  This structure con‐
 * tains a single pointer to the first element on the list.  The elements are doubly
 * linked so that an arbitrary element can be removed without traversing the list.  New
 * elements can be added to the list after an existing element or at the head of the list.
 * A LIST_HEAD structure is declared as follows:
 *
 *       LIST_HEAD(HEADNAME, TYPE) head;
 *
 * where HEADNAME is the name of the structure to be defined, and TYPE is the type of the
 * elements to be linked into the list.  A pointer to the head of the list can later be
 * declared as:
 *
 *       struct HEADNAME *headp;
 *
 * (The names head and headp are user selectable.)
 */
#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

/*
 * Set a list head variable to LIST_HEAD_INITIALIZER(head)
 * to reset it to the empty list.
 */
#define	LIST_HEAD_INITIALIZER(head)					\
	{ NULL }

/*
 * Use this inside a structure "LIST_ENTRY(type) field" to use
 * x as the list piece.
 *
 * The le_prev points at the pointer to the structure containing
 * this very LIST_ENTRY, so that if we want to remove this list entry,
 * we can do *le_prev = le_next to update the structure pointing at us.
 */
#define	LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* ptr to ptr to this element */	\
}

/*
 * List functions.
 */

/*
 * Is the list named "head" empty?
 */
#define	LIST_EMPTY(head)	((head)->lh_first == NULL)

/*
 * Return the first element in the list named "head".
 */
#define	LIST_FIRST(head)	((head)->lh_first)

/*
 * Return the element after "elm" in the list.
 * The "field" name is the link element as above.
 */
#define	LIST_NEXT(elm, field)	((elm)->field.le_next)

/*
 * Iterate over the elements in the list named "head".
 * During the loop, assign the list elements to the variable "var"
 * and use the LIST_ENTRY structure member "field" as the link field.
 */
#define	LIST_FOREACH(var, head, field)					\
	for ((var) = LIST_FIRST((head));				\
	    (var);							\
	    (var) = LIST_NEXT((var), field))

/*
 * Reset the list named "head" to the empty list.
 */
#define	LIST_INIT(head) do {						\
	LIST_FIRST((head)) = NULL;					\
} while (0)

/*
 * Insert the element "elm" *after* the element "listelm" which is
 * already in the list.  The "field" name is the link element
 * as above.
 */
#define	LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if ((LIST_NEXT((elm), field) = LIST_NEXT((listelm), field)) != NULL)\
		LIST_NEXT((listelm), field)->field.le_prev =		\
		    &LIST_NEXT((elm), field);				\
	LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &LIST_NEXT((listelm), field);		\
} while (0)

/*
 * Insert the element "elm" *before* the element "listelm" which is
 * already in the list.  The "field" name is the link element
 * as above.
 */
#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	LIST_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &LIST_NEXT((elm), field);		\
} while (0)

/*
 * Insert the element "elm" at the head of the list named "head".
 * The "field" name is the link element as above.
 */
#define	LIST_INSERT_HEAD(head, elm, field) do {				\
	if ((LIST_NEXT((elm), field) = LIST_FIRST((head))) != NULL)	\
		LIST_FIRST((head))->field.le_prev = &LIST_NEXT((elm), field);\
	LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &LIST_FIRST((head));			\
} while (0)

/*
 * Remove the element "elm" from the list.
 * The "field" name is the link element as above.
 */
#define	LIST_REMOVE(elm, field) do {					\
	if (LIST_NEXT((elm), field) != NULL)				\
		LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = LIST_NEXT((elm), field);		\
} while (0)

#endif	/* !_SYS_QUEUE_H_ */
