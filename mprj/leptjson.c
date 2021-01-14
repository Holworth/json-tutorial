#include "leptjson.h"

#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

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
    v->num = strtod(c->json, &tmp);
    if(tmp == c->json) {
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
        default:   
            if (*c->json == '-' || isdigit(*c->json)) {
                return lept_parse_number(c, v);
            } else {
                return LEPT_PARSE_INVALID_VALUE;
            }
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    int ret =  lept_parse_value(&c, v);
    if (ret == LEPT_PARSE_EXPECT_VALUE || ret == LEPT_PARSE_INVALID_VALUE)
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
