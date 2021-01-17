#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%lf")

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_STRING(expect, addr, len) EXPECT_EQ_BASE((strlen(expect) == len && strncmp(expect, addr, len) == 0), expect, addr, "%s")

#define TEST_NUMBER(expect, json) \
    do  {\
        lept_value v;\
        lept_init(&v); \
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v)); \
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
    } while (0)  

#define TEST_ERROR(error, json) \
    do {\
        lept_value v;   \
        lept_init(&v);  \
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));    \
    } while (0)

static void test_parse_null() {
    lept_value v;
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
    lept_value v;
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "   true   "));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
    lept_value v;
    lept_init(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "   false   "));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */;
}

static void test_parse_expect_value() {

    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "     ");

}

static void test_parse_invalid_value() {

    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?"  );

#if 1
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
#endif

}

static void test_parse_root_not_singular() {
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

#if 0
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
#endif
}

static void test_parse_number_too_big() {
#if 1
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}

static void test_access_string() {
    lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "hello", 5);
    EXPECT_EQ_STRING("hello", lept_get_string(&v), lept_get_string_length(&v));
    // if (strncmp("hello", lept_get_string(&v), lept_get_string_length(&v)) == 0) 
    //     puts("YES");
    lept_free(&v);
}

#define TEST_STRING(expect, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
#if 1
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}


static void test_parse_missing_quotation_mark() {
    TEST_ERROR(LEPT_PARSE_STRING_MISS_QUOTE, "\"");
    TEST_ERROR(LEPT_PARSE_STRING_MISS_QUOTE, "\"abc");
}

static void test_parse_invalid_string_escape() {
#if 1
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 1
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_array () {
    const char *test1 = "[ [1,2], [2, 3], [4,5,6]]";
    lept_value v;
    lept_init (&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, test1));
    EXPECT_EQ_INT(3, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_array_element(&v, 2)));

    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(lept_get_array_element(&v, 0), 0)));
    EXPECT_EQ_INT(1, lept_get_number(lept_get_array_element(lept_get_array_element(&v, 0), 0)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(lept_get_array_element(lept_get_array_element(&v, 0), 1)));
    EXPECT_EQ_INT(5, lept_get_number(lept_get_array_element(lept_get_array_element(&v, 2), 1)));

    const char *test2 = "[\"hello, world\", 1, true, null, [\"hello\", false, [1, 3, 4]], 1234]";
    // Test if parse ok
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, test2));

    // Test if the array attributes are true
    EXPECT_EQ_INT(6, lept_get_array_size(&v));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_array_element(&v, 4)));

    // Test the type of every array element
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type((lept_get_array_element(&v, 0))));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type((lept_get_array_element(&v, 1))));
    EXPECT_EQ_INT(LEPT_TRUE  , lept_get_type((lept_get_array_element(&v, 2))));
    EXPECT_EQ_INT(LEPT_NULL  , lept_get_type((lept_get_array_element(&v, 3))));
    EXPECT_EQ_INT(LEPT_ARRAY , lept_get_type((lept_get_array_element(&v, 4))));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type((lept_get_array_element(&v, 5))));

    // Test the value of each array element
    // tmp and tmp2 points to the array
    lept_value *tmp = lept_get_array_element(&v, 0), *tmp2 = lept_get_array_element(&v, 4);
    EXPECT_EQ_STRING("hello, world", lept_get_string(tmp), lept_get_string_length(tmp));

    tmp = lept_get_array_element(lept_get_array_element(&v, 4), 0);
    EXPECT_EQ_STRING("hello", lept_get_string(tmp), lept_get_string_length(tmp));

    tmp2 = lept_get_array_element(tmp2, 2);
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(tmp2));
    EXPECT_EQ_INT(3, lept_get_array_size(tmp2));
    EXPECT_EQ_INT(3, lept_get_number(lept_get_array_element(tmp2, 1)));
}


static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_access_string();
    test_parse_string();

    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();

    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();

    test_parse_array();
}

int main() {
     test_parse();
    // test_parse_array();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
