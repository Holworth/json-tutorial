#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stdlib.h>

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct lept_value lept_value;
typedef struct lept_member lept_member;

struct lept_value {
    lept_type type;
    union {
        double num;
        struct { char *addr; size_t len;} str;
        struct { lept_value *addr; size_t size; } arr;
        struct { lept_member *addr; size_t cnt; } obj;
    } u;
};

/*
 *  struct lept_member denotes a single key-value
 *  map which is denoted as "key : value". Here key
 *  must be a json string.
 */
struct lept_member {
    char *key; size_t klen;
    lept_value *value;
};



enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_STRING_MISS_QUOTE,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_INCOMPLETE_ARRAY,
    LEPT_PARSE_INCOMPLETE_OBJECT,
    LEPT_PARSE_OBJECT_MISSING_VALUE
};

int lept_parse(lept_value* v, const char* json);

lept_type lept_get_type(const lept_value* v);

double lept_get_number(const lept_value *v);
void lept_set_number(lept_value *v, double n);

int lept_get_boolean(const lept_value *v);
void lept_set_boolean(lept_value *v, int b);

const char *lept_get_string(const lept_value *v);
size_t lept_get_string_length(const lept_value *v);
void lept_set_string(lept_value *v, const char *s, size_t len);

size_t lept_get_array_size (const lept_value *v);
lept_value *lept_get_array_element (const lept_value *v, size_t idx);

size_t lept_get_object_size (const lept_value *v);
const char *lept_get_object_key (const lept_value *v, size_t idx);
size_t lept_get_object_keylen (const lept_value *v, size_t idx);
lept_value *lept_get_object_value (const lept_value *v, size_t idx);


void lept_init(lept_value *v);
void lept_free(lept_value *v);

#endif /* LEPTJSON_H__ */
