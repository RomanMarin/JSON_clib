# JSON_CLIB

A super-fast, extra-compact, JSON library written in pure C 
No dependencies 
RFC 4627 compliant 
Checks against malicious inputs

# Features
The library contains functions to create a new JSON object from scratch, parse memory based JSON data (ascii or UTF-8 encoded), access individual keys and values, add, delete, modify and serialize tree-based JSON data to strings. 
It supports dual-mode operations: with dynamic memory allocations or using preallocated tree structure and pool of nodes, which may require more memory (constant volume) but is slightly faster. To switch between the modes a constant definition `#define JSON_NO_MEMALLOC` is used in `json_clib.h` header file. 
Note: the internal JSON tree stores only pointers (nor copies) for all 'string' values of parsed or created JSON objects. Therefore while JSON object tree and nodes are in use, a client application must not change those memory locations uncontrollably during a 'lifetime' of JSON tree. 

# Files
Header `json_clib.h` and source `json_clib.c` contain API functions while `clib_aux.h` and `clib_aux.c` contain thoroughly optimized helper functions used as replacent for C standard library functions by the target library.  `json_test.c` and `json_test1.c` simulate different test scenarios. 

# Build
Add all sources and headers ('src', 'include' and 'test' folders  and its content) in your favorite IDE, build and run or just run against included Makefile: `$ make` on Linux or `mingw32-make` on Windows, which will create LIB folder with `libcjson.a` static library. Including the header `#include "json_clib.h"`and linking against `libcjson.a` will provide all required API for an application. To get a faster executable -O2 or -O3 compiler switch must be used. 
Running against 'test' target `$ make test` or `mingw32-make test` will automatically create the static library and run a test file, which source is located in `./test` folder, demonstrating major library functionality in Console output. See the source `./test/json_test.c` and `./test/json_test1.c` for reference. Running   `$ make clean` or `mingw32-make clean`  deletes crated obj files and executable

# Interface
## Typedefs:
 ```
 typedef enum json_type{
    JSON_DUMMY,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_DOUBLE,
    JSON_INTEGER,
    JSON_BOOL
} json_type; /* used to reference the types of JSON values */ 

typedef union json_value{
    char*           string_value;   /* pointer to string value */
    long long       integer_value;  /* actual long long value for JSON number */
    double          double_value; 	/* actual double value for JSON number */
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

/* JSON context base structure */
 typedef struct json_ctx{
    json_node*      root;       /* pointer to the root node */
	int             pos;        /* # bytes parsed */
    int             nused;      /* number of nodes used\allocated */
	int             ndepth;     /* indentation depth counter */
	int             decode;     /* if not 0 - strings with escapes are decoded to utf-8 */
    json_error      err;        /* error code -  see json_clib.h source for error codes */
} json_ctx;
```

## Functions     
```
json_ctx* json_init(void);
```
Initialize a new JSON context structure
**Return:** pointer to the json_ctx struct on NULL if allocation error occurred
**Remark:** the function allocates and zeroes  json_ctx struct.
	If JSON_NO_MEMALLOC defined it also makes one-time allocation for
	a pool of JSON_MAX_NODES. Otherwise calloc() is called for every
	node when it is created.
	A call to json_remove_node() releases the memory for a particular node and
	for all its descendants.
	A call to json_destroy(ctx) will release everything 
 
 ```
 json_node* json_parse(json_ctx* ctx, char* buf, int buflen, int to_utf8);
 ```
 Parse existing JSON string
**Input:**
	*ctx* - pointer to preallocated ison_ctx structure
	*buf* - memory location of data to be parsed
	*buflen* - maximum # bytes to parse
	*to_utf8* - if == 0 (no decoding takes place) if != 0 - all u-escapes in strings are decoded
