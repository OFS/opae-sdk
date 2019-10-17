// Copyright(c) 2019, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <glob.h>
#include <poll.h>
#include <json-c/json.h>

#include "safe_string/safe_string.h"
#include "fpgad/api/opae_events_api.h"
#include "fpgad/api/device_monitoring.h"
#include "fpgad/api/sysfs.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("fpgad-vc: " format, ##__VA_ARGS__)

#define SYSFS_PATH_MAX 256
#define FME_ERR_NAME "errors"
#define SEU_ERR_NAME "seu_emr"
#define CATFATAL_ERR_NAME "catfatal_errors"

#define FPGA_SEU_ERR_BIT 20
#define BMC_SEU_ERR_BIT 12

#define TRIM_LOG_MODULUS 20
#define LOG_MOD(__r, __fmt, ...) \
do { \
\
	++(__r); \
	if (!((__r) % TRIM_LOG_MODULUS)) { \
		log_printf("fpgad-vc: " __fmt, ##__VA_ARGS__); \
	} \
} while (0)


typedef struct _vc_sensor {
	fpga_object sensor_object;
	char *name;
	char *type;
	uint64_t id;
	fpga_object value_object;
	uint64_t value;
	uint64_t high_fatal;
	uint64_t high_warn;
	uint64_t low_fatal;
	uint64_t low_warn;
	uint32_t flags;
#define FPGAD_SENSOR_VC_IGNORE           0x00000001
#define FPGAD_SENSOR_VC_HIGH_FATAL_VALID 0x00000002
#define FPGAD_SENSOR_VC_HIGH_WARN_VALID  0x00000004
#define FPGAD_SENSOR_VC_LOW_FATAL_VALID  0x00000008
#define FPGAD_SENSOR_VC_LOW_WARN_VALID   0x00000010
	uint32_t read_errors;
#define FPGAD_SENSOR_VC_MAX_READ_ERRORS  25
} vc_sensor;

typedef struct _vc_config_sensor {
	uint64_t id;
	uint64_t high_fatal;
	uint64_t high_warn;
	uint64_t low_fatal;
	uint64_t low_warn;
	uint32_t flags;
} vc_config_sensor;

#define MAX_SENSORS_ENUM_RETRIES	5
#define MAX_VC_SENSORS 128
typedef struct _vc_device {
	fpgad_monitored_device *base_device;
	fpga_object group_object;
	vc_sensor sensors[MAX_VC_SENSORS];
	uint32_t num_sensors;
	uint64_t max_sensor_id;
	uint8_t *state_tripped; // bit set
	uint8_t *state_last;    // bit set
	uint64_t tripped_count;
	uint32_t num_config_sensors;
	vc_config_sensor *config_sensors;
	bool aer_disabled;
	uint32_t previous_ecap_aer[2];
	fpga_handle fpga_h;
	fpga_event_handle event_h;
	bool poll_seu_event;
	int poll_timeout_msec;
	struct pollfd event_fd;
	bool fpga_seu_err;
	bool bmc_seu_err;
	char sbdf[16];
	uint16_t device_id;
#define VC_PF0_DEV_ID 0x0b30
#define VC_VF0_DEV_ID 0x0b31
} vc_device;

#define BIT_SET_MASK(__n)  (1 << ((__n) % 8))
#define BIT_SET_INDEX(__n) ((__n) / 8)

#define BIT_SET_SET(__s, __n) \
((__s)[BIT_SET_INDEX(__n)] |= BIT_SET_MASK(__n))

#define BIT_SET_CLR(__s, __n) \
((__s)[BIT_SET_INDEX(__n)] &= ~BIT_SET_MASK(__n))

#define BIT_IS_SET(__s, __n) \
((__s)[BIT_SET_INDEX(__n)] & BIT_SET_MASK(__n))


#define HIGH_FATAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_VC_HIGH_FATAL_VALID) && \
 ((__sens)->value > (__sens)->high_fatal))

#define HIGH_WARN(__sens) \
(((__sens)->flags & FPGAD_SENSOR_VC_HIGH_WARN_VALID) && \
 ((__sens)->value > (__sens)->high_warn))

#define HIGH_NORMAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_VC_HIGH_WARN_VALID) && \
 ((__sens)->value <= (__sens)->high_warn))

#define LOW_FATAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_VC_LOW_FATAL_VALID) && \
 ((__sens)->value < (__sens)->low_fatal))

#define LOW_WARN(__sens) \
(((__sens)->flags & FPGAD_SENSOR_VC_LOW_WARN_VALID) && \
 ((__sens)->value < (__sens)->low_warn))

#define LOW_NORMAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_VC_LOW_WARN_VALID) && \
 ((__sens)->value >= (__sens)->low_warn))

STATIC bool vc_threads_running = true;

STATIC void stop_vc_threads(void)
{
	vc_threads_running = false;
}

