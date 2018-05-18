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
#include "json_clib.h"

#ifdef JSON_NO_MEMALLOC
#include <string.h>
#endif // JSON_NO_MEMALLOC

#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

#ifdef JSON_ON_DEBUG
#define JSON_SHOW_ERROR(TXT) \
do{ \
    fprintf(stderr, "%s failed in line %d: %s\n", __func__, __LINE__, TXT); \
    fprintf(stderr, "cursor position %d\n", ctx->pos); \
} while(0)
#else
#define JSON_SHOW_ERROR(TXT)
#endif // JSON_ON_DEBUG

#ifndef LLONG_MIN
#define LLONG_MIN (-(long long)((unsigned long long)~0 >> 1)-1)
#endif // LLONG_MIN

#ifndef LLONG_MAX
#define LLONG_MAX ((long long)((unsigned long long)~0 >> 1))
#endif // LLONG_MAX

#define CUTOFF_POS (LLONG_MAX/10)
#define TRESHOLD_POS ((int)(LLONG_MAX%10))

#define CUTOFF_NEG (LLONG_MIN/10)
#define TRESHOLD_NEG (-(int)(LLONG_MAX%10))

#define IS_DIGIT_GEZ(c) ((unsigned)(c - 0x30) < 10)

/** Largest possible base 10 exponent. Any larger number
*   will result in overflow or underflow so we stop the parsing
*/
#define MAX_EXPONENT 511

/** To to convert decimal into floating point numbers. Usage: 10^2^i */
static double pof_ten[] = {10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256};
static double tens_map[] = {1.,10.,100.,1000.,1.0e4,1.0e5,1.0e6,1.0e7,1.0e8,1.0e9,1.0e10,
1.0e11,1.0e12,1.0e13,1.0e14,1.0e15,1.0e16,1.0e17,1.0e18};


