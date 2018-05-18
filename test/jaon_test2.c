#include "json_clib.h"

#include <string.h> // for memcpy() nad memset()


//    char sample[] = "{\"\":[{}]}";

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


/*
char* sample = "[\
      {\
         \"precision\": \"zip\",\
         \"Latitude\":  37.7668,\
         \"Longitude\": -122.3959,\
         \"Address\":   \"\",\
         \"City\":      \"SAN FRANCISCO\",\
         \"State\":     \"CA\",\
         \"Zip\":       \"94107\",\
         \"Country\":   \"US\"\
      },\
      {\
         \"precision\": \"zip\",\
         \"Latitude\":  37.371991,\
         \"Longitude\": -122.026020,\
         \"Address\":   \"\",\
         \"City\":      \"SUNNYVALE\",\
         \"State\":     \"CA\",\
         \"Zip\":       \"94085\",\
         \"Country\":   \"US\"\
      }\
   ]";
*/

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
                printf("Found key-value pair:  %s:%s", url->key, url->val.string_value);
            }
            else printf("Url not found");
        }
        else printf("Thumbnail not found");
    }
    else printf("Image not found");

    json_destroy(ctx);
    return 0;
}
