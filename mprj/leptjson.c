#include "leptjson.h"

#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>

extern errno;

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT_1TO9(ch)    ((ch) >= '1' && (ch) <= '9')
#define HIGH_SURRO(u) ((u) >= 0xD800 && (u) <= 0xDBFF)
#define LOW_SURRO(u) ((u) >= 0xDC00 && (u) <= 0xDFFF)
#define COMBINE_HIGH_LOW_SURRO(h, l)   (0x10000 + ((h) - 0xD800) * 0x400 + ((l) - 0xDC00))

const size_t stack_init_size = 256;

typedef struct {
    const char* json;
    char *stack;
    int top, capacity;
}lept_context;

void lept_context_init(lept_context *c) {
    assert (c != NULL);
    c->stack = (char *)malloc(stack_init_size);
    c->top = 0;
    c->capacity = stack_init_size;
}

/**
 * @param c: The context that needs to be changed
 * @param src: Where the contents come from
 * @param size: Number of bytes to push into stack
 * @return void *: The start address of the pushed contents
 */
void *lept_context_push(lept_context *c, const char *src, size_t size) {
    assert (c != NULL);
    while (c->top + size > c->capacity) 
        c->capacity += c->capacity / 2;
    // Allocate new memory space
    c->stack = (char *)realloc(c->stack, c->capacity);
    memcpy(c->stack+c->top, src, size);
    c->top += size;
    return c->stack+c->top-size;
}

void *lept_context_push_single_char(lept_context *c, char ch) {
    assert(c != NULL);
    return lept_context_push(c, &ch, 1);
}

void *lept_context_pop(lept_context *c, size_t size) {
    assert (c != NULL);
    if (size > c->top) {
        fprintf(stderr, "[lept_context]: stack has no such space\n");
        return NULL;
    }
    c->top -= size;
    return c->stack + c->top;
}

void lept_free(lept_value *v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING) {
        free(v->u.str.addr);
        v->u.str.len = 0;
    }
    v->type = LEPT_NULL;
}

void lept_init(lept_value *v) {
    assert(v != NULL);
    v->type = LEPT_NULL;
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}


static int lept_parse_literal(lept_context *c, lept_value *v, const char *t)
{
    int len = strlen(t);
    if (strncmp(c->json, t, len) != 0)
        return LEPT_PARSE_INVALID_VALUE;
    c->json += len;
    int type;
    switch (*t) { 
        case 'n': type = LEPT_NULL; break;
        case 't': type = LEPT_TRUE; break;
        case 'f': type = LEPT_FALSE; break;
        default : type = -1;
    }
    if (type != -1)  {
        v->type = type;
        return LEPT_PARSE_OK;
    } else {
        return LEPT_PARSE_INVALID_VALUE;
    }
}

static int lept_parse_hex4 (lept_context *c, unsigned *u)
{
    *u = 0;
    // Assert that there must be 4 hex characters
    for (int i = 0; i < 4; ++i) {
        int j = 0;
        char ch = *(c->json + i);
        if (isdigit (ch)) j = ch - '0';
        else if (ch >= 'a' && ch <= 'f') j = 10 + ch - 'a';
        else if (ch >= 'A' && ch <= 'F') j = 10 + ch - 'A';
        else return 0;
        *u = *u * 16 + j;
    }
    c->json += 4;
    return 1;
}

static int lept_encode_utf8 (lept_context *c, unsigned u)
{
    assert (u <= 0x10FFFF);
    if (u <= 0x007F) {
        lept_context_push_single_char (c, (char)u);
        return 1;
    } else if (u <= 0x07FF) {
        lept_context_push_single_char (c, 0xC0 | ((u >> 6) & 0x1F));
        lept_context_push_single_char (c, 0x80 | ( u       & 0x3F));
        return 2;
    } else if (u <= 0xFFFF) {
        lept_context_push_single_char (c, 0xE0 | ((u >> 12) & 0x0F));
        lept_context_push_single_char (c, 0x80 | ((u >>  6) & 0x3F));
        lept_context_push_single_char (c, 0x80 | ( u        & 0x3F));
        return 3;
    } else {
        lept_context_push_single_char (c, 0xF0 | ((u >> 18) & 0x07));
        lept_context_push_single_char (c, 0x80 | ((u >> 12) & 0x3F));
        lept_context_push_single_char (c, 0x80 | ((u >>  6) & 0x3F));
        lept_context_push_single_char (c, 0x80 | ( u        & 0x3F));
        return 4;
    }
    return 0;
}