/**
*   Turn valid JSON ascii sequence into a number value: long long, or double
*   (replacement for strtod() and strtoll())
*   Input:  buf - buffer pointer
*           len - maximun # bytes to parse
*           jnum - container for the output (min 8 bytes, e.g. a pointer
*                to preallocated union)
*   Output: len - actual # bytes parsed
*           ri - result (long long or double value)
*   Return: 1 - result is long long
*           2 - result is double
*           -1 - format error
*           -2 - the ascii representation can not be converted to number without
*               precision loss. However if it is a valid JSON number, inum will
*               contain buf pointer and the number may be used as string. Null
*               terminator is not added. Len will contain # actually parsed symbols
*               in JSON number
*   Remark: parsing stops when first invalid character
*           encountered (see json.org for valid number format)
*           Leading 0s are not permitted in the integer section
*           If pointer to the union of long long and double is
*           provided as input, the result will be casted automatically
*           The initial string is unmodified
*/
int json_atonum(char* buf, int* len, void* jnum)
{
    long long* intgr = (long long*)jnum;
    double* dbl = (double*)jnum;
    char** str = jnum;
    double  dexp = 1.0;
    long long rint = 0LL;   /* value read from integer part */
    int rexp = 0;           /* value read from exponent part */
    int i = 0, rlen = *len;
    int neg = 0;            /* sign - negative or positive */
    int neg_exp = 0;        /* exponent sign */
    int nbeforedp;          /* # digits before decimal point */
    int nmantissa;          /* # digits in mantissa */
    char ch;
    if(buf[i] == '-'){
        i++;
        neg = 1;
    }
    ch = buf[i];
    if(IS_DIGIT_GEZ(ch)){
        if(ch == '0'){
            ch = buf[++i];
            if(ch == '.'){
                i++;
                goto LB_FRAC;
            }
            if((ch == 'e')||
               (ch == 'E')){
                    i++;
                    goto LB_EXP;
               }
            if(IS_DIGIT_GEZ(ch)){
                *len = i;
                return -1; /* leading 0s are not allowed */
            }
            *intgr = 0LL;
            *len = i;
            return 1; /* return 0 */
        }
        ch -= '0';
        if(neg) rint -= ch;
        else  rint += ch;
        i++;
    } else return -1;
    /* continue parsing integer part */
    while(i < rlen){
        ch = buf[i];
        if(IS_DIGIT_GEZ(ch)){
            /* next digit */
            if(neg){
                if((rint < CUTOFF_NEG)||(rint == CUTOFF_NEG && ch > TRESHOLD_NEG)){
                    goto LB_OFL_INT;
                }
                else{
                    rint *= 10;
                    rint -= ch - '0';
                }
            }
            else{
                if((rint > CUTOFF_POS)||(rint == CUTOFF_POS && ch > TRESHOLD_POS)){
                    goto LB_OFL_INT;
                }
                else{
                    rint *= 10;
                    rint += ch - '0';
                }
            }
        }
        else if(ch == '.') { i++; goto LB_FRAC;}
        else if((ch == 'e')||(ch == 'E')){
            *dbl = (double)rint;
            i++;
            goto LB_EXP;
        }
        else break;
        i++;
    }
    *intgr = rint;
    *len = i;
    return 1; /* the result is long long */
LB_FRAC:
    /* here we have buf[i-1] == ',' */
    /* get number of digits before decimal point */
    nbeforedp = neg?(i-2):(i-1);
    nmantissa = nbeforedp;
    /* JSON number must have at least one digit after the dec pt */
    ch = buf[i];
    if(!IS_DIGIT_GEZ(ch)){
        *len = i;
        return -1;
    }
    while(i < rlen){
        if(IS_DIGIT_GEZ(ch)){
            if(nmantissa > 18){
                /* the value is too long. It can not be represented without rounding */
                /* we threat this as overflow */
                i++;
                goto LB_OFL_DEC;
            }
            /* continue consuming the fractional part */
            if(neg){
                rint = rint * 10 - (ch - '0');
            } else{
                rint = rint * 10 + (ch - '0');
            }
        }
        else if((ch == 'e')||(ch == 'E')){
            i++;
            *dbl = (double)rint/(tens_map[nmantissa - nbeforedp]);
            goto LB_EXP;
        }
        else break;
        ch = buf[++i];
        nmantissa++;
    }
    /* the result is double */
    *dbl = (double)rint/(tens_map[nmantissa - nbeforedp]);
    *len = i;
    return 2;
LB_EXP:
    /* here we have exponent symbol at buf[i-1] */
    if(buf[i] == '-'){
        i++;
        neg_exp = 1;
    }
    else if (buf[i] == '+') i++;
    /* at least one digit must follow exponent symbol */
    if(!IS_DIGIT_GEZ(buf[i])) return -1;
    /* trim leading 0s */
    while(buf[i] == '0'){
        if(i >= rlen) break;
        i++;
    }
    while(i < rlen){
        ch = buf[i];
        if(IS_DIGIT_GEZ(ch)){
            rexp = rexp * 10 + (ch - '0');
            if(rexp > MAX_EXPONENT) goto LB_OFL_EXP;
            i++;
        } else  break;
    }
     /* Generate a floating-point number that represents the exponent.
        Process one bit at a time to combine many powers of 2 of 10.
        Then combine the exponent with the fractional part */
    for (int k = 0; rexp != 0; k++){
        if (rexp & 1){
            dexp *= pof_ten[k];
        }
        rexp >>= 1;
    }
    if(neg_exp) *dbl /= dexp;
    else *dbl *= dexp;
    *len = i;
    return 2;
LB_OFL_INT:
    while(i < rlen){
        ch = buf[i++];
        if(IS_DIGIT_GEZ(ch)) continue;
        if(ch == '.') goto LB_OFL_DEC;
        if((ch == 'e')||(ch == 'E')) goto LB_OFL_EXP;
#ifdef JSON_LIMIT_CHECK
        if(i > JSON_MAX_STRING_SIZE) return -1;
#endif // JSON_LIMIT_CHECK
    }
    *len = i;
    *str = buf;
    return -2;
LB_OFL_DEC:
    if(!IS_DIGIT_GEZ(buf[i])){
        return -1; /* format error */
    }
    i++;
    while(i < rlen){
        ch = buf[i++];
        if(IS_DIGIT_GEZ(ch)) continue;
        if((ch == 'e')||(ch == 'E')) goto LB_OFL_DEC_EXP;
#ifdef JSON_LIMIT_CHECK
        if(i > JSON_MAX_STRING_SIZE) return -1;
#endif // JSON_LIMIT_CHECK
    }
    *len = i;
    *str = buf;
    return -2;
LB_OFL_DEC_EXP:
    ch = buf[i];
    if((ch == '+')||(ch == '-')) i++;
    if(!IS_DIGIT_GEZ(buf[i])) return -1;
    i++;
LB_OFL_EXP:
    while(i < rlen){
        ch = buf[i++];
        if(!IS_DIGIT_GEZ(ch)) break;
#ifdef JSON_LIMIT_CHECK
        if(i > JSON_MAX_STRING_SIZE) return -1;
#endif // JSON_LIMIT_CHECK
    }
    *len = i;
    *str = buf;
    return -2;
}

#define IS_SURROGATE_HIGH(a) (((a>=0xd800)&&(a<=0xdbff)) ? 1 : 0)
#define IS_SURROGATE_LOW(a) (((a>=0xdc00)&&(a<=0xdfff)) ? 1 : 0)

#define _SP_ 0x20
#define _CR_ 0x0d
#define _LF_ 0x0a
#define _TAB_ 0x09

const char hex_tb[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};


/*  1 -  values to skip
*   2 - digits and '-'
*   0 - not valid in the current context
*/
static const unsigned char json_ch_map[256] = {
    0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,    // 0-15     TAB, CR, LF
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // 16-31
    1,0,34,0,0,0,0,0,0,0,0,0,44,2,0,0,   // 32-47   Space '"','-',','
    2,2,2,2,2,2,2,2,2,2,0,0,0,0,0,0,    // 48-63    0-9
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // 64-79
    0,0,0,0,0,0,0,0,0,0,0,91,0,93,0,0,  // 80-95    '[', ']'
    0,0,0,0,0,0,102,0,0,0,0,0,0,0,110,0,    // 96-111 'f','n'
    0,0,0,0,116,0,0,0,0,0,0,123,0,125,0,0,    // 112-127 't','{','}'
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // 128-143
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0     // 240-255
};

static const char hex_val[]= "0123456789abcdef";

