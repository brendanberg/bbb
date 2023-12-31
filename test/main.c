#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
// Must be included last; comment to prevent include reordering
#include <cmocka.h>
#include "table.c"

int main(void) {
    const struct CMUnitTest tests[] = {
        // Don't reformat the array initializer
        cmocka_unit_test(test_table_symbol_resize),
        cmocka_unit_test(test_table_symbol_push),
        cmocka_unit_test(test_table_symbol_define),
        cmocka_unit_test(test_table_label_push),
        cmocka_unit_test(test_table_label_resize),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
