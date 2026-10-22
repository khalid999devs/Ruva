#define _GNU_SOURCE

#include "../internal.h"
#include "compiler/common/host/numeric.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>

bool rv_check_integer(RvChecker *checker, RvAstExpr *expression, bool allow_minimum) {
    uint64_t limit = allow_minimum ? UINT64_C(2147483648) : INT32_MAX;
    uint64_t value = 0;
    const RvToken *token = expression->as.atom;
    for (size_t index = 0; index < token->length; ++index) {
        unsigned int digit = (unsigned int)(token->spelling[index] - '0');
        if (value > (limit - digit) / 10) {
            return rv_check_fail(checker, RV_CHECK_TYPE, expression->span,
                                 "integer literal is outside the signed 32-bit range");
        }
        value = value * 10 + digit;
    }
    expression->number.integer = (int64_t)value;
    expression->type.kind = RV_TYPE_INT;
    return true;
}

bool rv_check_real(RvChecker *checker, RvAstExpr *expression) {
    char *end;
    double value;
    if (!rv_host_parse_real(expression->as.atom->spelling, &end, &value)) {
        return rv_check_fail(checker, RV_CHECK_ALLOCATION, expression->span,
                             "cannot create the numeric locale");
    }

    bool valid = errno != ERANGE && *end == '\0' && isfinite(value);
    if (!valid) {
        return rv_check_fail(checker, RV_CHECK_TYPE, expression->span,
                             "real literal is outside the finite double range");
    }
    expression->number.real = value;
    expression->type.kind = RV_TYPE_REAL;
    return true;
}
