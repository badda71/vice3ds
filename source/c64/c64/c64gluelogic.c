/*
 * c64gluelogic.c - C64 glue logic emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "alarm.h"
#include "c64_256k.h"
#include "c64gluelogic.h"
#include "c64mem.h"
//#include "cmdline.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "vicii.h"

static int glue_logic_type = GLUE_LOGIC_DISCRETE;
static int old_vbank = 0;
static int glue_alarm_active = 0;
static alarm_t *glue_alarm = NULL;

/* ------------------------------------------------------------------------- */

static void perform_vbank_switch(int vbank)
{
    if (c64_256k_enabled) {
        c64_256k_cia_set_vbank(vbank);
    } else {
        mem_set_vbank(vbank);
    }
}

static void glue_alarm_set(void)
{
    alarm_set(glue_alarm, maincpu_clk + 1);
    glue_alarm_active = 1;
}

static void glue_alarm_unset(void)
{
    alarm_unset(glue_alarm);
    glue_alarm_active = 0;
}

static void glue_alarm_handler(CLOCK offset, void *data)
{
    perform_vbank_switch(old_vbank);
    glue_alarm_unset();
}

/* ------------------------------------------------------------------------- */

void c64_glue_undump(int vbank)
{
    perform_vbank_switch(vbank);
    old_vbank = vbank;
}

void c64_glue_set_vbank(int vbank, int ddr_flag)
{
    int new_vbank = vbank;
    int update_now = 1;

    if (glue_logic_type == 1) {
        if (((old_vbank ^ vbank) == 3) && ((vbank & (vbank - 1)) == 0) && (vbank != 0)) {
            new_vbank = 3;
            glue_alarm_set();
        } else if (ddr_flag && (vbank < old_vbank) && ((old_vbank ^ vbank) != 3)) {
            /* this is not quite accurate; the results flicker in some cases */
            update_now = 0;
            glue_alarm_set();
        }
    }

    if (update_now) {
        perform_vbank_switch(new_vbank);
    }

    old_vbank = vbank;
}

void c64_glue_reset(void)
{
    if (glue_alarm_active) {
        glue_alarm_unset();
    }

    old_vbank = 0;
    perform_vbank_switch(old_vbank);
}

/* ------------------------------------------------------------------------- */

static int set_glue_type(int val, void *param)
{
    switch (val) {
        case GLUE_LOGIC_DISCRETE:
        case GLUE_LOGIC_CUSTOM_IC:
            break;
        default:
            return -1;
    }

    glue_logic_type = val;
    return 0;
}

static resource_int_t resources_int[] = {
    { "GlueLogic", GLUE_LOGIC_CUSTOM_IC, RES_EVENT_NO, NULL,
      &glue_logic_type, set_glue_type, NULL },
    RESOURCE_INT_LIST_END
};

int c64_glue_resources_init(void)
{
    if (machine_class == VICE_MACHINE_C64) {
        resources_int[0].factory_value = GLUE_LOGIC_DISCRETE;
    }
    return resources_register_int(resources_int);
}
/*
static const cmdline_option_t cmdline_options[] =
{
    { "-gluelogictype", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "GlueLogic", NULL,
      "<Type>", "Set glue logic type (0 = discrete, 1 = 252535-01)" },
    CMDLINE_LIST_END
};

int c64_glue_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
*/
void c64_glue_init(void)
{
    glue_alarm = alarm_new(maincpu_alarm_context, "Glue", glue_alarm_handler, NULL);
}

/* ------------------------------------------------------------------------- */

/* GLUE snapshot module format:

   type | name         | description
   ------------------------------
   BYTE | type         | glue logic type
   BYTE | old vbank    | old video bank
   BYTE | alarm active | alarm is active
 */

static char snap_module_name[] = "GLUE";
#define SNAP_MAJOR 1
#define SNAP_MINOR 0

int c64_glue_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)glue_logic_type) < 0
            || SMW_B(m, (uint8_t)old_vbank) < 0
            || SMW_B(m, (uint8_t)glue_alarm_active) < 0) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}

int c64_glue_snapshot_read_module(snapshot_t *s)
{
    uint8_t major_version, minor_version;
    int snap_type, snap_alarm_active;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);
    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &snap_type) < 0
            || SMR_B_INT(m, &old_vbank) < 0
            || SMR_B_INT(m, &snap_alarm_active) < 0) {
        goto fail;
    }

    if (snap_type != glue_logic_type) {
        log_warning(LOG_DEFAULT,
                    "GlueLogic: Snapshot type %i differs from selected type %i, changing.",
                    snap_type, glue_logic_type);
        glue_logic_type = snap_type;
    }

    if (glue_alarm_active) {
        glue_alarm_unset();
    }

    glue_alarm_active = snap_alarm_active;

    if (glue_alarm_active && (glue_logic_type == 1)) {
        glue_alarm_set();
    }

    snapshot_module_close(m);
    return 0;

fail:
    if (m != NULL) {
        snapshot_module_close(m);
    }
    return -1;
}
