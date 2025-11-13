/*******************************************************************************
 *    NEM Wallet
 *    (c) 2020 Ledger
 *    (c) 2020 FDS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/
#include "idle_menu.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "bagl_utils.h"
#include "glyphs.h"
#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#endif
#include "main_std_app.h"
#include "display.h"

#ifdef HAVE_BAGL
UX_STEP_NOCB(ux_idle_flow_1_step,
             pnn,
             {
                 &C_icon_NEM,
                 "Welcome to",
                 "NEM Wallet",
             });

UX_STEP_NOCB(ux_idle_flow_2_step,
             bn,
             {
                 "Version",
                 APPVERSION,
             });

UX_STEP_VALID(ux_idle_flow_3_step,
              pb,
              app_exit(),
              {
                  &C_icon_dashboard_x,
                  "Quit",
              });

const ux_flow_step_t* const ux_idle_flow[] = {
    &ux_idle_flow_1_step,
    &ux_idle_flow_2_step,
    &ux_idle_flow_3_step,
    FLOW_END_STEP,
};

void display_idle_menu() {
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

#else  // HAVE_BAGL

// 'About' menu
#define SETTING_INFO_NB 3
static const char *const INFO_TYPES[SETTING_INFO_NB] = {"Version", "Developer", "Copyright"};
static const char *const INFO_CONTENTS[SETTING_INFO_NB] = {APPVERSION, "Ledger", "Ledger (c) 2025"};

static const nbgl_contentInfoList_t infoList = {
    .nbInfos = SETTING_INFO_NB,
    .infoTypes = INFO_TYPES,
    .infoContents = INFO_CONTENTS,
};

void display_idle_menu(void) {
    nbgl_useCaseHomeAndSettings(APPNAME,
                                &ICON_APP_HOME,
                                NULL,
                                INIT_HOME_PAGE,
                                NULL,
                                &infoList,
                                NULL,
                                app_exit);
}

#endif  // HAVE_BAGL
