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
#include <os_io_seproxyhal.h>
#include <ux.h>
#include "glyphs.h"
#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#endif

static void app_quit(void) {
    // exit app here
    os_sched_exit(-1);
}

#ifdef HAVE_BAGL
UX_STEP_NOCB(
        ux_idle_flow_1_step,
        pnn,
        {
            &C_icon_NEM,
            "Welcome to",
            "NEM Wallet",
        });

UX_STEP_NOCB(
        ux_idle_flow_2_step,
        bn,
        {
            "Version",
            APPVERSION,
        });

UX_STEP_VALID(
        ux_idle_flow_3_step,
        pb,
        app_quit(),
        {
            &C_icon_dashboard_x,
            "Quit",
        });

const ux_flow_step_t * const ux_idle_flow [] = {
        &ux_idle_flow_1_step,
        &ux_idle_flow_2_step,
        &ux_idle_flow_3_step,
        FLOW_END_STEP,
};

void display_idle_menu() {
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

#else // HAVE_BAGL

// 'About' menu
static const char* const INFO_TYPES[] = {"Version"};
static const char* const INFO_CONTENTS[] = {APPVERSION};

static bool nav_callback(uint8_t page, nbgl_pageContent_t* content) {
    UNUSED(page);
    content->type = INFOS_LIST;
    content->infosList.nbInfos = 1;
    content->infosList.infoTypes = (const char**) INFO_TYPES;
    content->infosList.infoContents = (const char**) INFO_CONTENTS;
    return true;
}

static void ui_menu_about(void) {
    nbgl_useCaseSettings(APPNAME, 0, 1, false, display_idle_menu, nav_callback, NULL);
}

void display_idle_menu(void) {
    nbgl_useCaseHome(APPNAME, &C_stax_app_nem_64px,
                     NULL, false, ui_menu_about, app_quit);
}

#endif // HAVE_BAGL
