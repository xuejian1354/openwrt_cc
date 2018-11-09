/************************************************************************************************/
/*																								*/
/* Copyright (c) 2015-2016	Shanghai Upointech Information Techology Co. All Rights Reserved.	*/
/*																								*/
/*				上海有点信息科技有限公司	   版权所有 2015-2016								*/
/*											   													*/
/* This program is the proprietary software of Upointech Co., and may only be used,				*/
/* duplicated, modified or distributed pursuant to the terms and conditions of a separate,		*/
/* written license agreement executed between you and Upointech(an "Authorized License").		*/
/* The recipient of this software implicitly accepts the terms of the license.					*/
/*																								*/
/* 本程序的版权属于上海有点信息科技有限公司，任何人士阅读、使用、复制、修改或者发行都必			*/
/* 须获得相应的书面授权,承担保密责任和接受相应的法律约束.										*/
/*																					   			*/
/************************************************************************************************/
#ifndef _CAPI_VARIABLE_TYPE_H_
#define _CAPI_VARIABLE_TYPE_H_

#include <gio/gio.h>
#include <stdarg.h>

typedef GVariant CtSgwVariant_t;
typedef GVariant CtSgwVariant;
typedef unsigned int uint32_t;
typedef gboolean boolean;

/*
 * Function Name: GVar2CtsgwVar
 * 
 * Description:    Cast the Gvariant * type to CtSgwVariant_t * type
 *         
 * Parameter:
 *    v <IN>:    A pointer to GVariant type parameter
 *
 * Return: A pointer to CtSgwVariant_t variable come from v
 *
 */
CtSgwVariant_t* GVar2CtSgwVar (GVariant* v);


/*
 * Function Name: CtsgwVar2GVar
 *
 * Description: Cast the CtSgwVariant_t * type to GVariant * type
 *
 * Parameter:
 *    inst <IN>:    A pointer to CtSgwVariant_t type parameter
 *
 * Return: A pointer to GVariant variable come from v
 *
 */
GVariant* CtSgwVar2GVar (CtSgwVariant_t* v);


/*
 * Function Name: CtSgwVariantRef
 *
 * Description: Increases the reference count of v
 *
 * Parameter:
 *    v <IN>:    A pointer to CtSgwVariant_t type parameter
 *
 * Return: The same v
 *
 */
CtSgwVariant_t* CtSgwVariantRef (CtSgwVariant_t *v);


/*
 * Function Name: CtSgwVariantUnRef
 *
 * Description: Decreases the reference count of v. When its reference count 
 *              drops to 0, the memory used by the variant is freed
 *
 * Parameter:
 *    v <IN>:  A pointer to CtSgwVariant_t type parameter
 *
 * Return:
 *
 */
void CtSgwVariantUnRef (CtSgwVariant_t *v);


/*
 * Function Name: CtSgwVariantNew
 *
 * Description: Creates a new CtSgwVariant_t instance
 *
 * Parameter:
 *   format_string <IN>:    A GVariant format string
 *
 * Return:    A new floating CtSgwVariant_t instance
 *
 */
CtSgwVariant_t *CtSgwVariantNew (const char *format, ...);


/*
 * Function Name: CtSgwVariantNewArray
 *
 * Description: Creates a new CtSgwVariant_t arry from children
 *
 * Parameter:
 *      children <IN>:    the element type of the new array.
 *    n_children <IN>:    the length of children
 *
 * Return:    A floating reference to a new CtSgwVariant_t array
 *
 */
CtSgwVariant_t* CtSgwVariantNewArray (CtSgwVariant_t* const *children, uint32_t n_children);


/*
 * Function Name: CtSgwVariantNewTuple
 *
 * Description: Creates a new tuple CtSgwVariant_t out of items in children.
 *              The type is determined from the types of children, No entry
 *              in the children array may be NULL
 *
 * Parameter:
 *      children <IN>:   the items to make the tuple out of.
 *    n_children <IN>:   the length of children
 *
 * Return:    A floating reference to a new CtSgwVariant_t tuple
 *
 */
CtSgwVariant_t* CtSgwVariantNewTuple (CtSgwVariant_t * const * children, uint32_t n_children);


