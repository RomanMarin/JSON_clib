/*
MIT License Copyright (c) 2018 Roman Marin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __JSON_CLIB_H
#define __JSON_CLIB_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "clib_aux.h"

#define JSON_ON_DEBUG
/* if defined error messages will be printed to stderr */

#define JSON_LIMIT_CHECK
/*  if defined the parser will check the limits (see
below) which may be more secure but slightly slower */

//#define JSON_NO_MEMALLOC
/*  no dynamic memory allocation - json_ctx structure including
json_node pool with JSON_MAX_NODES will be allocated once in calling json_init() */

/* setting limits to protect against malicious data */
#ifdef JSON_LIMIT_CHECK
#define JSON_MAX_STRING_SIZE    512
#define JSON_MAX_DEPTH          10
#endif // JSON_LIMIT_CHECK

/* If JSON_NO_MEMALLOC is defined setting lowest possible value for JSON_MAX_NODES
*   will require less memory for the json nodes pool. Otherwise the value's main purpose
*   is to safeguard against malicious inputs */
#define JSON_MAX_NODES          1000000

typedef enum json_error{
    ERR_JSON_OK,             /* successfully parsed */
    ERR_JSON_INCOMPLETE,     /* must get more data */
    ERR_JSON_UNEXPECTED,     /* unexpected character encountered */
    ERR_JSON_NUMBER,         /* invalid number value */
    ERR_JSON_STRING,         /* maximum allowed value for string size exceeded */
    ERR_JSON_DEPTH,          /* maximum allowed value for depth exceeded */
    ERR_JSON_NODES,          /* maximum allowed value for # nodes exceeded */
    ERR_JSON_COMMENT,        /* comment line error */
    ERR_JSON_MEMALLOC,       /* memory allocation error */
    ERR_JSON_NULLPTR,        /* function received null pointer */
    ERR_JSON_OVERFLOW,       /* not enough space in the buffer */
    ERR_JSON_NOTACONTAINER,  /* not a container type (object or array) of a node */
    ERR_JSON_TYPE,           /* not expected type of the json value */
    ERR_JSON_NOSTRING        /* String is missing in non empty JSON object type */
} json_error;

typedef enum json_type{
    JSON_DUMMY,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_DOUBLE,
    JSON_INTEGER,
    JSON_BOOL
} json_type;

/* we access all JSON values through this union */
typedef union json_value{
    char*           string_value;   /* pointer to the string value */
    long long       integer_value;
    double          double_value;
    int             bool_value;     /* 0 - false, ~0 - true */
} json_value;

typedef struct json_node{
    json_type       type;
    json_value      val;
    const char*     key;            /* pointer to the key value */
    struct json_node*    parent;    /* points to the parent node (null for root node) */
    struct json_node*    next;      /* points at the next sibling node (siblings have same parent) of the tree */
    struct json_node*    first_child;    /* points at the first child node if any - for object or array types */
} json_node;

/* JSON context structure */
typedef struct json_ctx{
    json_node*      root;       /* pointer to the root node */
#ifdef JSON_NO_MEMALLOC
    json_node       pool[JSON_MAX_NODES];   /* if no dynamic allocations -
                                            the root pointer will be equal to &pool[0]  */
#endif // JSON_NO_MEMALLOC
    int             pos;        /* # bytes parsed */
    int             nused;      /* number of nodes used\allocated */
    int             ndepth;     /* indentation depth counter */
    int             decode;     /* if not 0 - strings are decoded to utf-8 */
    json_error      err;        /* error code */
} json_ctx;

/** Initialize a new JSON context structure
*    Return: pointer to the json_ctx struct
*    Remark: the function allocates and zeroes  json_ctx struct.
*       If JSON_NO_MEMALLOC defined it also makes one-time allocation for
*       a pool of JSON_MAX_NODES. Otherwise calloc() is called for every
*       node when it is created.
*       A call to json_remove_node() releases the memory for a particular node and
*       for all its descendants.
*       A call to json_destroy(ctx) releases everything.
*/
json_ctx* json_init(void);

