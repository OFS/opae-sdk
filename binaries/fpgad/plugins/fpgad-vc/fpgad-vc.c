// Copyright(c) 2019-2021, Intel Corporation
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

#include "fpgad/api/opae_events_api.h"
#include "fpgad/api/device_monitoring.h"
#include "fpgad/api/sysfs.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("fpgad-vc: " format, ##__VA_ARGS__)

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
	uint64_t id;
	fpga_object value_object;
	char *name;
	char *type;
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

#define MAX_SENSOR_NAME 32
typedef struct _vc_config_sensor {
	char name[MAX_SENSOR_NAME];
	uint64_t high_fatal;
	uint64_t high_warn;
	uint64_t low_fatal;
	uint64_t low_warn;
	uint32_t flags;
} vc_config_sensor;

#define MAX_SENSORS_ENUM_RETRIES 5
#define MAX_VC_SENSORS 128
#define MAX_AER_CMD 64
typedef struct _vc_device {
	fpgad_monitored_device *base_device;
	vc_sensor sensors[MAX_VC_SENSORS];
	uint32_t num_sensors;
	uint8_t *state_tripped; // bit set
	uint8_t *state_last;    // bit set
	uint64_t tripped_count;
	uint32_t num_config_sensors;
	vc_config_sensor *config_sensors;
	char get_aer[2][MAX_AER_CMD];
	char disable_aer[2][MAX_AER_CMD];
	char set_aer[2][MAX_AER_CMD];
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

// The percentage by which we adjust the power trip
// points so that we catch anomolies before the hw does.
#define VC_PERCENT_ADJUST_PWR 5
// The number of degrees by which we adjust the
// temperature trip points so that we catch anomolies
// before the hw does.
#define VC_DEGREES_ADJUST_TEMP 5
STATIC fpga_result vc_sensor_get(vc_device *vc, char *label, vc_sensor *s)
{
	fpga_result res;
	char *p;
	int i;
	bool is_temp;
	char buf[SYSFS_PATH_MAX] = { 0, };
	char path[SYSFS_PATH_MAX] = { 0, };
	size_t len;
	char *endptr;
	vc_config_sensor *cfg_sensor = NULL;

	s->flags = 0;

	if (s->name) {
		free(s->name);
		s->name = NULL;
	}

	if (s->type) {
		free(s->type);
		s->type = NULL;
	}

	if (file_read_string(label, buf, sizeof(buf))) {
		return FPGA_EXCEPTION;
	}
	s->name = cstr_dup(buf);
	if (!s->name)
		return FPGA_NO_MEMORY;

	// Determine sensor type.
	p = strrchr(label, '/') + 1;
	if (!strncmp("curr", p, 4)) {
		s->type = cstr_dup("Current");
	} else if (!strncmp("in", p, 2)) {
		s->type = cstr_dup("Voltage");
	} else if (!strncmp("power", p, 5)) {
		s->type = cstr_dup("Power");
	} else if (!strncmp("temp", p, 4)) {
		s->type = cstr_dup("Temperature");
	} else {
		s->type = cstr_dup("Unknown");
	}

	if (!s->type)
		return FPGA_NO_MEMORY;

	is_temp = (strcmp(s->type, "Temperature") == 0);

	s->id = vc->num_sensors;

	// Extract the FME-relative path, given label
	// which is the full path through /sys/bus/pci/devices.
	p = label;
	for (i = 0 ; i < 8 ; ++i) {
		p = strchr(p + 1, '/');
	}
	++p; // don't include the first '/'

	len = strnlen(p, sizeof(buf) - 1);
	memcpy(buf, p, len);
	buf[len] = '\0';

	strncpy(&buf[len - 5], "input", 6);

	res = fpgaTokenGetObject(vc->base_device->token,
				 buf,
				 &s->value_object,
				 0);
	if (res != FPGA_OK) {
		LOG("failed to acquire sensor object for "
		    "\"%s\" at %s\n", s->name, buf);
		return res;
	}

	res = fpgaObjectRead64(s->value_object,
			       &s->value,
			       FPGA_OBJECT_SYNC);
	if (res != FPGA_OK)
		return res;

	len = strnlen(label, sizeof(path) - 1);
	memcpy(path, label, len);
	path[len] = '\0';
	p = &path[len - 5];

	strncpy(p, "crit", 5);
	s->flags &= ~FPGAD_SENSOR_VC_HIGH_FATAL_VALID;
	if (!file_read_string(path, buf, sizeof(buf))) {
		s->high_fatal = strtoul(buf, &endptr, 0);
		if (endptr == buf + strlen(buf)) {
			s->flags |= FPGAD_SENSOR_VC_HIGH_FATAL_VALID;
			if (is_temp)
				s->high_fatal -= VC_DEGREES_ADJUST_TEMP;
			else
				s->high_fatal -=
					(s->high_fatal * VC_PERCENT_ADJUST_PWR) / 100;
		}
	}

	strncpy(p, "max", 4);
	s->flags &= ~FPGAD_SENSOR_VC_HIGH_WARN_VALID;
	if (!file_read_string(path, buf, sizeof(buf))) {
		s->high_warn = strtoul(buf, &endptr, 0);
		if (endptr == buf + strlen(buf))
			s->flags |= FPGAD_SENSOR_VC_HIGH_WARN_VALID;
	}

	// FPGAD_SENSOR_VC_LOW_FATAL_VALID
	// FPGAD_SENSOR_VC_LOW_WARN_VALID
	// dfl driver doesn't expose low warn nor low fatal values.

	/* Do we have a user override (via the config) for
	 * this sensor? If so, then honor it.
	 */
	for (i = 0 ; i < (int)vc->num_config_sensors ; ++i) {
		if (vc->config_sensors[i].flags &
		    FPGAD_SENSOR_VC_IGNORE)
			continue;
		if (!strcmp(vc->config_sensors[i].name, s->name)) {
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
				  char *label)
{
	fpga_result res;
	vc_sensor *s;

	if (vc->num_sensors == MAX_VC_SENSORS) {
		LOG("exceeded max number of sensors.\n");
		return FPGA_EXCEPTION;
	}

	s = &vc->sensors[vc->num_sensors];

	res = vc_sensor_get(vc, label, s);
	if (res == FPGA_OK)
		++vc->num_sensors;
	else {
		LOG("warning: sensor attribute enumeration failed.\n");
		vc_destroy_sensor(s);
	}

	return res;
}

STATIC const char *glob_patterns[] = {
	"/sys/bus/pci/devices/%s/fpga_region/region*/dfl-fme.*/"
	"dfl_dev.*/subdev_spi_altera.*.auto/spi_master/spi*/spi*.*/"
	"*-hwmon.*.auto/hwmon/hwmon*/*_label",
	"/sys/bus/pci/devices/%s/fpga_region/region*/dfl-fme.*/"
	"dfl_dev.*/*-hwmon.*.auto/hwmon/hwmon*/*_label",
	"/sys/bus/pci/devices/%s/fpga_region/region*/dfl-fme.*/"
	"dfl_dev.*/spi*/spi*/spi*/*-hwmon.*.auto/hwmon/hwmon*/*_label",
	NULL
};

STATIC fpga_result vc_enum_sensors(vc_device *vc)
{
	size_t i;
	int ires;
	char glob_pattern[SYSFS_PATH_MAX];
	glob_t glob_data;

	for (i = 0 ; glob_patterns[i] ; ++i) {
		snprintf(glob_pattern, sizeof(glob_pattern),
			 glob_patterns[i], vc->sbdf);

		ires = glob(glob_pattern, 0, NULL, &glob_data);

		if (ires) {
			if (glob_data.gl_pathv)
				globfree(&glob_data);
			continue;
		}

		break;
	}

	if (!glob_patterns[i])
		return FPGA_NOT_FOUND;

	for (i = 0 ; i < glob_data.gl_pathc ; ++i) {
		vc_enum_sensor(vc, glob_data.gl_pathv[i]);
	}

	globfree(&glob_data);

	if (vc->num_sensors > 0) {
		vc->state_tripped = calloc((vc->num_sensors + 7) / 8, 1);
		vc->state_last = calloc((vc->num_sensors + 7) / 8, 1);

		return (vc->state_tripped && vc->state_last) ?
			FPGA_OK : FPGA_NO_MEMORY;
	}

	return FPGA_NOT_FOUND;
}

STATIC fpga_result vc_disable_aer(vc_device *vc)
{
	char path[PATH_MAX];
	char rlpath[PATH_MAX];
	char *p;
	char cmd[256];
	char output[256];
	FILE *fp;
	size_t sz;

	snprintf(path, sizeof(path),
		 "/sys/bus/pci/devices/%s",
		 vc->sbdf);

	memset(rlpath, 0, sizeof(rlpath));

	if (readlink(path, rlpath, sizeof(rlpath)) < 0) {
		LOG("readlink \"%s\" failed.\n", path);
		return FPGA_EXCEPTION;
	}

	// (rlpath)
	//                    1111111111
	//          01234567890123456789
	// ../../../devices/pci0000:ae/0000:ae:00.0/0000:af:00.0/
	// 0000:b0:09.0/0000:b2:00.0

	p = strstr(rlpath, "devices/pci");

	p += 19;
	*(p + 12) = '\0';

	// Save the current ECAP_AER values.

	snprintf(cmd, sizeof(cmd),
		 vc->get_aer[0], p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("popen(\"%s\") failed\n", cmd);
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


	snprintf(cmd, sizeof(cmd),
		 vc->get_aer[1], p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("popen(\"%s\") failed\n", cmd);
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

	snprintf(cmd, sizeof(cmd),
		 vc->disable_aer[0], p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("popen(\"%s\") failed\n", cmd);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	snprintf(cmd, sizeof(cmd),
		 vc->disable_aer[1], p);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("popen(\"%s\") failed\n", cmd);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	return FPGA_OK;
}

STATIC fpga_result vc_enable_aer(vc_device *vc)
{
	char path[PATH_MAX];
	char rlpath[PATH_MAX];
	char *p;
	char cmd[256];
	FILE *fp;

	snprintf(path, sizeof(path),
		 "/sys/bus/pci/devices/%s",
		 vc->sbdf);

	memset(rlpath, 0, sizeof(rlpath));

	if (readlink(path, rlpath, sizeof(rlpath)) < 0) {
		LOG("readlink \"%s\" failed.\n", path);
		return FPGA_EXCEPTION;
	}

	// (rlpath)
	//                    1111111111
	//          01234567890123456789
	// ../../../devices/pci0000:ae/0000:ae:00.0/0000:af:00.0/
	// 0000:b0:09.0/0000:b2:00.0

	p = strstr(rlpath, "devices/pci");

	p += 19;
	*(p + 12) = '\0';

	// Write the saved ECAP_AER values to enable AER.

	snprintf(cmd, sizeof(cmd),
		 vc->set_aer[0],
		 p, vc->previous_ecap_aer[0]);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("popen(\"%s\") failed\n", cmd);
		return FPGA_EXCEPTION;
	}

	pclose(fp);

	LOG("restored previous ECAP_AER+0x08 value 0x%08x for %s\n",
	    vc->previous_ecap_aer[0], p);


	snprintf(cmd, sizeof(cmd),
		 vc->set_aer[1],
		 p, vc->previous_ecap_aer[1]);

	fp = popen(cmd, "r");
	if (!fp) {
		LOG("popen(\"%s\") failed\n", cmd);
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

	if (vc->num_sensors == 0) { // no sensor found
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

	memset(vc->state_tripped, 0, (vc->num_sensors + 7) / 8);

	return res;
}

STATIC void vc_handle_err_event(vc_device *vc)
{
	fpgad_monitored_device *d = vc->base_device;
	fpga_properties props = NULL;
	uint32_t num_errors;
	struct fpga_error_info errinfo;
	int i;

	if (fpgaGetProperties(d->token, &props) != FPGA_OK) {
		LOG("failed to get FPGA properties.\n");
		return;
	}

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
		vc->fpga_seu_err = false;
		vc->bmc_seu_err = false;
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

	memset(&vc->event_fd, 0, sizeof(struct pollfd));
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
	json_object *j_get_aer = NULL;
	json_object *j_get_aer_0 = NULL;
	json_object *j_get_aer_1 = NULL;
	json_object *j_disable_aer = NULL;
	json_object *j_disable_aer_0 = NULL;
	json_object *j_disable_aer_1 = NULL;
	json_object *j_set_aer = NULL;
	json_object *j_set_aer_0 = NULL;
	json_object *j_set_aer_1 = NULL;
	json_object *j_config_sensors_enabled = NULL;
	json_object *j_sensors = NULL;
	int res = 1;
	int sensor_entries;
	int i;
	size_t len;

	root = json_tokener_parse_verbose(cfg, &j_err);
	if (!root) {
		LOG("error parsing config: %s\n",
		    json_tokener_error_desc(j_err));
		return 1;
	}

	// cool-down
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

	// get-aer
	if (!json_object_object_get_ex(root,
				       "get-aer",
				       &j_get_aer)) {
		LOG("failed to find get-aer key in config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_get_aer, json_type_array)) {
		LOG("get-aer key not array.\n");
		goto out_put;
	}

	if (json_object_array_length(j_get_aer) != 2) {
		LOG("get-aer key expects two entries.\n");
		goto out_put;
	}

	// get-aer[0]
	j_get_aer_0 = json_object_array_get_idx(j_get_aer, 0);
	if (!json_object_is_type(j_get_aer_0, json_type_string)) {
		LOG("get-aer[0] key is not a string.\n");
		goto out_put;
	}

	len = strnlen(json_object_get_string(j_get_aer_0), MAX_AER_CMD - 1);
	memcpy(vc->get_aer[0],
	       json_object_get_string(j_get_aer_0),
	       len);
	vc->get_aer[0][len] = '\0';

	// get_aer[1]
	j_get_aer_1 = json_object_array_get_idx(j_get_aer, 1);
	if (!json_object_is_type(j_get_aer_1, json_type_string)) {
		LOG("get-aer[1] key is not a string.\n");
		goto out_put;
	}

	len = strnlen(json_object_get_string(j_get_aer_1), MAX_AER_CMD - 1);
	memcpy(vc->get_aer[1],
	       json_object_get_string(j_get_aer_1),
	       len);
	vc->get_aer[1][len] = '\0';

	// disable-aer
	if (!json_object_object_get_ex(root,
				       "disable-aer",
				       &j_disable_aer)) {
		LOG("failed to find disable-aer key in config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_disable_aer, json_type_array)) {
		LOG("disable-aer key not array.\n");
		goto out_put;
	}

	if (json_object_array_length(j_disable_aer) != 2) {
		LOG("disable-aer key expects two entries.\n");
		goto out_put;
	}

	// disable-aer[0]
	j_disable_aer_0 = json_object_array_get_idx(j_disable_aer, 0);
	if (!json_object_is_type(j_disable_aer_0, json_type_string)) {
		LOG("disable-aer[0] key is not a string.\n");
		goto out_put;
	}

	len = strnlen(json_object_get_string(j_disable_aer_0), MAX_AER_CMD - 1);
	memcpy(vc->disable_aer[0],
	       json_object_get_string(j_disable_aer_0),
	       len);
	vc->disable_aer[0][len] = '\0';

	// disable-aer[1]
	j_disable_aer_1 = json_object_array_get_idx(j_disable_aer, 1);
	if (!json_object_is_type(j_disable_aer_1, json_type_string)) {
		LOG("disable-aer[1] key is not a string.\n");
		goto out_put;
	}

	len = strnlen(json_object_get_string(j_disable_aer_1), MAX_AER_CMD - 1);
	memcpy(vc->disable_aer[1],
	       json_object_get_string(j_disable_aer_1),
	       len);
	vc->disable_aer[1][len] = '\0';

	// set-aer
	if (!json_object_object_get_ex(root,
				       "set-aer",
				       &j_set_aer)) {
		LOG("failed to find set-aer key in config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_set_aer, json_type_array)) {
		LOG("set-aer key not array.\n");
		goto out_put;
	}

	if (json_object_array_length(j_set_aer) != 2) {
		LOG("set-aer key expects two entries.\n");
		goto out_put;
	}

	// set-aer[0]
	j_set_aer_0 = json_object_array_get_idx(j_set_aer, 0);
	if (!json_object_is_type(j_set_aer_0, json_type_string)) {
		LOG("set-aer[0] key is not a string.\n");
		goto out_put;
	}

	len = strnlen(json_object_get_string(j_set_aer_0), MAX_AER_CMD - 1);
	memcpy(vc->set_aer[0],
	       json_object_get_string(j_set_aer_0),
	       len);
	vc->set_aer[0][len] = '\0';

	// set_aer[1]
	j_set_aer_1 = json_object_array_get_idx(j_set_aer, 1);
	if (!json_object_is_type(j_set_aer_1, json_type_string)) {
		LOG("set-aer[1] key is not a string.\n");
		goto out_put;
	}

	len = strnlen(json_object_get_string(j_set_aer_1), MAX_AER_CMD - 1);
	memcpy(vc->set_aer[1],
	       json_object_get_string(j_set_aer_1),
	       len);
	vc->set_aer[1][len] = '\0';

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
		json_object *j_name;
		json_object *j_high_fatal;
		json_object *j_high_warn;
		json_object *j_low_fatal;
		json_object *j_low_warn;

		if (!json_object_object_get_ex(j_sensor_sub_i,
					       "name",
					       &j_name)) {
			LOG("failed to find name key in config sensors[%d].\n"
			    "Skipping entry %d.\n", i, i);
			vc->config_sensors[i].name[0] = '\0';
			vc->config_sensors[i].flags = FPGAD_SENSOR_VC_IGNORE;
			continue;
		}

		if (!json_object_is_type(j_name, json_type_string)) {
			LOG("sensors[%d].name key not string.\n"
			    "Skipping entry %d.\n", i, i);
			vc->config_sensors[i].name[0] = '\0';
			vc->config_sensors[i].flags = FPGAD_SENSOR_VC_IGNORE;
			continue;
		}

		len = strnlen(json_object_get_string(j_name), MAX_SENSOR_NAME - 1);
		memcpy(vc->config_sensors[i].name,
		       json_object_get_string(j_name),
		       len);
		vc->config_sensors[i].name[len] = '\0';

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
				LOG("user \"%s\" high-fatal: %f\n",
				    vc->config_sensors[i].name,
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
				LOG("user \"%s\" high-warn: %f\n",
				    vc->config_sensors[i].name,
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
				LOG("user \"%s\" low-fatal: %f\n",
				    vc->config_sensors[i].name,
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
				LOG("user \"%s\" low-warn: %f\n",
				    vc->config_sensors[i].name,
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
		fpga_properties props = NULL;
		uint16_t seg = 0;
		uint8_t bus = 0;
		uint8_t dev = 0;
		uint8_t fn = 0;

		d->thread_fn = monitor_fme_vc_thread;
		d->thread_stop_fn = stop_vc_threads;

		vc = calloc(1, sizeof(vc_device));
		if (!vc)
			return res;

		vc->base_device = d;
		d->thread_context = vc;

		res = vc_parse_config(vc, cfg);
		if (res) {
			free(vc);
			goto out_exit;
		}

		if (fpgaGetProperties(d->token, &props) != FPGA_OK) {
			LOG("failed to get properties.\n");
			goto out_exit;

		}

		if ((fpgaPropertiesGetSegment(props, &seg) != FPGA_OK) ||
		    (fpgaPropertiesGetBus(props, &bus) != FPGA_OK) ||
		    (fpgaPropertiesGetDevice(props, &dev) != FPGA_OK) ||
		    (fpgaPropertiesGetFunction(props, &fn) != FPGA_OK)) {
			LOG("failed to get PCI attributes.\n");
			fpgaDestroyProperties(&props);
			goto out_exit;
		}

		fpgaDestroyProperties(&props);

		snprintf(vc->sbdf, sizeof(vc->sbdf), "%04x:%02x:%02x.%d",
			 (int)seg, (int)bus, (int)dev, (int)fn);

		LOG("monitoring vid=0x%04x did=0x%04x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");

	}

	// Not currently monitoring the Port device
out_exit:
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
