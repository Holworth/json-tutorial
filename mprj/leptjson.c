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
    // add 1.5 capacity
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

static int lept_parse_number(lept_context *c, lept_value *v)
{
    char *tmp;
    char ch = *c->json;
    char *start = *c->json == '-' ? c->json+1 : c->json;

    int check1 = ISDIGIT_1TO9(*start) || *start == '0' && !isdigit(*(start+1));
    int check2 = 1;

    char *find_dot = c->json;
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
    char *tmp = c->json;
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
                switch (*(c->json+1))
                {
                    case '\"': lept_context_push_single_char(c, '\"'); break;
                    case '\\': lept_context_push_single_char(c, '\\'); break;
                    case '/' : lept_context_push_single_char(c, '/') ; break;
                    case 'b' : lept_context_push_single_char(c, '\b'); break;
                    case 'f' : lept_context_push_single_char(c, '\f'); break;
                    case 'n' : lept_context_push_single_char(c, '\n'); break;
                    case 'r' : lept_context_push_single_char(c, '\r'); break;
                    case 't' : lept_context_push_single_char(c, '\t'); break;

                    default:
                        lept_context_pop(c, len);
                        return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }
                ++len;
                c->json += 2;
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




