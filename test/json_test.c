#include "json_clib.h"

#ifdef _WIN32
#define WINPAUSE system("pause")
#else
#define WINPAUSE
#endif

#include <string.h>

#define MY_BUF_SIZE 512


int main(void)
{
// we use this stack array for serialization and parsing of JSON
    char out[MY_BUF_SIZE];

// we need preallocated values to construct JSON object as soon as
// it will store only pointers for strings and keys. Integers, doubles
// and bools will be stored after parsing explicitly
    char* keys[] = {"color", "size", "day", "height", "price"};
    char* color_val[] = {"red", "blue", "green", "white"};
    char* size_val[] = {"big", "medium", "small"};
    char* day_val[] = {"mon", "tue", "wed", "thu", "fri"};
    int height_val[] = {456, 1234562, 98077};
    double price_val[] ={1.09, 34.4236, 17.45, 0.12};

    int i, rc;
// basic JSON struct to keep JSON data
    json_ctx* ctx;

// JSON nodes will be created
    json_node* root;
    json_node* array;
    json_node* nd;

    printf("STEP1: initialize JSON\n");
    ctx = json_init();
    if(!ctx){
        printf("json_init() failed\n");
        return -1;
    }
    printf("STEP2: add JSON root node - OBJECT type\n");
    root = json_add_first(ctx, NULL, JSON_OBJECT, NULL);
    if(!ctx){
        printf("json_add_first() failed\n");
        return -1;
    }
    printf("root json_node added successfully\n");

    printf("STEP3: add values inside arrays for keys 'color', 'size', 'day', 'price'\n");
//Add array value for the given key.
    array = json_add_last(ctx, root, JSON_ARRAY, keys[0]);
    if(!array){
        printf("json_add_last() failed on cerate array value\n");
        return -1;
    }
    printf("added array value for the key %s\n",keys[0]);
// fill the array with values
    for(i=0; i < 4; i++){
        nd = json_add_last(ctx, array, JSON_STRING, keys[0]);
        if(!nd){
            printf("json_add_last() failed on create array value\n");
            return -1;
        }
    // set the value explicitly
        nd->val.string_value = color_val[i];
    }
    array = json_add_last(ctx, root, JSON_ARRAY, keys[1]);
    if(!array){
        printf("json_add_last() failed on cerate array value\n");
        return -1;
    }
    printf("added array value for the key %s\n",keys[1]);
    for(i=0; i < 3; i++){
        nd = json_add_last(ctx, array, JSON_STRING, keys[1]);
        if(!nd){
            printf("json_add_last() failed on create array value\n");
            return -1;
        }
    // set the value for explicitly
        nd->val.string_value = size_val[i];
    }
    array = json_add_last(ctx, root, JSON_ARRAY, keys[2]);
    if(!array){
        printf("json_add_last() failed on creating array value\n");
        return -1;
    }
    printf("added array value for the key %s\n",keys[2]);
// fill the array with values
    for(i=0; i < 5; i++){
        nd = json_add_last(ctx, array, JSON_STRING, keys[2]);
        if(!nd){
            printf("json_add_last() failed on creating array value\n");
            return -1;
        }
    // set the value explicitly
        nd->val.string_value = day_val[i];
    }
    array = json_add_last(ctx, root, JSON_ARRAY, keys[3]);
    if(!array){
        printf("json_add_last() failed on cerate array value\n");
        return -1;
    }
    printf("added array value for the key %s\n",keys[3]);
    for(i=0; i < 3; i++){
// this array will keep double values
        nd = json_add_last(ctx, array, JSON_INTEGER, keys[3]);
        if(!nd){
            printf("json_add_last() failed on cerate array value\n");
            return -1;
        }
// set the value explicitly
        nd->val.integer_value = height_val[i];
    }

    array = json_add_last(ctx, root, JSON_ARRAY, keys[4]);
    if(!array){
        printf("json_add_last() failed on cerate array value\n");
        return -1;
    }
    for(i=0; i < 4; i++){
// this array will keep double values
        nd = json_add_last(ctx, array, JSON_DOUBLE, keys[4]);
        if(!nd){
            printf("json_add_last() failed on cerate array value\n");
            return -1;
        }
// set the value explicitly
        nd->val.double_value = price_val[i];
    }
    printf("All arrays are filled with values\n\n");
    printf("STEP4: serialize JSON object in the buffer and print it in compact form\n");
    rc = json_to_string(root, out, MY_BUF_SIZE, 1);
    if(rc == -1){
        printf("json_to_string() failed\n");
        return -1;
    }
    printf("Serialized JSON - compact form:\n\n %s\n\n", out);

    printf("STEP5: serialize JSON object in the buffer and print it in formatted form\n");
    rc = json_to_string(root, out, MY_BUF_SIZE, 0);
    if(rc == -1){
        fprintf(stderr, "json_to_string() failed\n");
        return -1;
    }
    printf("Serialized JSON value - formatted:\n\n %s\n\n", out);

    printf("STEP6: remove 'day' key and its values from the JSON object and print result formatted\n");
// get node pointer first
    nd = json_get_node(root, "day");
    if(!nd){
        printf("key '%s' in root not found, exiting\n", keys[2]);
        return -1;
    }
    json_remove_node(ctx, nd);
    memset(out, 0, MY_BUF_SIZE);
    rc = json_to_string(root, out, MY_BUF_SIZE, 0);
    if(rc == -1){
        printf("json_to_string() failed\n");
        return -1;
    }
    printf("Serialized JSON value with 'day' key removed - formatted form:\n\n %s\n\n", out);

    printf("STEP7: delete JSON object (ctx struct) and parse the string we have from STEP5\n");
    json_destroy(ctx);
    ctx = NULL;
    root = NULL;
    printf("JSON destroyed\n");
    ctx = json_init();
    if(!ctx){
        printf("ctx initialization failed\n");
        return -1;
    }
    printf("JSON initialized\nNow parsing..\n");
    root = json_parse(ctx, out, MY_BUF_SIZE, 1);
    if(!root){
        printf("json_parse() failed, error code: %d\n", ctx->err);
        return -1;
    }
    printf("JSON string parsed OK\n\n");
    printf("STEP8: serialize newly parsed JSON object in the buffer and print it in formatted form\n \
        We're fine if the object is the same one we had after STEP5\n");
// we use same buffer as we print in compact form
    rc = json_to_string(ctx->root, out, MY_BUF_SIZE, 0);
    if(rc == -1){
        printf("json_to_string() failed\n");
        return -1;
    }
    printf("Serialized JSON value - formatted:\n\n %s\n\n", out);
    printf("STEP9: delete JSON object\n");
    json_destroy(ctx);
    printf("JSON destroyed, exiting..\n\n");

    return 0;
}
