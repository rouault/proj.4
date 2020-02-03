/* <<< Release Notice for library >>> */

#define PROJ_IGNORE_DEPRECATION_WARNINGS

#include "proj.h"
#include "proj_internal.h"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

char const pj_release[] =
    "Rel. "
    STR(PROJ_VERSION_MAJOR)"."
    STR(PROJ_VERSION_MINOR)"."
    STR(PROJ_VERSION_PATCH)", "
    "March 1st, 2020";

const char *pj_get_release() {
    return pj_release;
}