**Return:** pointer to root json_node structure or NULL - an error occurred (ctx->err is set - see codes in source file `json_clib.h`). In any case json_ctx struct will have its values set: 
ctx->pos - will be equal to # bytes parsed - 1; On error it will keep the position of a byte where parser stopped
ctx->nused - # nodes created\used so far
**Remarks:** The initial buffer content will be modified as null terminators are placed at the end of the key and string values as well as escapes are decoded in place. Parsing stops when one of the following conditions is met:
	- complete JSON object is parsed
	- end of buffer reached
	- an error encountered
	On function return ctx->err is set to json_error value
The parser accepts only UTF-8 encoded strings.
Don't try to parse string literals! They're read-only and will cause segmentation fault

 ```
 json_node* json_get_node(json_node* parent, const char* key);
 ```
Get a json_node pointer by a given key
 *parent* must be a valid container object - json array or object
Return: NULL if a node was not found or an error occurred 

```
json_node* json_get_element(json_node* parent, int index);
```
Get an array's or object's element by a given index
 Return: valid json_node pointer or NULL on error (e.g. parent is not a container
          type node - object or array) 

```
int json_get_nelements(json_node* parent);
```
Get # elements in array or in object
Return: # elements or -1 on error (if the node is not a container type) 

```
json_node* json_add_first(json_ctx* ctx, json_node *parent, json_type tp, const char* key);
```
Add JSON node to the tree as first child of given parent
**Input:**  
	       *ctx* - pointer to the json_ctx structure
           *parent* - pointer to parent node (must be object or array type)
           *tp*  - type of new node
           *key* - pointer to preallocated string or NULL. Must be a pointer to valid memory location with value - not a literal constant, as the pointer will be used for future reference
   **Return:** pointer to the new json node or NULL on error
    **Remark:** the value of newly created node has to be set explicitly (e.g. node->val = 0.1 if node->type == JSON_DOUBLE) 

```
json_node* json_add_last(json_ctx* ctx, json_node *parent, json_type tp, const char* key);
``` 
Insert as last element of an array or an object 

```
json_node* json_add_after(json_ctx* ctx, json_node *nd, json_type tp, const char* key);
```
Insert after the given node 

```
json_node* json_add_before(json_ctx* ctx, json_node *nd, json_type tp, const char* key)
```
Insert before the given node 

```
void json_remove_node(json_ctx* ctx, json_node *nd);
```
Delete individual json node, rearranging the tree.
Clear all its children if any and release the memory  

```
void json_destroy(json_ctx* ctx);
```
Delete json_ctx structure and all its content
**Remark:** the function deletes ctx->root node first including all its descendants if it still exists, then json_ctx structure is released.
Alternatively ctx->root may be released by calling json_remove_node(ctx, root); 

```
int json_to_string(json_node* nd, char* out, int outlen, int compact);
```
Serialize json_node object into preallocated buffer
**Input:**  
	*nd* - json_node to be serialized (must be array or object type)
           *out* - byte array
           *outlen* - maximum # bytes in the array
           *compact* - if equals 0, output string will be formatted (cr, lf, tab, space are inserted),
	               if compact != 0 - not formatted
   **Return:** # bytes written (not including null terminator) or -1 on error (e.g. buffer is too small)
   **Remark:** null terminator is placed at the end of output string. if nd == ctx->root the whole JSON tree will be serialized

