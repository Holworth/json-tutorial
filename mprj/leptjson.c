#include "leptjson.h"

#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>

extern errno;

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT_1TO9(ch)    ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

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
    if (errno == ERANGE && (ret == HUGE_VAL || ret == -HUGE_VAL)) {
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    v->num = ret;
    if (tmp == c->json) {
        return LEPT_PARSE_INVALID_VALUE;
    }
    v->type = LEPT_NUMBER;
    c->json = tmp;
    return LEPT_PARSE_OK;
}


static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null");
        case 't':  return lept_parse_literal(c, v, "true");
        case 'f':  return lept_parse_literal(c, v, "false");
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
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    int ret =  lept_parse_value(&c, v);
    if (ret == LEPT_PARSE_EXPECT_VALUE || ret == LEPT_PARSE_INVALID_VALUE
        || ret == LEPT_PARSE_NUMBER_TOO_BIG)
        return ret;
    lept_parse_whitespace(&c);
    if (*c.json != '\0') return LEPT_PARSE_ROOT_NOT_SINGULAR;
    else return LEPT_PARSE_OK;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value *v) {
    assert(v->type == LEPT_NUMBER);
    return v->num;
}
