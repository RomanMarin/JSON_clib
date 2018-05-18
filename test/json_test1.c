#include "json_clib.h"
#include <string.h>

#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

#ifdef _MSC_VER
#define lltoa(a,b,c) _i64toa(a,b,c)
#endif

typedef struct _TIMESTAMP
{
	time_t seconds; /*seconds since the Epoch */
	int milliseconds;
} TIMESTAMP;

/** the following functions used for running time comparative testing */
void get_timestamp(TIMESTAMP *ts);
char *diff_time(TIMESTAMP *start, TIMESTAMP *stop);
void get_timestamp(TIMESTAMP *ts)
{
	ts->seconds = time(NULL);
#ifdef _WIN32
	SYSTEMTIME st;
	GetSystemTime(&st);
	ts->milliseconds = st.wMilliseconds;
#else
	struct timespec tspec;
	clock_gettime(CLOCK_REALTIME, &tspec);
	ts->milliseconds = (tspec.tv_nsec / 1000000) % 1000;
#endif // _WIN32
}
char *diff_time(TIMESTAMP *start, TIMESTAMP *stop)
{
	static char str[16];
	long long diff;
	int i = 0;
	if ((start == NULL) || (stop == NULL))
		return NULL;
	diff = stop->seconds * 1000LL + stop->milliseconds - start->seconds * 1000LL - start->milliseconds;
	if (diff < 0)
	{
		*str = '-';
		i = sizeof(char);
		diff = llabs(diff);
	}
#ifdef _WIN32
	_snprintf(str + i, 16, "%05d:%02d:%02d.%03d", (int)(diff / 3600000LL), (int)((diff % 3600000) / 60000),
		(int)((diff % 60000) / 1000), (int)(diff % 1000));
#else
    snprintf(str + i, 16, "%05d:%02d:%02d.%03d", (int)(diff / 3600000LL), (int)((diff % 3600000) / 60000),
		(int)((diff % 60000) / 1000), (int)(diff % 1000));
#endif // _WIN32

	return str;
}


void json_test_itoa()
{
    TIMESTAMP start;
    TIMESTAMP stop;

char str[][256] = {
    "0",
    "42",
    "-42",
    "1234567890123456789",
    "-9876345",
    "2345678"
};

long long res[] ={0,0,0,0,0,0};

    int len;
    int j;

    int ntests = 1000000;

    printf("Input array of strings:\n\n");
    for(j=0; j < 6; j++)
        printf("%s\n", str[j]);


    printf("testing json_autonum(), # tests = %d\n", ntests);
    get_timestamp(&start);
    for(j = 0; j < ntests; j++)
    for(int i=0; i < 6; i++){
        len = strlen(str[i]);
        if(json_atonum(str[i], &len, &res[i]) != 1){
            printf("json_atonum() error!\n");
            printf("here: %s\n", str[i]);
            exit(1);
        }
        len = 0;
        memset(res, 0, sizeof(res));
    }
    get_timestamp(&stop);
    printf("Time elapsed for json_atonum %s\n\n", diff_time(&start, &stop));

    printf("testing atoll(), # tests = %d\n", ntests);
    get_timestamp(&start);
    for(j = 0; j < ntests; j++)
    for(int i=0; i < 6; i++){
        len = strlen(str[i]);
        res[i] = atoll(str[i]);
        len = 0;
        memset(res, 0, sizeof(res));
    }
    get_timestamp(&stop);
    printf("Time elapsed for atoll %s\n\n", diff_time(&start, &stop));


    /* load the data again */
    for(int i=0; i < 6; i++){
        len = strlen(str[i]);
        if(json_atonum(str[i], &len, &res[i]) != 1){
            printf("json_atonum() error!\n");
            printf("here: %s\n", str[i]);
            exit(1);
        }
    }

    printf("Input array of long long values:\n\n");
    for(j=0; j < 6; j++)
#ifdef _WIN32
        printf("%I64d\n", res[j]);
#else
        printf("%lld\n", res[j]);
#endif
    printf("testing itoa_aux(), # tests = %d\n", ntests);
    get_timestamp(&start);
    for(j = 0; j < ntests; j++)
    for(int i=0; i < 6; i++){
        if(itoa_aux(res[i], str[i], 256) == -1){
            printf("itoa_aux() error!\n");
#ifdef _WIN32
        printf("%I64d\n", res[i]);
#else
        printf("%lld\n", res[i]);
#endif
            exit(1);
        }
    }
    get_timestamp(&stop);
    printf("Time elapsed for itoa_aux %s\n\n", diff_time(&start, &stop));

    printf("result for itoa_aux:\n");
    for(j = 0; j < 6; j++)
        printf("%s\n", str[j]);


    printf("testing lltoa(), # tests = %d\n", ntests);
    get_timestamp(&start);
    for(j = 0; j < ntests; j++)
    for(int i=0; i < 6; i++){
#ifdef _WIN32
        lltoa(res[i], str[i], 10);
#else
        snprintf(str[i], 100, "%lld", res[i]);
#endif // _WIN32

    }
    get_timestamp(&stop);
    printf("Time elapsed for lltoa %s\n\n", diff_time(&start, &stop));

    printf("result for lltoa:\n");
    for(j = 0; j < 6; j++)
        printf("%s\n", str[j]);
}