json_ctx* json_init(void)
{
    json_ctx* new_ctx;
    if(!(new_ctx = calloc(1, sizeof(json_ctx)))){
        fprintf(stderr, "json_init() failed: memory allocation error\n");
        return NULL;
    }
    return new_ctx;
}

int json_get_nelements(json_node* parent)
{
    if((!parent)||((parent->type != JSON_ARRAY)&&(parent->type != JSON_OBJECT))){
        return -1;
    }
    int i = 0;
    json_node* nd = parent->first_child;
    while(nd){
       i++;
       nd = nd->next;
    }
    return i;
}


/** Get previous json_node sibling
*   Return NULL if it is the first child
*/
static __inline json_node* json_get_prev(json_node* nd)
{
    json_node* child = NULL;
	if(!nd) return NULL;
    if(nd->parent){
       child = nd->parent->first_child;
    }
    if((!child)||(child == nd)){
        /* we have root node or first child*/
        return NULL;
    }
    while(child->next != nd){
        child = child->next;
    }
    return child;
}

static void json_free_all(json_ctx* ctx, json_node* nd)
{
    json_node* n = nd->first_child;
    json_node* nn;
    /* delete all children nodes recursively */
    while(n){
        nn = n->next;
        json_free_all(ctx, n);
        n = nn;
    }
#ifdef JSON_NO_MEMALLOC
    /* mark the object as dummy */
    nd->type = JSON_DUMMY;
#else
    free(nd);
#endif // JSON_NO_MEMALLOC
    ctx->nused--;
}

void json_remove_node(json_ctx* ctx, json_node* nd)
{
    json_node* prev = json_get_prev(nd);
	if((!nd) || (!ctx)) return;
    /* maintain the tree structure */
    if(prev){
        prev->next = nd->next;
    }
    else{
        /* we have first child or root*/
        if(nd->parent) nd->parent->first_child = nd->next;
        else ctx->root = NULL;
    }
    json_free_all(ctx, nd);
}

void json_destroy(json_ctx* ctx)
{
    if(!ctx) return;
    if(ctx->root){
        json_free_all(ctx, ctx->root);
    }
#ifdef JSON_ON_DEBUG
    if(ctx->nused != 0){
        fprintf(stderr, "json_destroy() failed: unable to delete %d nodes\n", ctx->nused);
    }
#endif // JSON_ON_DEBUG
#ifndef JSON_NO_MEMALLOC
    free(ctx);
#endif // JSON_NO_MEMALLOC
}

