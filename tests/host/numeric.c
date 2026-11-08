#include "test.h"
#include "compiler/common/host/numeric.h"

#include <errno.h>
#include <locale.h>
#include <math.h>
#include <string.h>

bool test_host_numbers(void) {
    char saved[256];
    const char *current = setlocale(LC_NUMERIC, NULL);
    CHECK(current != NULL && strlen(current) < sizeof(saved));
    strcpy(saved, current);
    const char *candidates[] = {"de_DE.UTF-8", "German_Germany.1252", "C"};
    const char *selected = NULL;
    for (size_t index = 0; index < 3 && selected == NULL; ++index) {
        selected = setlocale(LC_NUMERIC, candidates[index]);
    }
    CHECK(selected != NULL);
    char active[256];
    CHECK(strlen(selected) < sizeof(active));
    strcpy(active, selected);

    char *end;
    double value;
    CHECK(rv_host_parse_real("1.25tail", &end, &value));
    CHECK(errno == 0 && value == 1.25 && strcmp(end, "tail") == 0);
    CHECK(rv_host_parse_real("-0", &end, &value));
    CHECK(errno == 0 && *end == '\0' && signbit(value));
    CHECK(rv_host_parse_real("1e999", &end, &value) && errno == ERANGE);
    CHECK(rv_host_parse_real("1e-999", &end, &value) && errno == ERANGE);

    char buffer[64];
    CHECK(rv_host_format_real(buffer, sizeof(buffer), "%.17g", 1.25) == 4);
    CHECK(strcmp(buffer, "1.25") == 0);
    CHECK(rv_host_format_real(buffer, sizeof(buffer), "%a", 1.25) > 0);
    CHECK(strchr(buffer, ',') == NULL);
    CHECK(rv_host_parse_real(buffer, &end, &value) && *end == '\0' && value == 1.25);
    CHECK(strcmp(setlocale(LC_NUMERIC, NULL), active) == 0);
    CHECK(setlocale(LC_NUMERIC, saved) != NULL);
    return true;
}