void json_test_numbers()
{
    union result{
        double d;
        long long l;
        char* s;
    } result;

/* Here are valid JSON numbers */
char* str[] = {
    "0",
    "-0",
    "-0.0",
    "42",
    "-42",
    "12345678901234567890123456789",
    "12345678901234567890123456789.0",
    "123456789012345678",
    "5E3",
    "5e+3",
    "500e-3",
    "5e+45",
    "-128",
    "-128,",
    "1.0e+500000000000",
    "1.0e+5000",
    "1.0e-5000",
    "-1.0e+5000",
    "-1.0e+500000000000",
    "-62.509726999999998",
    "1.234e+3",
    "0.1",
    "3.1415926535897931",
    "3.14159265358979323846264338327950288419716939937510",
    NULL
};

/* Invalid JSON numbers */
char* str1[] = {
    "+0",
    "--5",
    "---5",
    ".0",
    "034",
    "080",
    "0x80",    /* json_atonum() will parse this and return long long value equal to 0. The following
                    'x' will be treated as unexpected char by the parser */
    "0xfB7",   /* same as above */
    "NaN",
    "Infinity",
    "-Infinity",
    "0.",
    NULL
};


    int len;
    int rc;
    int digits;

    printf("\n%s\n","...Testing json_atonum() with valid JSON numbers");
    char tmp[22];
    for(int i=0; str[i] != NULL; i++){
        len = strlen(str[i]);
        rc = json_atonum(str[i], &len, &result);
        switch(rc){
        case 1:
#ifdef _WIN32
            printf("Input: %s\n Parsed result - long long, printed with printf():\
                    long long %I64d\n", str[i], result.l);
#else
            printf("Input: %s\n Parsed result - long long, printed with printf(): \
                   long long %lld\n", str[i], result.l);
#endif
            digits = itoa_aux(result.l, tmp, 22);
            if(digits <= 0){
                printf("itoa_aux() failed!\n");
            } else{
                tmp[digits] = '\0';
                printf("itoa_aux() result: %s\n", tmp);
            }

        break;
        case 2:
            printf("Input: %s\n Parsed result - double, printed with printf():\
                    %.17g\n", str[i], result.d);
            printf("Input: %s\n Parsed result - double, printed with printf() with exp: double %e\n", str[i], result.d);
        break;
        case -1:
            printf("Input: %s Output: -1 format error\n", str[i]);
        break;
        case -2:
            printf("Input: %s Output: -2 overflow, we may use the value as string: %s\n", str[i], result.s);
        break;
        default:
            printf("Unexpected error!\n");
            break;
        }
        printf("Parsed bytes: %d\n", len);
    }
    printf("\n%s\n",".......Testing invalid JSON numbers");
    for(int i=0; str1[i] != NULL; i++){
        len = strlen(str[i]);
        rc = json_atonum(str1[i], &len, &result);
        switch(rc){
        case 1:
#ifdef _WIN32
            printf("Input: %s Output: long long %I64d\n", str1[i], result.l);
#else
            printf("Input: %s Output: long long %lld\n", str1[i], result.l);
#endif
        break;
        case 2:
            printf("Input: %s Output: double %.17g\n", str1[i], result.d);
            printf("Input: %s Output: double %e\n", str1[i], result.d);
        break;
        case -1:
            printf("Input: %s Output: -1 format error\n", str1[i]);
        break;
        case -2:
            printf("Input: %s Output: -2 we may use the value as string: %s\n", str1[i], result.s);
        break;
        default:
            printf("Unexpected error!\n");
            break;
        }
    }

}




void json_test_floats(void)
{

    int rc;

    int i;
    char s[64];
    double d[10];
    char* str[] = {
        "0.0",
        "42.0",
        "1234567.89012345",
        "0.000000000000018",
        "555555.55555555555555555",
        "-888888888888888.8888888",
        "111111111111111111111111.2222222222",
        "2.34563e+200",
        "-1.2345190876456",
        "22222222222222.0"
    };

    printf("\n\nTesting json_atonum() and dtoa_aux() - ascii to double and double to ascii conversion and \n\n");


    for(i = 0; i < 10; i++){
        int len = strlen(str[i]);
        rc=json_atonum(str[i], &len, &d[i]);
        if(rc == -1){
                printf("Format error!\n");
                d[i] = 0.0;
        }
        if(rc == -2){
                printf("Precision loss! We'll save as 0.0\n");
                d[i] = 0.0;
        }
    }
    for (i = 0; i < 10; i++) {
        memset(s, 0, 64);
        if((rc=dtoa_aux(d[i], s, 24))== -1){
            printf("Error!\n");
            exit(rc);
        }
        printf("%d: input ascii value:  %s  printf gives: %.14g, dtoa_aux() gives: %s\n", i+1, str[i], d[i], s);
    }
    return;
}