STATIC void vc_destroy_sensor(vc_sensor *sensor)
{
	fpgaDestroyObject(&sensor->sensor_object);
	if (sensor->name) {
		free(sensor->name);
		sensor->name = NULL;
	}
	if (sensor->type) {
		free(sensor->type);
		sensor->type = NULL;
	}
	if (sensor->value_object) {
		fpgaDestroyObject(&sensor->value_object);
		sensor->value_object = NULL;
	}
	sensor->flags = 0;
	sensor->read_errors = 0;
}

STATIC void vc_destroy_sensors(vc_device *vc)
{
	uint32_t i;

	for (i = 0 ; i < vc->num_sensors ; ++i) {
		vc_destroy_sensor(&vc->sensors[i]);
	}
	vc->num_sensors = 0;
	vc->max_sensor_id = 0;

	if (vc->group_object) {
		fpgaDestroyObject(&vc->group_object);
		vc->group_object = NULL;
	}

	if (vc->state_tripped) {
		free(vc->state_tripped);
		vc->state_tripped = NULL;
	}

	if (vc->state_last) {
		free(vc->state_last);
		vc->state_last = NULL;
	}
}

STATIC void vc_destroy_device(vc_device *vc)
{
	vc_destroy_sensors(vc);
	if (vc->config_sensors) {
		free(vc->config_sensors);
		vc->config_sensors = NULL;
		vc->num_config_sensors = 0;
	}
}

STATIC fpga_result vc_sensor_get_string(vc_sensor *sensor,
					const char *obj_name,
					char **name)
{
	fpga_result res;
	fpga_object obj = NULL;
	char buf[SYSFS_PATH_MAX] = { 0, };
	uint32_t len = 0;

	res = fpgaObjectGetObject(sensor->sensor_object,
				  obj_name,
				  &obj,
				  0);
	if (res != FPGA_OK)
		return res;

	res = fpgaObjectGetSize(obj, &len, 0);
	if (res != FPGA_OK)
		goto out_free_obj;

	res = fpgaObjectRead(obj, (uint8_t *)buf, 0, len, 0);
	if (res != FPGA_OK)
		goto out_free_obj;

	if (buf[len-1] == '\n')
		buf[len-1] = '\0';

	*name = cstr_dup((const char *)buf);
	if (!(*name))
		res = FPGA_NO_MEMORY;

out_free_obj:
	fpgaDestroyObject(&obj);
	return res;
}

STATIC fpga_result vc_sensor_get_u64(vc_sensor *sensor,
				     const char *obj_name,
				     uint64_t *value)
{
	fpga_result res;
	fpga_object obj = NULL;

	res = fpgaObjectGetObject(sensor->sensor_object,
				  obj_name,
				  &obj,
				  0);
	if (res != FPGA_OK)
		return res;

	res = fpgaObjectRead64(obj, value, 0);

	fpgaDestroyObject(&obj);

	return res;
}

