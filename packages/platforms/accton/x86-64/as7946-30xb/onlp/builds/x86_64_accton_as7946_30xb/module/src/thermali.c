/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

static char* ipmi_devfiles__[] = { /* must map with onlp_thermal_id */
    NULL,
    NULL,
    "/sys/devices/platform/as7946_30xb_thermal/temp1_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp2_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp3_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp4_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp5_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp6_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp7_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp8_input",
    "/sys/devices/platform/as7946_30xb_thermal/temp9_input",
    "/sys/devices/platform/as7946_30xb_psu/psu1_temp1_input",
    "/sys/devices/platform/as7946_30xb_psu/psu1_temp2_input",
    "/sys/devices/platform/as7946_30xb_psu/psu1_temp3_input",
    "/sys/devices/platform/as7946_30xb_psu/psu2_temp1_input",
    "/sys/devices/platform/as7946_30xb_psu/psu2_temp2_input",
    "/sys/devices/platform/as7946_30xb_psu/psu2_temp3_input",
};

static char* cpu_coretemp_files[] = {
    "/sys/devices/platform/coretemp.0*temp2_input",
    "/sys/devices/platform/coretemp.0*temp3_input",
    "/sys/devices/platform/coretemp.0*temp4_input",
    "/sys/devices/platform/coretemp.0*temp5_input",
    "/sys/devices/platform/coretemp.0*temp6_input",
    "/sys/devices/platform/coretemp.0*temp7_input",
    "/sys/devices/platform/coretemp.0*temp8_input",
    "/sys/devices/platform/coretemp.0*temp9_input",
    NULL,
};

#define AS7946_30XB_THERMAL_CAPS (ONLP_THERMAL_CAPS_GET_TEMPERATURE | \
			     ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD | \
			     ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD | \
			     ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD)

#define AS7946_30XB_PSU_CAPS (ONLP_THERMAL_CAPS_GET_TEMPERATURE | \
			     ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD)

/* Static values */
static onlp_thermal_info_t linfo[] = {
    { }, /* Not used */
    { { ONLP_THERMAL_ID_CREATE(THERMAL_CPU_CORE), "CPU Core", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_CPU_MODULE), "CPU Module LM75_4B", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAINBOARD), "Main Board TMP432_4C", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAINBOARD), "Main Board TMP432_4C", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAINBOARD), "Main Board LM75_4A", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_MAINBOARD), "Main Board LM75_4D", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_5_ON_MAINBOARD), "Main Board LM75_4E", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_6_ON_MAINBOARD), "Main Board LM75_4F", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_FANCPLD), "Fan CPLD SENSOR_4D", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_FANCPLD), "Fan CPLD SENSOR_4E", 0, {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_THERMAL_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU1), "PSU-1 Thermal Sensor 1", ONLP_PSU_ID_CREATE(PSU1_ID), {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_PSU_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_PSU1), "PSU-1 Thermal Sensor 2", ONLP_PSU_ID_CREATE(PSU1_ID), {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_PSU_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_PSU1), "PSU-1 Thermal Sensor 3", ONLP_PSU_ID_CREATE(PSU1_ID), {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_PSU_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU2), "PSU-2 Thermal Sensor 1", ONLP_PSU_ID_CREATE(PSU2_ID), {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_PSU_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_PSU2), "PSU-2 Thermal Sensor 2", ONLP_PSU_ID_CREATE(PSU2_ID), {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_PSU_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_PSU2), "PSU-2 Thermal Sensor 3", ONLP_PSU_ID_CREATE(PSU2_ID), {0} },
            ONLP_THERMAL_STATUS_PRESENT,
            AS7946_30XB_PSU_CAPS, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
        },
};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int tid;
    VALIDATE(id);

    tid = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[tid];

    if(tid == THERMAL_CPU_CORE) {
        int rv = onlp_file_read_int_max(&info->mcelsius, cpu_coretemp_files);
        return rv;
    }

    return onlp_file_read_int(&info->mcelsius, ipmi_devfiles__[tid]);
}