int json_test_file(char* fname)
{
    /* create a ctx contained on the stack if JSON_NO_MEMALLOC is defined */
  //  json_ctx ct;
    int rc;
    char* buf = 0;
    char* out1 = 0;
    int length;
printf("file name: %s\n", fname);

    FILE* fl = fopen(fname, "rb");

    if(!fl){
        printf("File open error");
        exit(1);
    }
    fseek (fl, 0, SEEK_END);
    length = ftell(fl);
    fseek (fl, 0, SEEK_SET);
    buf = malloc(length);
    if(!buf){
        printf("Memory allocation error");
        fclose(fl);
        exit(1);
    }

    /* read the whole file into the buffer */
    printf("...Parsing file: %s\n", fname);
    rc = fread (buf, 1, length, fl);
    if(rc != length){
        printf("Reading from file failed!\n");
        exit(1);
    }
    fclose(fl);
    printf("Loaded from file: %d bytes\n", length);


    json_ctx* ctx;
    ctx = json_init();
    if(!ctx){
        printf("json_init() failed\n");
        exit(-1);
    }
    json_node* root;
    /* here we parse the buffer */
    root = json_parse(ctx, buf, length, 1);
//    ctx = json_parse(&ct, buf, length, 1);
    if(!root){
        printf("json_parse() failed, error code: %d\n", ctx->err);
        exit(-1);
    }
//    printf("depth: %d\n", ctx->ndepth);
    printf("Parsed bytes (cursor position): %d\n", ctx->pos);
    printf("Used nodes: %d\n", ctx->nused);
    printf("Used memory: %d bytes\n", (int)(ctx->nused * sizeof(json_node) + sizeof(json_ctx)));
    if(!ctx->err){
        printf("Parsed OK!\n");
    }else{
        printf("Parser error code: %d\n", ctx->err);
    }
    printf("Root node contains: %d elements\n", json_get_nelements(ctx->root));


    printf("\nGet element by key-name \"size\"\n");
    json_node* nd = json_get_node(ctx->root, "size");
    if(nd){
        printf("Element found! its type is: %d\n", nd->type);
        if(nd->type == JSON_STRING)
            printf("The value is string: %s\n", nd->val.string_value);
        else
            printf("The value is not a string type\n");
    }
    else{
        printf("\nElement not found\n");
    }

    printf("...Printing JSON object - compact form\n");
    /* we may use same buffer for output as soon as it is in compact form and must fit in it */
    if((rc=json_to_string(ctx->root, buf, ctx->pos, 1)) < 0)
            printf("Print json error!\n");
    else{
        printf("\n%s\n", buf);
        printf("\nOutput string size: %d bytes\n", rc);
    }

    /* when we don't need the object and its content, the following functions will release everything */
    /* alternatively we could call only json_destroy(), which releases everything */
    json_remove_node(ctx, ctx->root);
    if(!ctx->root){
        printf("Root node is freed, nnodes = %d\n", ctx->nused);
    }
    json_destroy(ctx);
    /* initialize the context again */
    ctx = json_init();
    if(!ctx){
        printf("json_init() failed\n");
        exit(-1);
    }
    /* let's parse the compact string we created which is now in the buffer - buf and occupies ctx->pos bytes */
    root = json_parse(ctx, buf, length, 0);
    if(!root){
        printf("json_parse() failed, error code: %d\n", ctx->err);
        exit(-1);
    }

    printf("\nParsed bytes (cursor position): %d\n", ctx->pos);
    printf("Used nodes: %d\n", ctx->nused);
    printf("Used memory: %d bytes\n", (int)(ctx->nused * sizeof(json_node) + sizeof(json_ctx)));
    if(!ctx->err){
        printf("Parsed OK!\n");
    }else{
        printf("Parser error code: %d\n", ctx->err);
    }

    /* we will use extra buffer for formatted output */
    out1 = calloc(1, length);
    if(!out1){
        printf("Memory allocation error");
        fclose(fl);
        free(buf);
        exit(1);
    }

    printf("...Printing JSON object again - formatted output\n");
    /* now we print it formatted in human readable form. A bigger buffer is needed as
        we insert additional control symbols.
        We should have identical json object we read from the file at the beginning.
        The format may vary though  */
    if((rc=json_to_string(ctx->root, out1, length, 0)) < 0)
            printf("Print json error!\n");
    else{
        printf("\n%s\n", out1);
        printf("\nOutput string size: %d bytes\n", rc);
    }

    free(buf);
    free(out1);
    return rc;
}




int main()
{
    int rc;

/* this is a big json file with a lot of floating point numbers */
//rc = json_test_file("./test/sample/example_6big.json");


rc = json_test_file("./test/sample/example_1.json");
rc = json_test_file("./test/sample/example_2.json");
rc = json_test_file("./test/sample/example_3.json");
rc = json_test_file("./test/sample/example_4.json");

    json_test_numbers();

    json_test_itoa();

    json_test_floats();

    return 0;

}

