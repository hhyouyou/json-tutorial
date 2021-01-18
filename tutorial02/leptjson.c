#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>
#include <math.h>


#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
typedef struct {
    const char *json;
} lept_context;

static void lept_parse_whitespace(lept_context *c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_true(lept_context *c, lept_value *v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context *c, lept_value *v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context *c, lept_value *v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context *c, lept_value *v) {
    char *end;
    /* \TODO validate number
     * number = [ "-" ] int [ frac ] [ exp ]
     * int = "0" / digit1-9 *digit
     * frac = "." 1*digit
     * exp = ("e" / "E") ["-" / "+"] 1*digit
     * */

    /* 校验 number = [ "-" ] int [ frac ] [ exp ] */
    /* const char *check  = c->json;
     while (*check != '\0') {
         if (*check != '-' && *check != '+' && !ISDIGIT(*check) && *check != '.' && *check != 'e' && *check != 'E') {
             return LEPT_PARSE_INVALID_VALUE;
         }
         check++;
     }*/

    const char *p = c->json;
    /* 以数字或者'-'开头 */
    if (*p != '-' && !ISDIGIT(*p)) {
        return LEPT_PARSE_INVALID_VALUE;
    }
    /* 如果以'0' 开头, 数字只能是0 */
    if (*p == '0') {
        if (*(p + 1) != '\0') {
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    p++;
    /* 如果有数字  跳过整数 数字部分 */
    while (ISDIGIT(*p)) {
        p++;
    }
    /* 小数部分 后面是 >=1 个的数字 */
    if (*p == '.') {
        if (!ISDIGIT(*(p + 1))) {
            return LEPT_PARSE_INVALID_VALUE;
        }
        /* 如果有数字 跳过小数 数字部分 */
        while (ISDIGIT(*p)) {
            p++;
        }
    }
    /* 指数部分 exp = ("e" / "E") ["-" / "+"] 1*digit */
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '-' || *p == '+') {
            p++;
        }
        if (!ISDIGIT(*p)) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    double result = strtod(c->json, &end);
    if (errno == ERANGE || result == HUGE_VAL || result == -HUGE_VAL) {
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }

    v->n = result;
    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context *c, lept_value *v) {
    switch (*c->json) {
        case 't':
            return lept_parse_true(c, v);
        case 'f':
            return lept_parse_false(c, v);
        case 'n':
            return lept_parse_null(c, v);
        default:
            return lept_parse_number(c, v);
        case '\0':
            return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value *v, const char *json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value *v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value *v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
