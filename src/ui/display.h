#pragma once

#include <stdbool.h>  // bool

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define ICON_APP_HOME C_icon_NEM
#elif defined(TARGET_STAX) || defined(TARGET_FLEX)
#define ICON_APP_HOME C_stax_app_nem_64px
#elif defined(TARGET_APEX_P)
#define ICON_APP_HOME C_apex_app_nem_48px
#endif