static int lept_parse_number(lept_context *c, lept_value *v)
{
    char *tmp;
    // char ch = *c->json;
    const char *start = *c->json == '-' ? c->json+1 : c->json;

    int check1 = ISDIGIT_1TO9(*start) || *start == '0' && !isdigit(*(start+1));
    int check2 = 1;

    const char *find_dot = c->json;
    while (*find_dot && *find_dot != '.') 
        ++find_dot;

    if (*find_dot == '\0') {
        check2 = 1;
    } else {
        assert(*find_dot == '.');
        if (!isdigit(*(find_dot+1))) 
            check2 = 0;
    }

    if (!check1 || !check2)  {
        return LEPT_PARSE_INVALID_VALUE;
    }

    double ret = strtod(c->json, &tmp);
    // check if overflow occurs
    if (errno == ERANGE && (ret == HUGE_VAL || ret == -HUGE_VAL)) {
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    lept_set_number(v, ret);
    if (tmp == c->json) {
        return LEPT_PARSE_INVALID_VALUE;
    }
    v->type = LEPT_NUMBER;
    c->json = tmp;
    return LEPT_PARSE_OK;
}

static int lept_parse_string(lept_context *c, lept_value *v) {
    assert(c != NULL && v != NULL);
    // Note that EXPECT will increase c->json
    EXPECT(c, '\"');
    // const char *tmp = c->json;
    int len = 0;
    for (;;) {
        switch (*c->json)
        {
            case '\"': {
                lept_set_string(v, lept_context_pop(c, len), len);
                // jump over the last quote
                c->json++;
                return LEPT_PARSE_OK;
            }
            // Deal with escape
            case '\\': {
                c->json++;
                switch (*(c->json))
                {
                    case '\"': lept_context_push_single_char(c, '\"'); goto increament;
                    case '\\': lept_context_push_single_char(c, '\\'); goto increament;
                    case '/' : lept_context_push_single_char(c, '/') ; goto increament;
                    case 'b' : lept_context_push_single_char(c, '\b'); goto increament;
                    case 'f' : lept_context_push_single_char(c, '\f'); goto increament;
                    case 'n' : lept_context_push_single_char(c, '\n'); goto increament;
                    case 'r' : lept_context_push_single_char(c, '\r'); goto increament;
                    case 't' : lept_context_push_single_char(c, '\t'); goto increament;

                    // Do increament for common case
                    increament:
                        ++c->json, ++len;
                        break;

                    // Do unicode parse
                    case 'u' : 
                    {
                        // Store unicode
                        unsigned u, h, l;
                        ++c->json;
                        // Note that lept_parse_hex4 will increate the json
                        // attribute itself
                        if (!lept_parse_hex4 (c, &h)) 
                            return LEPT_PARSE_INVALID_UNICODE_HEX;

                        if (HIGH_SURRO(h)) {
                            // Judge if the next two characters imply another 
                            // unicode.
                            if (*(c->json) != '\\' || *(c->json+1) != 'u') 
                                return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
                            else 
                            {
                                c->json += 2;
                                if (!lept_parse_hex4 (c, &l)) 
                                    return LEPT_PARSE_INVALID_UNICODE_HEX;
                                if (!LOW_SURRO(l))
                                    return LEPT_PARSE_INVALID_UNICODE_SURROGATE;
                                u = COMBINE_HIGH_LOW_SURRO(h, l);
                            }
                        } else {
                            u = h;
                        }
                        // Parse the result value into utf-8 format
                        // and push each single character into the c-stack
                        int len_ = lept_encode_utf8(c, u);
                        len += len_;
                        break;
                    }
                    // No match case, just pop out all pushed characters
                    default:
                        lept_context_pop(c, len);
                        return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            }
            case '\0': {
                return LEPT_PARSE_STRING_MISS_QUOTE;
            }
            
            default: {
                if ((unsigned char)(*c->json) < 0x20) {
                    lept_context_pop(c, len);
                    return LEPT_PARSE_INVALID_STRING_CHAR;
                }
                lept_context_push_single_char(c, *c->json);
                c->json++; 
                ++len;
                break;
            }
        }
    }
    return LEPT_PARSE_OK;
}


static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null");
        case 't':  return lept_parse_literal(c, v, "true");
        case 'f':  return lept_parse_literal(c, v, "false");
        case '\"': return lept_parse_string (c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);

        /*
            if (*c->json == '-' || isdigit(*c->json)) {
                return lept_parse_number(c, v);
            } else {
                return LEPT_PARSE_INVALID_VALUE;
            }
        */
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    lept_context_init(&c);
    c.json = json;
    lept_parse_whitespace(&c);
    int ret =  lept_parse_value(&c, v);
    if (ret != LEPT_PARSE_OK)
        return ret;
    lept_parse_whitespace(&c);
    if (*c.json != '\0') return LEPT_PARSE_ROOT_NOT_SINGULAR;
    else return LEPT_PARSE_OK;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

void lept_set_number(lept_value *v, double n) {
    assert(v != NULL);
    v->type = LEPT_NUMBER;
    v->u.num = n;
}

double lept_get_number(const lept_value *v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.num;
}

void letp_set_boolean(lept_value *v, int b) {
    assert(v != NULL);
    v->type = (b == 1) ? LEPT_TRUE : LEPT_FALSE;
}

int lept_get_boolean(const lept_value *v) {
    assert(v != NULL);
    assert(v->type == LEPT_FALSE || v->type == LEPT_TRUE);
    return v->type == LEPT_FALSE ? 0 : 1;
}

const char *lept_get_string(const lept_value *v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.str.addr;
}

size_t lept_get_string_length(const lept_value *v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.str.len;
}

void lept_set_string (lept_value *v, const char *s, size_t len) {
    assert(v != NULL);
    assert(s != NULL || len == 0);
    lept_free(v);
    v->u.str.addr = (char *)malloc(len + 1);
    memcpy(v->u.str.addr, s, len);
    v->u.str.addr[len] = '\0';
    v->u.str.len = len;
    v->type = LEPT_STRING;
}