json_node* json_add_last(json_ctx* ctx, json_node *parent, json_type tp, const char* key)
{
    if(ctx->nused >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("maximum # nodes reached");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    json_node* newnode;
#ifdef JSON_NO_MEMALLOC
    /* find not used slot in the factory which has JSON_DUMMY type */
    int i = 0;
    for( ; i < JSON_MAX_NODES; i++){
        if(ctx->pool[i].type == JSON_DUMMY) break;
    }
    if(i >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("JSON_MAX_NODES exceeded");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    newnode = &ctx->pool[i];
    memset(newnode, 0, sizeof(json_node));
#else
    newnode = calloc(1, sizeof(json_node));
    if(!newnode){
        JSON_SHOW_ERROR("memory allocation error");
        return NULL;
    }
#endif // JSON_NO_MEMALLOC
    ctx->nused++;
    /* populate the new object and place it in the list */
    newnode->type = tp;
    newnode->key = key;
    newnode->parent = parent;
    if(!parent){
        /* this will be root node */
        ctx->root = newnode;
    }
    else{
        json_node* nd = parent->first_child;
        if(nd){
            while(nd->next) nd = nd->next;
            nd->next = newnode;
        }
        else{
            parent->first_child = newnode;
        }
    }
    return newnode;
}

json_node* json_add_first(json_ctx* ctx, json_node *parent, json_type tp, const char* key)
{
    if(ctx == NULL){
        JSON_SHOW_ERROR("null pointer received");
        ctx->err = ERR_JSON_NULLPTR;
        return NULL;
    }
    if(parent){
       if((parent->type != JSON_ARRAY)&&(parent->type != JSON_OBJECT)){
            JSON_SHOW_ERROR("parent must be array or object");
            ctx->err = ERR_JSON_NOTACONTAINER;
            return NULL;
       }
    }
    if(ctx->nused >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("maximum # nodes reached");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    json_node* newnode;
#ifdef JSON_NO_MEMALLOC
    /* find not used slot the factory which has JSON_DUMMY type */
    int i = 0;
    for( ; i < JSON_MAX_NODES; i++){
        if(ctx->pool[i].type == JSON_DUMMY) break;
    }
    if(i >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("JSON_MAX_NODES exceeded");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    newnode = &ctx->pool[i];
    memset(newnode, 0, sizeof(json_node));
#else
    newnode = calloc(1, sizeof(json_node));
    if(!newnode){
        JSON_SHOW_ERROR("memory allocation error");
        return NULL;
    }
#endif // JSON_NO_MEMALLOC
    ctx->nused++;
    /* populate the new object and place it in the list */
    newnode->type = tp;
    newnode->key = key;
    newnode->parent = parent;
    if(!parent){
        /* this will be root node */
        ctx->root = newnode;
    }
    else{
        newnode->next = parent->first_child;
        parent->first_child = newnode;
    }
    return newnode;
}

json_node* json_add_after(json_ctx* ctx, json_node *nd, json_type tp, const char* key)
{
    if((ctx == NULL)||(nd == NULL)){
        JSON_SHOW_ERROR("null pointer received");
        ctx->err = ERR_JSON_NULLPTR;
        return NULL;
    }
    if(ctx->nused >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("maximum # nodes reached");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    json_node* newnode;
#ifdef JSON_NO_MEMALLOC
    /* find not used slot the factory which has JSON_DUMMY type */
    int i = 0;
    for( ; i < JSON_MAX_NODES; i++){
        if(ctx->pool[i].type == JSON_DUMMY) break;
    }
    if(i >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("JSON_MAX_NODES exceeded");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    newnode = &ctx->pool[i];
    memset(newnode, 0, sizeof(json_node));
#else
    newnode = calloc(1, sizeof(json_node));
    if(!newnode){
        JSON_SHOW_ERROR("memory allocation error");
        return NULL;
    }
#endif // JSON_NO_MEMALLOC
    ctx->nused++;
    /* populate the new object and place it in the list */
    newnode->type = tp;
    newnode->key = key;
    newnode->parent = nd->parent;
    newnode->next = nd->next;
    nd->next = newnode;
    return newnode;
}

json_node* json_add_before(json_ctx* ctx, json_node *nd, json_type tp, const char* key)
{
    if((ctx == NULL)||(nd == NULL)){
        JSON_SHOW_ERROR("null pointer received");
        ctx->err = ERR_JSON_NULLPTR;
        return NULL;
    }
    if(ctx->nused >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("maximum # nodes reached");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    json_node* newnode;
#ifdef JSON_NO_MEMALLOC
    /* find not used slot the factory which has JSON_DUMMY type */
    int i = 0;
    for( ; i < JSON_MAX_NODES; i++){
        if(ctx->pool[i].type == JSON_DUMMY) break;
    }
    if(i >= JSON_MAX_NODES){
        JSON_SHOW_ERROR("JSON_MAX_NODES exceeded");
        ctx->err = ERR_JSON_NODES;
        return NULL;
    }
    newnode = &ctx->pool[i];
    memset(newnode, 0, sizeof(json_node));
#else
    newnode = calloc(1, sizeof(json_node));
    if(!newnode){
        JSON_SHOW_ERROR("memory allocation error");
        return NULL;
    }
#endif // JSON_NO_MEMALLOC
    ctx->nused++;
    /* populate the new object and place it in the list */
    newnode->type = tp;
    newnode->key = key;
    newnode->parent = nd->parent;
    /* insert before */
    newnode->next = nd;
    json_node* prev = json_get_prev(nd);
    if(prev){
        prev->next = newnode;
    } else{
        nd->parent->first_child = newnode;
    }
    return newnode;
}

json_node* json_get_node(json_node* parent, const char* key)
{
    if((!parent)||(!key)){
#ifdef JSON_ON_DEBUG
        fprintf(stderr, "%s failed in line %d: null pointer received\n", __func__, __LINE__);
#endif // JSON_ON_DEBUG
        return NULL;
    }
    json_node* nd = parent->first_child;
    while(nd){
        if((nd->key)&&(!astrcmp(nd->key, key))){
            return nd;
        }
        nd = nd->next;
    }
    return NULL;
}

json_node* json_get_element(json_node* parent, int index)
{
	json_node* nd;
	if(!parent) return NULL;
    nd = parent->first_child;
    if((!nd)||(index < 0)) return NULL;
    while(index){
        index--;
        nd = nd->next;
        if(!nd) return NULL;
    }
    return nd;
}

static char* parse_string(json_ctx* ctx, char* ptr, int len)
{
    /* here we have ptr[ctx->pos-1] == '"' */
    char* beg = ptr + ctx->pos;
    char* res = beg;
    char ch;
    while(ctx->pos < len){
#ifdef JSON_LIMIT_CHECK
        if((ptr + ctx->pos - beg) > JSON_MAX_STRING_SIZE){
            JSON_SHOW_ERROR("JSON_MAX_STRING_SIZE exceeded");
            ctx->err = ERR_JSON_STRING;
            return NULL;
        }
#endif // JSON_LIMIT_CHECK
        if(ptr[ctx->pos] == '"'){
            *res = '\0';
            ctx->pos++;
            return beg;
        }
        else if((ch = ptr[ctx->pos])== '\\'){
            /* escape - see json.org*/
            ctx->pos++;
            switch(ptr[ctx->pos]){
                case '\\': /* double // */
                case '/':
                case '"':
                    *res++ = ptr[ctx->pos++];
                    break;
                case 'b': /* backspace */
                    *res++ = '\b';
                    ctx->pos++;
                    break;
                case 'f': /* form feed */
                    *res++ = '\f';
                    ctx->pos++;
                    break;
                case 'n': /* new line */
                    *res++ = '\n';
                    ctx->pos++;
                    break;
                case 'r': /* CR */
                    *res++ = '\r';
                    ctx->pos++;
                    break;
                case 't': /* tab */
                    *res++ = '\t';
                    ctx->pos++;
                    break;
                case 'u':
                    /* decode UNICODE here? */
                     if(!ctx->decode){
                        *res++ = ch;
                        break;
                    }
                    if(ctx->pos + 4 < len) break;
                    int b1, b2, b3, b4;
                    if(((b1 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)||
                       ((b2 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)||
                       ((b3 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)||
                       ((b4 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)){
                            JSON_SHOW_ERROR("invalid UNICODE escape");
                            return 0;
                       }
                    b1 <<= 12;
                    b2 <<= 8;
                    b3 <<= 4;
                    int cp = b1|b2|b3|b4;
                    if(IS_SURROGATE_LOW(cp)){
                        /* low surrogate */
                        if(ctx->pos + 6 < len) break;
                        if((ptr[ctx->pos-1] != '\\')||
                            (ptr[ctx->pos++] != 'u')||
                            ((b1 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)||
                            ((b2 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)||
                            ((b3 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)||
                            ((b4 = hex_tb[(unsigned char)ptr[ctx->pos++]]) < 0)){
                            JSON_SHOW_ERROR("invalid UTF16 low surrogate");
                            return 0;
                        }
                        b1 <<= 12;
                        b2 <<= 8;
                        b3 <<= 4;
                        int cph = b1|b2|b3|b4;
                        if(!IS_SURROGATE_HIGH(cph)){
                            JSON_SHOW_ERROR("invalid UTF16 high surrogate");
                            return 0;
                        }
                        cp = ((cp - 0xd800) << 10) + (cph - 0xdc00) + 0x0010000UL;
                    }
                    if(cp < 0x80){
                        *res++ = (char)cp;
                    }
                    else if(cp < 0x800){
                        *res++ = (cp>>6) | 0xC0;
                        *res++ = (cp & 0x3F) | 0x80;
                    }
                    else if(cp < 0x10000){
                        *res++ = (cp>>12) | 0xE0;
                        *res++ = ((cp>>6) & 0x3F) | 0x80;
                        *res++ = (cp & 0x3F) | 0x80;
                    }
                    else if (cp < 0x110000) {
                        *res++ = (cp>>18) | 0xF0;
                        *res++ = ((cp>>12) & 0x3F) | 0x80;
                        *res++ = ((cp>>6) & 0x3F) | 0x80;
                        *res++ = (cp & 0x3F) | 0x80;
                    }
                    break;
                default:
                    /* leave untouched */
                    *res++ = ch;
                    break;
            }
        }
        else{
            *res = ptr[ctx->pos];
            res++;
            ctx->pos++;
        }
    }
    JSON_SHOW_ERROR("unterminated string");
    ctx->err = ERR_JSON_STRING;
    return NULL;
}


static int get_key(json_ctx* ctx, char* ptr, int len, const char** key)
{
    char* str;
    while(ctx->pos < len){
        switch(json_ch_map[(unsigned char)ptr[ctx->pos]]){
            case '"':
                ctx->pos++;
                str = parse_string(ctx, ptr, len);
                if(!str) return 0;
                while((ctx->pos < len)&&(json_ch_map[(unsigned char)ptr[ctx->pos]] == 1)){
                    /* skip tab, cr, lf, whitespace */
                    ctx->pos++;
                }
                if(ptr[ctx->pos] == ':'){
                    ctx->pos++;
                    *key = str;
                    return ~0;
                }
                JSON_SHOW_ERROR("expected ':' key-value separator");
                ctx->err = ERR_JSON_UNEXPECTED;
                return 0;
            case 1: /* skip */
                ctx->pos++;
                break;
            case '}':
                /* don't increment ctx->pos here - we need '}' on return */
                return ~0;
            default:
                JSON_SHOW_ERROR("unexpected char");
                ctx->err = ERR_JSON_UNEXPECTED;
                return 0;
        }
    }
    JSON_SHOW_ERROR("end of buffer reached");
    ctx->err = ERR_JSON_UNEXPECTED;
    return 0;
}


static int get_value(json_ctx* ctx, json_node* parent, char* ptr, int len, const char* key)
{
    json_node* nd;
    while(ctx->pos < len){
      switch(json_ch_map[(unsigned char)ptr[ctx->pos]]){
            case 0:
                JSON_SHOW_ERROR("unexpected char");
                ctx->err = ERR_JSON_UNEXPECTED;
                return 0;
            case 1:  /* skip */
                ctx->pos++;
                break;
            case 2:  /* a sign or number? */
                {
                    nd = json_add_last(ctx, parent, JSON_DUMMY, key);
                    if(!nd) return 0;
                    int parsed = len - ctx->pos;
                    switch(json_atonum(ptr + ctx->pos, &parsed, &nd->val)){
                        case 1: /* integer */
                            nd->type = JSON_INTEGER;
                            break;
                        case 2: /* double */
                            nd->type = JSON_DOUBLE;
                            break;
                        case -1:
                            JSON_SHOW_ERROR("invalid number");
                            ctx->err = ERR_JSON_NUMBER;
                            return 0;
                        case -2: /* overflow - keep it as string value */
                            /* add NULL terminator */
                            ptr[ctx->pos + parsed] = '\0';
                            nd->type = JSON_STRING;
                            nd->val.string_value = ptr + ctx->pos;
                            parsed++;
                            break;
                        default:
                            /* should never be here */
                            return 0;
                    }
                    ctx->pos += parsed;
                    return ~0;
                }
            case '{':
#ifdef JSON_LIMIT_CHECK
                ctx->ndepth++;
                if(ctx->ndepth > JSON_MAX_DEPTH){
                    JSON_SHOW_ERROR("maximum indentation level exceeded");
                    return 0;
                }
#endif // JSON_LIMIT_CHECK
                nd = json_add_last(ctx, parent, JSON_OBJECT, key);
                if(!nd) return 0;
                ctx->pos++;
                while(ctx->pos < len){
                    const char* new_key = NULL;
                    if(!get_key(ctx, ptr, len, &new_key)) return 0;
                    if(ptr[ctx->pos] == '}'){
                        ctx->pos++;
                        return ~0;
                    }
                    if(!get_value(ctx, nd, ptr, len, new_key)) return 0;
                    while((ctx->pos < len)&&(json_ch_map[(unsigned char)ptr[ctx->pos]] == 1)){
                    /* skip tab, cr, lf, whitespace */
                        ctx->pos++;
                    }
                    if(ptr[ctx->pos] == ','){
                        ctx->pos++;
                        continue;
                    }
                    else if(ptr[ctx->pos] == '}'){
#ifdef JSON_LIMIT_CHECK
                        ctx->ndepth--;
#endif // JSON_LIMIT_CHECK
                        ctx->pos++;
                        return ~0;
                    }
                    else{
                        JSON_SHOW_ERROR("unexpected char");
                        ctx->err = ERR_JSON_UNEXPECTED;
                        return 0;
                    }
                }
                break;
            case '[':
                nd = json_add_last(ctx, parent, JSON_ARRAY, key);
                if(!nd) return 0;
                ctx->pos++;
                while(ctx->pos < len){
                    if(!get_value(ctx, nd, ptr, len, 0)) return 0;
                    while((ctx->pos < len)&&(json_ch_map[(unsigned char)ptr[ctx->pos]] == 1)){
                    /* skip tab, cr, lf, whitespace */
                        ctx->pos++;
                    }
                    if(ptr[ctx->pos] == ','){
                        ctx->pos++;
                        continue;
                    }
                    if(ptr[ctx->pos] == ']'){
                        ctx->pos++;
                        return ~0;
                    }
                    else{
                        JSON_SHOW_ERROR("unexpected char");
                        ctx->err = ERR_JSON_UNEXPECTED;
                        return 0;
                    }
                }
                break;
            case ']':
                /* don't increment ctx->pos here - we need ']' on return */
                return ~0;
            case '"':
                ctx->pos++;
                nd = json_add_last(ctx, parent, JSON_STRING, key);
                if(!nd) return 0;
                nd->val.string_value = parse_string(ctx, ptr, len);
                if(!nd->val.string_value) return 0;
                return ~0;
            case 't':
                    if(memcmpeq_32(ptr + ctx->pos, "true", 4)){
                        nd = json_add_last(ctx, parent, JSON_BOOL, key);
                        if(!nd) return 0;
                        nd->val.bool_value = ~0;
                        ctx->pos += 4;
                        return ~0;
                    }
                    JSON_SHOW_ERROR("unexpected char");
                    ctx->err = ERR_JSON_UNEXPECTED;
                    return 0;
            case 'f':
                    if(memcmpeq_32(ptr + ctx->pos, "false", 5)){
                        nd = json_add_last(ctx, parent, JSON_BOOL, key);
                        if(!nd) return 0;
                        nd->val.bool_value = 0;
                        ctx->pos += 5;
                        return ~0;
                    }
                    JSON_SHOW_ERROR("unexpected char");
                    ctx->err = ERR_JSON_UNEXPECTED;
                    return 0;
            case 'n':
                    if(memcmpeq_32(ptr + ctx->pos, "null", 4)){
                        if(!json_add_last(ctx, parent, JSON_DUMMY, key)){
                            return 0;
                        }
                        ctx->pos += 4;
                        return ~0;
                    }
                    JSON_SHOW_ERROR("unexpected char");
                    ctx->err = ERR_JSON_UNEXPECTED;
                    return 0;
            default:
                JSON_SHOW_ERROR("unexpected char");
                ctx->err = ERR_JSON_UNEXPECTED;
                return 0;
        }
    }
    JSON_SHOW_ERROR("incomplete json string");
    ctx->err = ERR_JSON_INCOMPLETE;
    return 0;
}

json_node* json_parse(json_ctx* parser, char* buf, int buflen, int to_utf8)
{
    if(!parser){
        fprintf(stderr, "json_parse() failed: null pointer received\n");
        return NULL;
    }
    parser->decode = to_utf8;
    if(get_value(parser, NULL, buf, buflen, NULL)){
        return parser->root;
    }
    return NULL;
}


/** Convert a string to valid json string using escapes where appropriate
*   Return - # bytes written or -1 on error (overflow)
*   Remark: UTF-8 encoding only allowed for input strings
*/
static int print_str(const char* in, char* out, int maxlen)
{
    int j = 0, rlen;
    if((!in)||(!out)) return -1;
    int len = astrlen(in);
    rlen = maxlen - len - 2;
    if (rlen < 0) return -1;
    out[j++] = '"';
    for (int i = 0; i < len; i++){
        char ch = in[i];
        switch (ch) {
            case '\\':
            case '"':
            case '/':
            case '\b':
            case '\t':
            case '\n':
            case '\f':
            case '\r':
                if(--rlen < 0) return -1;
                out[j++] = '\\';
                out[j++] = ch;
                break;
            default:
                if((ch >= 0)&&(ch <= 0x1f)){
                    rlen -= 5;
                    if(rlen < 0) return -1;
                    out[j++] = '\\';
                    out[j++] = 'u';
                    out[j++] = '0';
                    out[j++] = '0';
                    out[j++] = hex_val[(ch>>4)&0xf];
                    out[j++] = hex_val[ch&0xf];
                }
                else{
                    out[j++] = ch;;
                }
                break;
        }
    }
    out[j++] = '"';
    return j;
}

/** Output JSON value into preallocated buffer - not formatted
*   Return: # bytes written
*/
static int print_value(json_ctx* ctx, json_node* nd, char* buf, int rlen)
{
    int rc, len = rlen;
    switch(nd->type){
        case JSON_DUMMY:
            if(len < 4) goto ERR_OVFL1;
            *buf++ = 'n';
            *buf++ = 'u';
            *buf++ = 'l';
            *buf = 'l';
            len -= 4;
            break;
        case JSON_STRING:
            rc = print_str(nd->val.string_value, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL1;
            break;
        case JSON_INTEGER:
            rc = itoa_aux(nd->val.integer_value, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL1;
            break;
        case JSON_DOUBLE:
            rc = dtoa_aux(nd->val.double_value, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL1;
            break;
        case JSON_BOOL:
            if(nd->val.bool_value){
                if(len < 4) goto ERR_OVFL1;
                *buf++ = 't';
                *buf++ = 'r';
                *buf++ = 'u';
                *buf = 'e';
                len -= 4;
            }
            else{
                if(len < 5) goto ERR_OVFL1;
                *buf++ = 'f';
                *buf++ = 'a';
                *buf++ = 'l';
                *buf++ = 's';
                *buf = 'e';
                len -= 5;
            }
            break;
        case JSON_ARRAY:
            if(len < 2) goto ERR_OVFL1;
            *buf++ = '[';
            len--;
            nd = nd->first_child;
            if(!nd){
                if(len < 1) goto ERR_OVFL1;
                *buf = ']';
                len--;
                break;
            }
            rc = print_value(ctx, nd, buf, len);
            len -= rc;
            if((len < 1)||(rc < 0)) goto ERR_OVFL1;
            buf += rc;
            nd = nd->next;
            while(nd){
                *buf++ = ',';
                len--;
                rc = print_value(ctx, nd, buf, len);
                len -= rc;
                if((len < 0)||(rc < 0)) goto ERR_OVFL1;
                buf += rc;
                nd = nd->next;
            }
            *buf = ']';
            len--;
            break;
        case JSON_OBJECT:
            if(len < 2) goto ERR_OVFL1;
            *buf++ = '{';
            len--;
            nd = nd->first_child;
            if(!nd){
                if(len < 1) goto ERR_OVFL1;
                *buf = '}';
                len--;
                break;
            }
            if(!nd->key){
                /* there must be a key in non empty object */
                JSON_SHOW_ERROR("string is missing in non empty JSON object type");
                ctx->err = ERR_JSON_NOSTRING;
                return -1;
            }
            rc = print_str(nd->key, buf, len);
            len -= (rc + 1);
            if((len < 0)||(rc < 0)) goto ERR_OVFL1;
            buf += rc;
            *buf++ = ':';
            rc = print_value(ctx, nd, buf, len);
            len -= rc;
            if(len < 1) goto ERR_OVFL1;
            buf += rc;
            nd = nd->next;
            while(nd){
                *buf++ = ',';
                len--;
                rc = print_str(nd->key, buf, len);
                len -= (rc + 1);
                if((len < 0)||(rc < 0)) goto ERR_OVFL1;
                buf += rc;
                *buf++ = ':';
                rc = print_value(ctx, nd, buf, len);
                len -= rc;
                if((len < 1)||(rc < 0)) goto ERR_OVFL1;
                buf += rc;
                nd = nd->next;
            }
            *buf = '}';
            len--;
            break;
        default:
            JSON_SHOW_ERROR("unexpected value type");
            ctx->err = ERR_JSON_TYPE;
            return -1;
    }
    return rlen - len;
ERR_OVFL1:
    JSON_SHOW_ERROR("output buffer max length exceeded");
    ctx->err = ERR_JSON_OVERFLOW;
    return -1;
}


/** Output JSON container (array or object) to the preallocated buffer - formatted */
static int print_value_fmt(json_ctx* ctx, json_node* nd, char* buf, int rlen)
{
    int len = rlen;
    int rc, i;
    switch(nd->type){
        case JSON_DUMMY:
            if(len < 4) goto ERR_OVFL;
            *buf++ = 'n';
            *buf++ = 'u';
            *buf++ = 'l';
            *buf = 'l';
            len -= 4;
            break;
        case JSON_STRING:
            rc = print_str(nd->val.string_value, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL;
            break;
        case JSON_INTEGER:
            rc = itoa_aux(nd->val.integer_value, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL;
            break;
        case JSON_DOUBLE:
            rc = dtoa_aux(nd->val.double_value, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL;
            break;
        case JSON_BOOL:
            if(nd->val.bool_value){
                if(len < 4) goto ERR_OVFL;
                *buf++ = 't';
                *buf++ = 'r';
                *buf++ = 'u';
                *buf = 'e';
                len -= 4;
            }
            else{
                if(len < 5) goto ERR_OVFL;
                *buf++ = 'f';
                *buf++ = 'a';
                *buf++ = 'l';
                *buf++ = 's';
                *buf = 'e';
                len -= 5;
            }
            break;
        case JSON_ARRAY:
            if(len < 2) goto ERR_OVFL;
            *buf++ = '[';
            len--;
            nd = nd->first_child;
            if(!nd){
                if(len < 1) goto ERR_OVFL;
                *buf = ']';
                len--;
                break;
            }
            rc = print_value_fmt(ctx, nd, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL;
            buf += rc;
            nd = nd->next;
            while(nd){
                len--;
                if(len < 1) goto ERR_OVFL;
                *buf++ = ',';
                rc = print_value_fmt(ctx, nd, buf, len);
                len -= rc;
                if((len < 0)||(rc < 0)) goto ERR_OVFL;
                buf += rc;
                nd = nd->next;
            }
            *buf = ']';
            len--;
            break;
        case JSON_OBJECT:
            if(ctx->ndepth <= 0){
                len -= 3;
                if(len < 1) goto ERR_OVFL;
                *buf++ = '{';
                *buf++ = _CR_;
                *buf++ = _LF_;
            }
            else if(*(buf - 1) == _SP_){
                len -= 3 + ctx->ndepth;
                if(len < 1) goto ERR_OVFL;
                *buf++ = '{';
                *buf++ = _CR_;
                *buf++ = _LF_;
                for(i = 0; i < ctx->ndepth; i++)
                    *buf++ = _TAB_;
            }
            else{
                len -= (5 + (ctx->ndepth)*2);
                if(len < 1) goto ERR_OVFL;
                *buf++ = _CR_;
                *buf++ = _LF_;
                for(i = 0; i < ctx->ndepth; i++)
                    *buf++ = _TAB_;
                *buf++ = '{';
                *buf++ = _CR_;
                *buf++ = _LF_;
                for(i = 0; i < ctx->ndepth; i++)
                    *buf++ = _TAB_;
            }
            ctx->ndepth++;
            nd = nd->first_child;
            if(!nd){
                *buf = '}';
                ctx->ndepth--;
                len--;
                break;
            }
            if(!nd->key){
                /* there must be a key in non empty object */
                JSON_SHOW_ERROR("string is missing in non empty JSON object type");
                ctx->err = ERR_JSON_NOSTRING;
                return -1;
            }
            /* print the key */
            rc = print_str(nd->key, buf, len);
            len -= (rc + 2);
            if((len < 0)||(rc < 0)) goto ERR_OVFL;
            buf += rc;
            *buf++ = ':';
            *buf++ = _SP_;
            rc = print_value_fmt(ctx, nd, buf, len);
            len -= rc;
            if((len < 0)||(rc < 0)) goto ERR_OVFL;
            buf += rc;
            nd = nd->next;
            while(nd){
                len -= (3 + (ctx->ndepth - 1));
                if(len < 1) goto ERR_OVFL;
                *buf++ = ',';
                *buf++ = _CR_;
                *buf++ = _LF_;
                for(i = 0; i < (ctx->ndepth - 1); i++)
                    *buf++ = _TAB_;
                rc = print_str(nd->key, buf, len);
                len -= (rc + 2);
                if((len < 0)||(rc < 0)) goto ERR_OVFL;
                buf += rc;
                *buf++ = ':';
                *buf++ = _SP_;
                rc = print_value_fmt(ctx, nd, buf, len);
                len -= rc;
                if((len < 0)||(rc < 0)) goto ERR_OVFL;
                buf += rc;
                nd = nd->next;
            }
            ctx->ndepth--;
            len -= (3 + ctx->ndepth);
            if(len < 1) goto ERR_OVFL;
            *buf++ = _CR_;
            *buf++ = _LF_;
            for(i = 0; i < ctx->ndepth; i++)
                *buf++ = _TAB_;
            *buf++ = '}';
            break;
        default:
            JSON_SHOW_ERROR("unexpected value type");
            ctx->err = ERR_JSON_TYPE;
            return -1;
    }
    return rlen - len;
ERR_OVFL:
    JSON_SHOW_ERROR("output buffer max length exceeded");
    ctx->err = ERR_JSON_OVERFLOW;
    return -1;
}


int json_to_string(json_node* nd, char* buf, int outlen, int compact)
{
    int rc = 0;
    json_node* root = nd;
	if(!nd){
        fprintf(stderr, "Nothing to serialize\n");
        return -1;
	}
    /* get json_ctx pointer */
	while(root->parent) root = root->parent;
    json_ctx* ctx = (json_ctx*)root;
    if(compact){
        rc = print_value(ctx, nd, buf, outlen);
    }
    else{
        /* output formatted string */
        ctx->ndepth = 0;
        rc = print_value_fmt(ctx, nd, buf, outlen);
    }
    if((rc < 0)||((outlen - rc) < 1)){
        JSON_SHOW_ERROR("output buffer max length exceeded");
        ctx->err = ERR_JSON_OVERFLOW;
        return -1;
    }
    buf[rc] = '\0';
    return rc;
}

