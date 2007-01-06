/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

/** @file
 * Declarations for internal utility functions
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "../kazlib/list.h"
#include "../kazlib/hash.h"
#include "cpluff.h"
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/

// For operating on smallish pointer sets implemented as lists 

/**
 * Compares pointers.
 * 
 * @param ptr1 the first pointer
 * @param ptr2 the second pointer
 * @return zero if the pointers are equal, otherwise non-zero
 */
CP_HIDDEN int cpi_comp_ptr(const void *ptr1, const void *ptr2) CP_CONST;

/**
 * Returns a hash value for a pointer.
 * 
 * @param ptr the pointer being hashed
 * @return the corresponding hash value
 */
CP_HIDDEN hash_val_t cpi_hashfunc_ptr(const void *ptr) CP_CONST;

/**
 * Adds a new pointer to a list if the pointer is not yet included.
 * 
 * @param set the set being operated on
 * @param ptr the pointer being added
 * @return non-zero if the operation was successful, zero if allocation failed
 */
CP_HIDDEN int cpi_ptrset_add(list_t *set, void *ptr);

/**
 * Removes a pointer from a pointer set, if it is included.
 * 
 * @param set the set being operated on
 * @param ptr the pointer being removed
 * @return whether the pointer was contained in the set
 */
CP_HIDDEN int cpi_ptrset_remove(list_t *set, const void *ptr);

/**
 * Returns whether a pointer is included in a pointer set.
 * 
 * @param set the set being operated on
 * @param ptr the pointer
 * @return non-zero if the pointer is included, zero otherwise
 */
CP_HIDDEN int cpi_ptrset_contains(list_t *set, const void *ptr) CP_PURE;


// Other list processing utility functions 

/**
 * Processes a node of the list by freeing the associated pointer and
 * deleting the node.
 * 
 * @param list the list being processed
 * @param node the list node being processed
 * @param dummy a dummy argument to comply with prototype
 */
CP_HIDDEN void cpi_process_free_ptr(list_t *list, lnode_t *node, void *dummy);


// For version strings 

/**
 * Checks the version string for validity.
 * 
 * @param v the version string to be checked
 * @return non-zero if the version string is valid, zero otherwise
 */
CP_HIDDEN int cpi_version_isvalid(const char *v) CP_PURE;

/**
 * Compares two version strings. If either of the version strings is invalid
 * the result is undetermined.
 * 
 * @param v1 the first version string
 * @param v2 the second version string
 * @param nc the number of version string components to compare (1-4)
 * @return negative if v1 < v2, zero if v1 = v2 or positive if v1 > v2
 */
CP_HIDDEN int cpi_version_cmp(const char *v1, const char *v2, int nc) CP_PURE;


// Miscellaneous utility functions 

/**
 * Makes a duplicate of the specified string. The required memory is allocated
 * using malloc.
 * 
 * @param str the string to be duplicated
 */
#ifdef HAVE_DMALLOC_H
CP_HIDDEN char * cpi_strdup_dm(const char *src, const char *file, int line);
#else
CP_HIDDEN char * cpi_strdup(const char *src);
#endif

#ifdef HAVE_DMALLOC_H
#define cpi_strdup(a) cpi_strdup_dm((a), __FILE__, __LINE__)
#endif


#ifdef __cplusplus
}
#endif //__cplusplus 

#endif //UTIL_H_