/*
 * Function Name: CtSgwVariantNewDictEntry
 *
 * Description: Creates a new dictionary entry CtSgwVariant_t.
 *              key and value must be non-NULL. key must be a
 *              value of basic type (ie: not a container)
 *
 * Parameter:
 *      key <IN>:   a basic CtSgwVariant, the key.
 *    value <IN>:   a CtSgwVariant_t, the value
 *
 * Return:    A floating reference to a new dictionary entry CtSgwVariant_t
 *
 */
CtSgwVariant_t *CtSgwVariantNewDictEntry (CtSgwVariant_t *key, CtSgwVariant_t *value);


/*
 * Function Name: CtSgwVariantGet
 *
 * Description: Deconstructs a CtSgwVariant_t instance.
 *
 * Parameter:
 *         v <IN>:    A CtSgwVariant_t instance
 *    format <IN>:    A CtSgwVariant_t format string
 *
 * Return:
 *
 */
void CtSgwVariantGet (CtSgwVariant_t *v, const char *format, ...);


/*
 * Function Name: CtSgwVariantGetVa
 *
 * Description: This function is intended to be used by libraries based on CtSgwVariant_t that
 *              want to provide CtSgwVariantGet()-like functionality to their user.
 *
 * Parameter:
 *         v <IN>:  A CtSgwVariant_t
 *    format <IN>:  A string that is prefixed with a format string
 *    endptr <IN>:  Location to store the end pointer, or NULL
 *       app <IN>:  A pointer to a va_list.
 *
 * Return:
 *
 */
void CtSgwVariantGetVa (CtSgwVariant *v, const char *format, const char **endptr, va_list *app);


/*
 * Function Name: CtSgwVariantChildNum
 *
 * Description: Determines the number of children in a container CtSgwVariant_t instance.
 *              This includes variants, maybes, arrays, tuples and dictionary entries.
 *
 * Parameter:
 *    v <IN>:    A container CtSgwVariant_t
 *
 * Return:    The number of children in the container
 *
 */
uint32_t CtSgwVariantChildNum ( CtSgwVariant_t *v);


/*
 * Function Name: CtSgwVariantGetChildVal
 *
 * Description: Determines the number of children in a container CtSgwVariant_t instance.
 *              This includes variants, maybes, arrays, tuples and dictionary entries.
 *
 * Parameter:
 *      v <IN>:    A container CtSgwVariant_t
 *
 * Return:    The number of children in the container
 *
 */
CtSgwVariant_t *CtSgwVariantGetChildVal (CtSgwVariant_t *v, uint32_t index);


/*
 * Function Name: CtSgwVariantGetChild
 *
 * Description: Reads a child item out of a container CtSgwVariant_t
 *              instance and deconstructs it according to format
 *
 * Parameter:
 *        v <IN>:   A container CtSgwVariant_t
 *    index <IN>:   The index of the child to deconstruct
 *   format <IN>:   A CtSgwVariant_t format string
 *
 * Return:    
 *
 */
void CtSgwVariantGetChild ( CtSgwVariant_t *v, uint32_t index, const char *format, ...);


/*
 * Function Name: CtSgwVariantLookupValue
 *
 * Description: Looks up a value in a dictionary CtSgwVariant_t
 *
 * Parameter:
 *       dict <IN>:   A dictionary CtSgwVariant_t
 *        key <IN>:   The key to lookup in the dictionary
 *    typestr <IN>:   a GVariantType, or NULL
 *
 * Return:    the value of the dictionary key, or NULL
 *
 */
CtSgwVariant_t *CtSgwVariantLookupValue( CtSgwVariant_t *dict, const char *key, const char *typestr);


/*
 * Function Name: CtSgwVariantLookup
 *
 * Description: Looks up a value in a dictionary CtSgwVariant_t
 *
 * Parameter:
 *    dict <IN>:   A dictionary CtSgwVariant_t
 *     key <IN>:   The key to lookup in the dictionary
 *  format <IN>:   a CtSgwVariant_t format string
 *
 * Return:    TRUE if a value was unpacked
 *
 */
boolean CtSgwVariantLookup (CtSgwVariant_t *dict, const char *key, const char *format, ...);


/*
 * Function Name: CtSgwVariantPrint
 *
 * Description: Pretty-prints v in the format understood by g_variant_parse
 *
 * Parameter:
 *    v <IN>:    A CtSgwVariant_t
 *
 * Return:    a newly-allocated string holding the result.
 *
 */
char *CtSgwVariantPrint (CtSgwVariant_t *v);


#endif