/**
*   Parse existing JSON string
*   Input:
*       parser - pointer to preallocated ison_ctx structure if JSON_NO_MEMALLOC defined.
*               the value is ignored otherwise
*       buflen - maximum # bytes to parse
*       to_utf8 - if == 0 (no decoding takes place) if != 0 - all u-escapes in strings are decoded
*   Return: pointer to root json_node structure
*       NULL - an error occurred (ctx->err is set - see codes above)
*       In any case json_ctx struct will have its values set
*       ctx->pos - will be equal to # bytes parsed - 1;
*           On error it will keep the position of a byte where parser stopped
*       ctx->nused - # nodes created\used so far
*   Remarks:
*       The initial buffer content will be modified as null terminators are placed at the end
*       of the key and string values as well as escapes are decoded in place.
*       Parsing stops when one of the following conditions is met:
*           - complete JSON object is parsed
*           - end of buffer reached
*           - error encountered
*           on return ctx->err is set to json_error value
*       The parser accepts only UTF-8 encoded strings.
*       Don't try to parse string literals! They're read-only and will cause segmentation fault.
*/
json_node* json_parse(json_ctx* ctx, char* buf, int buflen, int to_utf8);

/** Get node by a given key
*   Input:
*       parent must be a valid container object - json array or object
*       key - may be a string literal
*   Return: NULL if a node was not found or an error occurred
*/
json_node* json_get_node(json_node* parent, const char* key);

/** Get an array's or object's element by a given index
*   Return: valid json_node pointer or NULL on error (e.g. parent is not a container
*       type node - object or array)
*/
json_node* json_get_element(json_node* parent, int index);

/** Get # elements in array or in object
*   Return: # elements or -1 on error (if the node is not a container type)
*/
int json_get_nelements(json_node* parent);

/** Add json node
*    Input:  ctx - pointer to the json_ctx structure
*       parent - pointer to parent node (must be object or array type)
*       tp  - type of new node
*       key - pointer to preallocated string or NULL
*    Return: pointer to the new json node or NULL on error
*    Remark: the value of newly created node has to be set explicitly (e.g. node->val = 0.1
*       if node->type == JSON_DOUBLE)
*/
/* Insert as first element of an array or an object */
json_node* json_add_first(json_ctx* ctx, json_node *parent, json_type tp, const char* key);
/* Insert as last element of an array or an object */
json_node* json_add_last(json_ctx* ctx, json_node *parent, json_type tp, const char* key);
/* Insert after the given node */
json_node* json_add_after(json_ctx* ctx, json_node *nd, json_type tp, const char* key);
/* Insert before the given node */
json_node* json_add_before(json_ctx* ctx, json_node *nd, json_type tp, const char* key);

/** Delete individual json node, rearranging the tree.
*   Clear all its children if any and release the memory
*/
void json_remove_node(json_ctx* ctx, json_node *nd);

/** Delete json_ctx structure all its content
*   Remark: the function deletes ctx->root node first if it still exists
*   and then json_ctx structure is released.
*   Alternatively ctx->root may be released by calling json_node_remove(stx, root);
*/
void json_destroy(json_ctx* ctx);

/**
*   Serialize json_node object into preallocated buffer
*   Input:
*       nd - json_node to be serialized (must be array or object type)
*       out - byte array
*       outlen - maximum # bytes in the array
*       compact - if == 0 output string will be formatted (cr, lf, tab, space are inserted),
*               if compact != 0 - not formatted
*   Return: # bytes written (not including null terminator) or -1 on error (e.g. buffer is too small)
*   Remark: null terminator is placed at the end of output string even if
*       the object is serialized partially
*       if nd == ctx->root the whole object will be serialized
*/
int json_to_string(json_node* nd, char* out, int outlen, int compact);

/* just a forward declaration - for use in tests  - see source file for full description */
int json_atonum(char* buf, int* len, void* jnum);

#ifdef  __cplusplus
}
#endif

#endif