// The percentage by which we adjust the power trip
// points so that we catch anomolies before the hw does.
#define VC_PERCENT_ADJUST_PWR 5
// The number of degrees by which we adjust the
// temperature trip points so that we catch anomolies
// before the hw does.
#define VC_DEGREES_ADJUST_TEMP 5
STATIC fpga_result vc_sensor_get(vc_device *vc, vc_sensor *s)
{
	fpga_result res;
	bool is_temperature;
	int indicator = -1;
	vc_config_sensor *cfg_sensor = NULL;
	uint32_t i;

	if (s->name) {
		free(s->name);
		s->name = NULL;
	}

	if (s->type) {
		free(s->type);
		s->type = NULL;
	}

	res = vc_sensor_get_string(s, "name", &s->name);
	if (res != FPGA_OK)
		return res;

	res = vc_sensor_get_string(s, "type", &s->type);
	if (res != FPGA_OK)
		return res;

	res = vc_sensor_get_u64(s, "id", &s->id);
	if (res != FPGA_OK)
		return res;

	res = fpgaObjectRead64(s->value_object, &s->value, 0);
	if (res != FPGA_OK)
		return res;

	strcmp_s(s->type, 11, "Temperature", &indicator);
	is_temperature = (indicator == 0);

	res = vc_sensor_get_u64(s, "high_fatal", &s->high_fatal);
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_VC_HIGH_FATAL_VALID;
		if (is_temperature)
			s->high_fatal -= VC_DEGREES_ADJUST_TEMP;
		else
			s->high_fatal -=
				(s->high_fatal * VC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_VC_HIGH_FATAL_VALID;

	res = vc_sensor_get_u64(s, "high_warn", &s->high_warn);
	if (res == FPGA_OK)
		s->flags |= FPGAD_SENSOR_VC_HIGH_WARN_VALID;
	else
		s->flags &= ~FPGAD_SENSOR_VC_HIGH_WARN_VALID;

	res = vc_sensor_get_u64(s, "low_fatal", &s->low_fatal);
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_VC_LOW_FATAL_VALID;
		if (is_temperature)
			s->low_fatal += VC_DEGREES_ADJUST_TEMP;
		else
			s->low_fatal +=
				(s->low_fatal * VC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_VC_LOW_FATAL_VALID;

	res = vc_sensor_get_u64(s, "low_warn", &s->low_warn);
	if (res == FPGA_OK)
		s->flags |= FPGAD_SENSOR_VC_LOW_WARN_VALID;
	else
		s->flags &= ~FPGAD_SENSOR_VC_LOW_WARN_VALID;

	/* Do we have a user override (via the config) for
	 * this sensor? If so, then honor it.
	 */
	for (i = 0 ; i < vc->num_config_sensors ; ++i) {
		if (vc->config_sensors[i].flags &
		    FPGAD_SENSOR_VC_IGNORE)
			continue;
		if (vc->config_sensors[i].id == s->id) {
			cfg_sensor = &vc->config_sensors[i];
			break;
		}
	}

	if (cfg_sensor) {

		if (cfg_sensor->flags & FPGAD_SENSOR_VC_HIGH_FATAL_VALID) {
			// Cap the sensor at the adjusted max
			// allowed by the hardware.
			if ((s->flags & FPGAD_SENSOR_VC_HIGH_FATAL_VALID) &&
			    (cfg_sensor->high_fatal > s->high_fatal))
				/* nothing */ ;
			else
				s->high_fatal = cfg_sensor->high_fatal;

			s->flags |= FPGAD_SENSOR_VC_HIGH_FATAL_VALID;
		} else
			s->flags &= ~FPGAD_SENSOR_VC_HIGH_FATAL_VALID;

		if (cfg_sensor->flags & FPGAD_SENSOR_VC_HIGH_WARN_VALID) {

			if ((s->flags & FPGAD_SENSOR_VC_HIGH_WARN_VALID) &&
			    (cfg_sensor->high_warn > s->high_warn))
				/* nothing */ ;
			else
				s->high_warn = cfg_sensor->high_warn;

			s->flags |= FPGAD_SENSOR_VC_HIGH_WARN_VALID;
		} else
			s->flags &= ~FPGAD_SENSOR_VC_HIGH_WARN_VALID;

		if (cfg_sensor->flags & FPGAD_SENSOR_VC_LOW_FATAL_VALID) {

			if ((s->flags & FPGAD_SENSOR_VC_LOW_FATAL_VALID) &&
			    (cfg_sensor->low_fatal < s->low_fatal))
				/* nothing */ ;
			else
				s->low_fatal = cfg_sensor->low_fatal;

			s->flags |= FPGAD_SENSOR_VC_LOW_FATAL_VALID;
		} else
			s->flags &= ~FPGAD_SENSOR_VC_LOW_FATAL_VALID;

		if (cfg_sensor->flags & FPGAD_SENSOR_VC_LOW_WARN_VALID) {

			if ((s->flags & FPGAD_SENSOR_VC_LOW_WARN_VALID) &&
			    (cfg_sensor->low_warn < s->low_warn))
				/* nothing */ ;
			else
				s->low_warn = cfg_sensor->low_warn;

			s->flags |= FPGAD_SENSOR_VC_LOW_WARN_VALID;
		} else
			s->flags &= ~FPGAD_SENSOR_VC_LOW_WARN_VALID;

	}

	return FPGA_OK;
}

STATIC fpga_result vc_enum_sensor(vc_device *vc,
				  const char *name)
{
	fpga_result res;
	vc_sensor *s;

	if (vc->num_sensors == MAX_VC_SENSORS) {
		LOG("exceeded max number of sensors.\n");
		return FPGA_EXCEPTION;
	}

	s = &vc->sensors[vc->num_sensors];

	res = fpgaObjectGetObject(vc->group_object,
				  name,
				  &s->sensor_object,
				  0);

	if (res != FPGA_OK)
		return res;

	res = fpgaObjectGetObject(s->sensor_object,
				  "value",
				  &s->value_object,
				  0);

	if (res != FPGA_OK) {
		LOG("failed to get value object for %s.\n", name);
		fpgaDestroyObject(&s->sensor_object);
		return res;
	}

	res = vc_sensor_get(vc, s);
	if (res == FPGA_OK)
		++vc->num_sensors;
	else {
		LOG("warning: sensor attribute enumeration failed.\n");
		vc_destroy_sensor(s);
	}

	return res;
}

STATIC fpga_result vc_enum_sensors(vc_device *vc)
{
	fpga_result res;
	char name[SYSFS_PATH_MAX];
	int i;

	res = fpgaTokenGetObject(vc->base_device->token,
				 "spi-altera.*.auto/spi_master/spi*/spi*.*",
				 &vc->group_object,
				 FPGA_OBJECT_GLOB);
	if (res)
		return res;

	for (i = 0 ; i < MAX_VC_SENSORS ; ++i) {
		snprintf_s_i(name, sizeof(name), "sensor%d", i);
		vc_enum_sensor(vc, name);
	}

	if (vc->num_sensors > 0) {
		vc_sensor *s = &vc->sensors[vc->num_sensors - 1];

		vc->max_sensor_id = s->id;

		vc->state_tripped = calloc((vc->max_sensor_id + 7) / 8, 1);
		vc->state_last = calloc((vc->max_sensor_id + 7) / 8, 1);

		return (vc->state_tripped && vc->state_last) ?
			FPGA_OK : FPGA_NO_MEMORY;
	}

	return FPGA_NOT_FOUND;
}

STATIC fpga_result vc_disable_aer(vc_device *vc)
{
	fpga_token token;
	fpga_result res;
	fpga_properties prop = NULL;
	char path[PATH_MAX];
	char rlpath[PATH_MAX];
	char *p;
	errno_t err;
	char cmd[256];
	char output[256];
	FILE *fp;
	size_t sz;

	uint16_t seg = 0;
	uint8_t bus = 0;
	uint8_t dev = 0;
	uint8_t fn = 0;

	token = vc->base_device->token;

	res = fpgaGetProperties(token, &prop);
	if (res != FPGA_OK) {
		LOG("failed to get fpga properties.\n");
		return res;
	}

	if ((fpgaPropertiesGetSegment(prop, &seg) != FPGA_OK) ||
	    (fpgaPropertiesGetBus(prop, &bus) != FPGA_OK) ||
	    (fpgaPropertiesGetDevice(prop, &dev) != FPGA_OK) ||
	    (fpgaPropertiesGetFunction(prop, &fn) != FPGA_OK)) {
		LOG("failed to get PCI attributes.\n");
		fpgaDestroyProperties(&prop);
		return FPGA_EXCEPTION;
	}

	fpgaDestroyProperties(&prop);

	snprintf_s_iiii(path, PATH_MAX,
			"/sys/bus/pci/devices/%04x:%02x:%02x.%d",
			(int)seg, (int)bus, (int)dev, (int)fn);

	memset_s(rlpath, sizeof(rlpath), 0);

	if (readlink(path, rlpath, sizeof(rlpath)) < 0) {
		LOG("readlink \"%s\" failed.\n", path);
		return FPGA_EXCEPTION;
	}

	// (rlpath)
	//                    1111111111
	//          01234567890123456789
	// ../../../devices/pci0000:ae/0000:ae:00.0/0000:af:00.0/
	// 0000:b0:09.0/0000:b2:00.0

	err = strstr_s(rlpath, sizeof(rlpath),
		       "devices/pci", 11, &p);
	if (err != EOK) {
		LOG("error: no \"devices/pci\" in path \"%s\"\n", rlpath);
		return FPGA_EXCEPTION;
	}

	p += 19;
	*(p + 12) = '\0';

	// Save the current ECAP_AER values.

	snprintf_s_s(cmd, sizeof(cmd),
		      "setpci -s %s ECAP_AER+0x08.L", p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("failed to read ECAP_AER+0x08 for %s\n", p);
		return FPGA_EXCEPTION;
	}

	sz = fread(output, 1, sizeof(output), fp);

	if (sz >= sizeof(output))
		sz = sizeof(output) - 1;
	output[sz] = '\0';

	pclose(fp);

	vc->previous_ecap_aer[0] = strtoul(output, NULL, 16);

	LOG("saving previous ECAP_AER+0x08 value 0x%08x for %s\n",
	    vc->previous_ecap_aer[0], p);


	snprintf_s_s(cmd, sizeof(cmd),
		      "setpci -s %s ECAP_AER+0x14.L", p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("failed to read ECAP_AER+0x14 for %s\n", p);
		return FPGA_EXCEPTION;
	}

	sz = fread(output, 1, sizeof(output), fp);

	if (sz >= sizeof(output))
		sz = sizeof(output) - 1;
	output[sz] = '\0';

	pclose(fp);

	vc->previous_ecap_aer[1] = strtoul(output, NULL, 16);

	LOG("saving previous ECAP_AER+0x14 value 0x%08x for %s\n",
	    vc->previous_ecap_aer[1], p);


	// Disable AER.

	snprintf_s_s(cmd, sizeof(cmd),
		      "setpci -s %s ECAP_AER+0x08.L=0xffffffff", p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("failed to write ECAP_AER+0x08 for %s\n", p);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	snprintf_s_s(cmd, sizeof(cmd),
		      "setpci -s %s ECAP_AER+0x14.L=0xffffffff", p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("failed to write ECAP_AER+0x14 for %s\n", p);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	return FPGA_OK;
}

STATIC fpga_result vc_enable_aer(vc_device *vc)
{
	fpga_token token;
	fpga_result res;
	fpga_properties prop = NULL;
	char path[PATH_MAX];
	char rlpath[PATH_MAX];
	char *p;
	errno_t err;
	char cmd[256];
	FILE *fp;

	uint16_t seg = 0;
	uint8_t bus = 0;
	uint8_t dev = 0;
	uint8_t fn = 0;

	token = vc->base_device->token;

	res = fpgaGetProperties(token, &prop);
	if (res != FPGA_OK) {
		LOG("failed to get fpga properties.\n");
		return res;
	}

	if ((fpgaPropertiesGetSegment(prop, &seg) != FPGA_OK) ||
	    (fpgaPropertiesGetBus(prop, &bus) != FPGA_OK) ||
	    (fpgaPropertiesGetDevice(prop, &dev) != FPGA_OK) ||
	    (fpgaPropertiesGetFunction(prop, &fn) != FPGA_OK)) {
		LOG("failed to get PCI attributes.\n");
		fpgaDestroyProperties(&prop);
		return FPGA_EXCEPTION;
	}

	fpgaDestroyProperties(&prop);

	snprintf_s_iiii(path, PATH_MAX,
			"/sys/bus/pci/devices/%04x:%02x:%02x.%d",
			(int)seg, (int)bus, (int)dev, (int)fn);

	memset_s(rlpath, sizeof(rlpath), 0);

	if (readlink(path, rlpath, sizeof(rlpath)) < 0) {
		LOG("readlink \"%s\" failed.\n", path);
		return FPGA_EXCEPTION;
	}

	// (rlpath)
	//                    1111111111
	//          01234567890123456789
	// ../../../devices/pci0000:ae/0000:ae:00.0/0000:af:00.0/
	// 0000:b0:09.0/0000:b2:00.0

	err = strstr_s(rlpath, sizeof(rlpath),
		       "devices/pci", 11, &p);
	if (err != EOK) {
		LOG("error: no \"devices/pci\" in path \"%s\"\n", rlpath);
		return FPGA_EXCEPTION;
	}

	p += 19;
	*(p + 12) = '\0';

	// Write the saved ECAP_AER values to enable AER.

	snprintf_s_si(cmd, sizeof(cmd),
		      "setpci -s %s ECAP_AER+0x08.L=0x%08x",
		      p, vc->previous_ecap_aer[0]);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("failed to write ECAP_AER+0x08 for %s\n", p);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	LOG("restored previous ECAP_AER+0x08 value 0x%08x for %s\n",
	    vc->previous_ecap_aer[0], p);


	snprintf_s_si(cmd, sizeof(cmd),
		      "setpci -s %s ECAP_AER+0x14.L=0x%08x",
		      p, vc->previous_ecap_aer[1]);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("failed to write ECAP_AER+0x14 for %s\n", p);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	LOG("restored previous ECAP_AER+0x14 value 0x%08x for %s\n",
	    vc->previous_ecap_aer[1], p);

	return FPGA_OK;
}

STATIC bool vc_monitor_sensors(vc_device *vc)
{
	uint32_t i;
	uint32_t monitoring = 0;
	bool negative_trans = false;
	bool res = true;

	if (vc->num_sensors == 0) {		// no sensor found
		return true;
	}

	for (i = 0 ; i < vc->num_sensors ; ++i) {
		vc_sensor *s = &vc->sensors[i];

		if (s->flags & FPGAD_SENSOR_VC_IGNORE)
			continue;

		if (s->flags & (FPGAD_SENSOR_VC_HIGH_WARN_VALID|
				FPGAD_SENSOR_VC_LOW_WARN_VALID))
			++monitoring;

		if (fpgaObjectRead64(s->value_object,
				     &s->value,
				     FPGA_OBJECT_SYNC) != FPGA_OK) {
			if (++s->read_errors >=
				FPGAD_SENSOR_VC_MAX_READ_ERRORS)
				s->flags |= FPGAD_SENSOR_VC_IGNORE;
			continue;
		}

		if (HIGH_WARN(s) || LOW_WARN(s)) {
			opae_api_send_EVENT_POWER_THERMAL(vc->base_device);
			BIT_SET_SET(vc->state_tripped, s->id);
			if (!BIT_IS_SET(vc->state_last, s->id)) {
				LOG("sensor '%s' warning.\n", s->name);
				if (!vc->aer_disabled) {
					if (FPGA_OK == vc_disable_aer(vc))
						vc->aer_disabled = true;
				}
			}
		}

		if (HIGH_NORMAL(s) || LOW_NORMAL(s)) {
			if (BIT_IS_SET(vc->state_last, s->id)) {
				negative_trans = true;
				LOG("sensor '%s' back to normal.\n", s->name);
			}
		}

		if (BIT_IS_SET(vc->state_last, s->id) &&
		    BIT_IS_SET(vc->state_tripped, s->id)) {
			LOG_MOD(vc->tripped_count,
				"sensor '%s' still tripped.\n", s->name);
		}
	}

	if (negative_trans) {
		for (i = 0 ; i < vc->num_sensors ; ++i) {
			if (BIT_IS_SET(vc->state_tripped, vc->sensors[i].id))
				break;
		}

		if (i == vc->num_sensors) {
			// no remaining tripped sensors
			vc->tripped_count = 0;
			if (vc->aer_disabled) {
				if (FPGA_OK == vc_enable_aer(vc))
					vc->aer_disabled = false;
			}
		}
	}

	/*
	** Are we still monitoring any sensor that has a valid
	** high/low warn threshold? If not, then this fn should
	** return false so that we wait for sensor enumeration
	** in the caller. It's possible that we're ignoring all
	** of the sensors because they became unavailable due to
	** driver removal, eg.
	*/
	if (!monitoring)
		res = false;

	for (i = 0 ; i < vc->num_sensors ; ++i) {
		vc_sensor *s = &vc->sensors[i];
		if (BIT_IS_SET(vc->state_tripped, s->id))
			BIT_SET_SET(vc->state_last, s->id);
		else
			BIT_SET_CLR(vc->state_last, s->id);
	}

	memset_s(vc->state_tripped, (vc->max_sensor_id + 7) / 8, 0);

	return res;
}

STATIC void vc_handle_err_event(vc_device *vc)
{
	fpgad_monitored_device *d = vc->base_device;
	fpga_properties props = NULL;
	uint32_t num_errors;
	struct fpga_error_info errinfo;
	uint16_t seg = 0;
	uint8_t bus = 0;
	uint8_t dev = 0;
	uint8_t fn = 0;
	int i;

	if (fpgaGetProperties(d->token, &props) != FPGA_OK) {
		LOG("failed to get FPGA properties.\n");
		return;
	}

	if (fpgaPropertiesGetDeviceID(props, &vc->device_id) != FPGA_OK) {
		LOG("failed to get device ID.\n");
		fpgaDestroyProperties(&props);
		return;
	}

	if ((fpgaPropertiesGetSegment(props, &seg) != FPGA_OK) ||
	    (fpgaPropertiesGetBus(props, &bus) != FPGA_OK) ||
	    (fpgaPropertiesGetDevice(props, &dev) != FPGA_OK) ||
	    (fpgaPropertiesGetFunction(props, &fn) != FPGA_OK)) {
		LOG("failed to get PCI attributes.\n");
		fpgaDestroyProperties(&props);
		return;
	}

	snprintf_s_iiii(vc->sbdf, 16, "%04x:%02x:%02x.%d",
					(int)seg, (int)bus, (int)dev, (int)fn);

	fpgaPropertiesGetNumErrors(props, &num_errors);
	for (i = 0; i < (int)num_errors; i++) {
		uint64_t error_value = 0;
		fpgaGetErrorInfo(d->token, i, &errinfo);
		fpgaReadError(d->token, i, &error_value);
		if (error_value != 0) {
			LOG("detect %s 0x%zx @ %s\n", errinfo.name, error_value, vc->sbdf);
		}
		if (!strcmp(errinfo.name, FME_ERR_NAME)) {
			if (error_value & (1 << FPGA_SEU_ERR_BIT)) {
				vc->fpga_seu_err = true;
				LOG("SEU error occurred on fpga @ %s\n", vc->sbdf);
			}
		}
		if (!strcmp(errinfo.name, CATFATAL_ERR_NAME)) {
			if (error_value & (1 << BMC_SEU_ERR_BIT)) {
				vc->bmc_seu_err = true;
				LOG("SEU error occurred on bmc @ %s\n", vc->sbdf);
			}
		}
	}
	fpgaClearAllErrors(d->token);

	fpgaDestroyProperties(&props);

	if (vc->fpga_seu_err || vc->bmc_seu_err) {
		opae_api_send_EVENT_ERROR(d);
	}
}

STATIC void vc_register_err_event(vc_device *vc)
{
	fpgad_monitored_device *d = vc->base_device;
	fpga_result ret = FPGA_OK;

	vc->poll_seu_event = false;

	ret = fpgaOpen(d->token, &vc->fpga_h, FPGA_OPEN_SHARED);
	if (ret != FPGA_OK) {
		LOG("failed to get FPGA handle from token.\n");
		return;
	}

	ret = fpgaCreateEventHandle(&vc->event_h);
	if (ret != FPGA_OK) {
		LOG("failed to create event handle.\n");
		goto out_close;
	}

	/* FME error interrupt vector is 6 */
	ret = fpgaRegisterEvent(vc->fpga_h, FPGA_EVENT_ERROR, vc->event_h, 6);
	if (ret != FPGA_OK) {
		LOG("failed to register FPGA error event handle.\n");
		goto out_destroy;
	}

	memset_s(&vc->event_fd, sizeof(struct pollfd), 0);
	ret = fpgaGetOSObjectFromEventHandle(vc->event_h, &vc->event_fd.fd);
	if (ret == FPGA_OK) {
		vc->event_fd.events = POLLIN;
		vc->poll_seu_event = true;
		vc->fpga_seu_err = false;
		vc->bmc_seu_err = false;
		vc->poll_timeout_msec = d->config->poll_interval_usec / 1000;
	} else {
		LOG("failed to get event fd from event handle.\n");
		goto out_unregister;
	}

	return;

out_unregister:
	fpgaUnregisterEvent(vc->fpga_h, FPGA_EVENT_ERROR, vc->event_h);
out_destroy:
	fpgaDestroyEventHandle(&vc->event_h);
out_close:
	fpgaClose(vc->fpga_h);
}

STATIC void vc_unregister_err_event(vc_device *vc)
{
	fpgaUnregisterEvent(vc->fpga_h, FPGA_EVENT_ERROR, vc->event_h);
	fpgaDestroyEventHandle(&vc->event_h);
	fpgaClose(vc->fpga_h);
}

STATIC int cool_down = 30;

STATIC void *monitor_fme_vc_thread(void *arg)
{
	fpgad_monitored_device *d =
		(fpgad_monitored_device *)arg;

	vc_device *vc = (vc_device *)d->thread_context;

	uint32_t enum_retries = 0;
	uint8_t *save_state_last = NULL;

	while (vc_threads_running) {
		vc_register_err_event(vc);
		vc_handle_err_event(vc);  // handle error occurred before running fpgad
		while (vc_enum_sensors(vc) != FPGA_OK) {
			LOG_MOD(enum_retries, "waiting to enumerate sensors.\n");
			if (enum_retries > MAX_SENSORS_ENUM_RETRIES) {
				LOG("no sensors are found.\n");
				enum_retries = 0;
				break;
			}
			if (!vc_threads_running)
				return NULL;
			sleep(1);
		}

		if (save_state_last) {
			free(vc->state_last);
			vc->state_last = save_state_last;
			save_state_last = NULL;
		}

		while (vc_monitor_sensors(vc)) {
			if (vc->poll_seu_event) {
				int poll_ret = poll(&vc->event_fd, 1, vc->poll_timeout_msec);
				if (poll_ret > 0) {
					LOG("error interrupt event received.\n");
					uint64_t count = 0;
					ssize_t bytes_read = read(vc->event_fd.fd, &count,
											  sizeof(count));
					if (bytes_read > 0) {
						LOG("poll count = %zu.\n", count);
					}
					vc_handle_err_event(vc);
				} else if (poll_ret < 0) {
					LOG("poll error, errno = %s.\n", strerror(errno));
				}
			} else {
				usleep(d->config->poll_interval_usec);
			}

			if (!vc_threads_running) {
				vc_destroy_sensors(vc);
				vc_unregister_err_event(vc);
				return NULL;
			}
		}

		save_state_last = vc->state_last;
		vc->state_last = NULL;

		vc_destroy_sensors(vc);
		vc_unregister_err_event(vc);
	}

	return NULL;
}

STATIC int vc_parse_config(vc_device *vc, const char *cfg)
{
	json_object *root;
	enum json_tokener_error j_err = json_tokener_success;
	json_object *j_cool_down = NULL;
	json_object *j_config_sensors_enabled = NULL;
	json_object *j_sensors = NULL;
	int res = 1;
	int sensor_entries;
	int i;

	root = json_tokener_parse_verbose(cfg, &j_err);
	if (!root) {
		LOG("error parsing config: %s\n",
		    json_tokener_error_desc(j_err));
		return 1;
	}

	if (!json_object_object_get_ex(root,
				       "cool-down",
				       &j_cool_down)) {
		LOG("failed to find cool-down key in config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_cool_down, json_type_int)) {
		LOG("cool-down key not integer.\n");
		goto out_put;
	}

	cool_down = json_object_get_int(j_cool_down);
	if (cool_down < 0)
		cool_down = 30;

	LOG("set cool-down period to %d seconds.\n", cool_down);

	res = 0;

	if (!json_object_object_get_ex(root,
				       "config-sensors-enabled",
				       &j_config_sensors_enabled)) {
		LOG("failed to find config-sensors-enabled key in config.\n"
		    "Skipping user sensor config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_config_sensors_enabled, json_type_boolean)) {
		LOG("config-sensors-enabled key not Boolean.\n"
		    "Skipping user sensor config.\n");
		goto out_put;
	}

	if (!json_object_get_boolean(j_config_sensors_enabled)) {
		LOG("config-sensors-enabled key set to false.\n"
		    "Skipping user sensor config.\n");
		goto out_put;
	}

	if (!json_object_object_get_ex(root,
				       "sensors",
				       &j_sensors)) {
		LOG("failed to find sensors key in config.\n"
		    "Skipping user sensor config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_sensors, json_type_array)) {
		LOG("sensors key not array.\n"
		    "Skipping user sensor config.\n");
		goto out_put;
	}

	sensor_entries = json_object_array_length(j_sensors);
	if (!sensor_entries) {
		LOG("sensors key is empty array.\n"
		    "Skipping user sensor config.\n");
		goto out_put;
	}

	vc->config_sensors = calloc(sensor_entries, sizeof(vc_config_sensor));
	if (!vc->config_sensors) {
		LOG("calloc failed. Skipping user sensor config.\n");
		goto out_put;
	}

	vc->num_config_sensors = (uint32_t)sensor_entries;

	for (i = 0 ; i < sensor_entries ; ++i) {
		json_object *j_sensor_sub_i = json_object_array_get_idx(j_sensors, i);
		json_object *j_id;
		json_object *j_high_fatal;
		json_object *j_high_warn;
		json_object *j_low_fatal;
		json_object *j_low_warn;

		if (!json_object_object_get_ex(j_sensor_sub_i,
					       "id",
					       &j_id)) {
			LOG("failed to find id key in config sensors[%d].\n"
			    "Skipping entry %d.\n", i, i);
			vc->config_sensors[i].id = MAX_VC_SENSORS;
			vc->config_sensors[i].flags = FPGAD_SENSOR_VC_IGNORE;
			continue;
		}

		if (!json_object_is_type(j_id, json_type_int)) {
			LOG("sensors[%d].id key not int.\n"
			    "Skipping entry %d.\n", i, i);
			vc->config_sensors[i].id = MAX_VC_SENSORS;
			vc->config_sensors[i].flags = FPGAD_SENSOR_VC_IGNORE;
			continue;
		}

		vc->config_sensors[i].id = json_object_get_int(j_id);

		if (json_object_object_get_ex(j_sensor_sub_i,
					      "high-fatal",
					      &j_high_fatal)) {
			if (json_object_is_type(j_high_fatal,
						json_type_double)) {
				vc->config_sensors[i].high_fatal =
				(uint64_t)(json_object_get_double(j_high_fatal)
					* 1000.0);
				vc->config_sensors[i].flags |=
					FPGAD_SENSOR_VC_HIGH_FATAL_VALID;
				LOG("user sensor%u high-fatal: %f\n",
				    vc->config_sensors[i].id,
				    json_object_get_double(j_high_fatal));
			}
		}

		if (json_object_object_get_ex(j_sensor_sub_i,
					      "high-warn",
					      &j_high_warn)) {
			if (json_object_is_type(j_high_warn,
						json_type_double)) {
				vc->config_sensors[i].high_warn =
				(uint64_t)(json_object_get_double(j_high_warn)
					* 1000.0);
				vc->config_sensors[i].flags |=
					FPGAD_SENSOR_VC_HIGH_WARN_VALID;
				LOG("user sensor%u high-warn: %f\n",
				    vc->config_sensors[i].id,
				    json_object_get_double(j_high_warn));
			}
		}

		if (json_object_object_get_ex(j_sensor_sub_i,
					      "low-fatal",
					      &j_low_fatal)) {
			if (json_object_is_type(j_low_fatal,
						json_type_double)) {
				vc->config_sensors[i].low_fatal =
				(uint64_t)(json_object_get_double(j_low_fatal)
					* 1000.0);
				vc->config_sensors[i].flags |=
					FPGAD_SENSOR_VC_LOW_FATAL_VALID;
				LOG("user sensor%u low-fatal: %f\n",
				    vc->config_sensors[i].id,
				    json_object_get_double(j_low_fatal));
			}
		}

		if (json_object_object_get_ex(j_sensor_sub_i,
					      "low-warn",
					      &j_low_warn)) {
			if (json_object_is_type(j_low_warn,
						json_type_double)) {
				vc->config_sensors[i].low_warn =
				(uint64_t)(json_object_get_double(j_low_warn)
					* 1000.0);
				vc->config_sensors[i].flags |=
					FPGAD_SENSOR_VC_LOW_WARN_VALID;
				LOG("user sensor%u low-warn: %f\n",
				    vc->config_sensors[i].id,
				    json_object_get_double(j_low_warn));
			}
		}
	}

out_put:
	json_object_put(root);
	return res;
}

int fpgad_plugin_configure(fpgad_monitored_device *d,
			   const char *cfg)
{
	int res = 1;
	vc_device *vc;

	vc_threads_running = true;

	d->type = FPGAD_PLUGIN_TYPE_THREAD;

	if (d->object_type == FPGA_DEVICE) {

		d->thread_fn = monitor_fme_vc_thread;
		d->thread_stop_fn = stop_vc_threads;

		vc = calloc(1, sizeof(vc_device));
		if (!vc)
			return res;

		vc->base_device = d;
		d->thread_context = vc;

		LOG("monitoring vid=0x%04x did=0x%04x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");

		res = vc_parse_config(vc, cfg);

	}

	// Not currently monitoring the Port device

	return res;
}

void fpgad_plugin_destroy(fpgad_monitored_device *d)
{
	LOG("stop monitoring vid=0x%04x did=0x%04x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");

	if (d->thread_context) {
		vc_destroy_device((vc_device *)d->thread_context);
		free(d->thread_context);
		d->thread_context = NULL;
	}
}