# Examples
The following demonstrates simple basic usage.
```
#include "json_clib.h"
#include <string.h> // for memcpy() nad memset()
// a string to parse (taken from RFC 4627)
char* sample = "{\
        \"Image\": {\
            \"Width\":  800,\
            \"Height\": 600,\
            \"Title\":  \"View from 15th Floor\",\
            \"Thumbnail\": {\
                \"Url\":    \"http://www.example.com/image/481989943\",\
                \"Height\": 125,\
                \"Width\":  100\
            },\
            \"Animated\" : false,\"IDs\": [116, 943, 234, 38793]\
          }\
      }";
int main()
{
    int length = strlen(sample);
    char in[512];  // input buffer - we can not work with literals
    char out[512]; // we place serialized result here
    json_ctx* ctx = json_init();
    if(!ctx){
        printf("json_init() failed\n");
        exit(-1);
    }
    // copy input data
    memcpy(in, sample, length);
    // parse the buffer
    json_node* root = json_parse(ctx, in, length, 1);
    if(!root){
        printf("json_parse() failed, error code: %d\n", ctx->err);
        exit(-1);
    }
    // serialize the node's tree to string  in compact form
    if(json_to_string(root, out, 512, 1) == -1){
        printf("json_to_string() failed\n");
        exit(-1);
    }
    // show the result
    printf("Source string: %s\n\n", sample);
    printf("Serialized parsed result - compact:\n\n%s\n\n", out);
    // serialize the node tree to string  again in formatted form
    if(json_to_string(root, out, 512, 0) == -1){
        printf("json_to_string() failed\n");
        exit(-1);
    }
    printf("parse-serialize result - formatted:\n\n%s\n\n", out);
    json_destroy(ctx);

    // parse the result again
    ctx = json_init();
    if(!ctx){
        printf("json_init() failed\n");
        exit(-1);
    }
    // clear the buffer
    memset(in, 0, 512);

    root = json_parse(ctx, out, 512, 1);
    if(!root){
        printf("json_parse() failed, error code: %d\n", ctx->err);
        exit(-1);
    }
    // serialize the node tree to string  in formatted form
    if(json_to_string(root, in, 512, 0) == -1){
        printf("json_to_string() failed\n");
        exit(-1);
    }
    printf("parse-serialize-parse-serialize result - formatted:\n\n%s\n\n", in);

    // now let's get and print 'url' key:value pair
    json_node* image = json_get_node(root, "Image");
    if(image){
        json_node* thumbnail = json_get_node(image, "Thumbnail");
        if(thumbnail){
            json_node* url = json_get_node(thumbnail, "Url");
            if(url){
                printf("Found key-value pair:  %s:%s\n\n", url->key, url->val.string_value);
            }
            else printf("Url not found");
        }
        else printf("Thumbnail not found");
    }
    else printf("Image not found");
    json_destroy(ctx);
    return 0;
} 
```

Program Console output:
 ```
 c:\Users\Documents\CPP\json_clib\bin\Release\json_clib
 
 Source string: {        "Image": {            "Width":  800,            "Height": 600,            "Title":  "View from 15th Floor",            "Thumbnail": {                "Url":    "http://www.example.com/image/481989943",                "Height": 125,                "Width":  100            },            "Animated" : false,"IDs": [116, 943, 234, 38793]          }      }
Serialized parsed result - compact:
{"Image":{"Width":800,"Height":600,"Title":"View from 15th Floor","Thumbnail":{"Url":"http:\/\/www.example.com\/image\/481989943","Height":125,"Width":100},"Animated":false,"IDs":[116,943,234,38793]}}

parse-serialize result - formatted:
{
"Image": {
	"Width": 800,
	"Height": 600,
	"Title": "View from 15th Floor",
	"Thumbnail": {
		"Url": "http:\/\/www.example.com\/image\/481989943",
		"Height": 125,
		"Width": 100
		},
	"Animated": false,
	"IDs": [116,943,234,38793]
	}
}
parse-serialize-parse-serialize result - formatted:
{
"Image": {
	"Width": 800,
	"Height": 600,
	"Title": "View from 15th Floor",
	"Thumbnail": {
		"Url": "http:\/\/www.example.com\/image\/481989943",
		"Height": 125,
		"Width": 100
		},
	"Animated": false,
	"IDs": [116,943,234,38793]
	}
}
Found key-value pair:  Url:http://www.example.com/image/481989943
c:\Users\Documents\CPP\json_clib\bin\Release
```

See the sources in `./test` folder for more examples and different test scenarios
# License
MIT 
