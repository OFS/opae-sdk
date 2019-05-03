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
#include <float.h>
#include <math.h>
#include <glob.h>
#include <json-c/json.h>

#include "safe_string/safe_string.h"
#include "fpgad/api/opae_events_api.h"
#include "fpgad/api/device_monitoring.h"
#include "fpgad/api/sysfs.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("fpgad-dc: " format, ##__VA_ARGS__)

#define SYSFS_PATH_MAX 256

#define TRIM_LOG_MODULUS 20
#define LOG_MOD(__r, __fmt, ...) \
do { \
\
	++(__r); \
	if (!((__r) % TRIM_LOG_MODULUS)) { \
		log_printf("fpgad-dc: " __fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define FPGA_SENSOR_MILLI  1000

#define FPGA_SENSOR_HIGH_FATAL         "high_fatal"
#define FPGA_SENSOR_HIGH_WARN          "high_warn"
#define FPGA_SENSOR_LOW_FATAL          "low_fatal"
#define FPGA_SENSOR_LOW_WARN           "low_warn"
#define FPGA_SENSOR_ID                 "id"
#define FPGA_SENSOR_NAME               "name"
#define FPGA_SENSOR_VALUE              "value"
#define FPGA_SENSOR_TYPE               "type"
#define FPGA_SENSOR_TEMP               "Temperature"
#define FPGA_DC_SENSOR_PATH            "spi-*.auto/spi_master/spi*/spi*.*"


typedef struct _dc_sensor {
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
#define FPGAD_SENSOR_DC_IGNORE           0x00000001
#define FPGAD_SENSOR_DC_HIGH_FATAL_VALID 0x00000002
#define FPGAD_SENSOR_DC_HIGH_WARN_VALID  0x00000004
#define FPGAD_SENSOR_DC_LOW_FATAL_VALID  0x00000008
#define FPGAD_SENSOR_DC_LOW_WARN_VALID   0x00000010
	uint32_t read_errors;
#define FPGAD_SENSOR_DC_MAX_READ_ERRORS  25

	uint64_t id_cfg;
	double high_fatal_cfg;
	double high_warn_cfg;
	double low_fatal_cfg;
	double low_warn_cfg;

} dc_sensor;



#define MAX_DC_SENSORS 128
typedef struct _dc_device {
	fpgad_monitored_device *base_device;
	fpga_object group_object;
	dc_sensor sensors[MAX_DC_SENSORS];
	uint32_t num_sensors;
	uint64_t max_sensor_id;
	uint8_t *state_tripped; // bit set
	uint8_t *state_last;    // bit set

	bool remove_driver;
} dc_device;

#define BIT_SET_MASK(__n)  (1 << ((__n) % 8))
#define BIT_SET_INDEX(__n) ((__n) / 8)

#define BIT_SET_SET(__s, __n) \
((__s)[BIT_SET_INDEX(__n)] |= BIT_SET_MASK(__n))

#define BIT_SET_CLR(__s, __n) \
((__s)[BIT_SET_INDEX(__n)] &= ~BIT_SET_MASK(__n))

#define BIT_IS_SET(__s, __n) \
((__s)[BIT_SET_INDEX(__n)] & BIT_SET_MASK(__n))


#define HIGH_FATAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_DC_HIGH_FATAL_VALID) && \
 ((__sens)->value > (__sens)->high_fatal))

#define HIGH_WARN(__sens) \
(((__sens)->flags & FPGAD_SENSOR_DC_HIGH_WARN_VALID) && \
 ((__sens)->value > (__sens)->high_warn))

#define HIGH_NORMAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_DC_HIGH_WARN_VALID) && \
 ((__sens)->value <= (__sens)->high_warn))

#define LOW_FATAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_DC_LOW_FATAL_VALID) && \
 ((__sens)->value < (__sens)->low_fatal))

#define LOW_WARN(__sens) \
(((__sens)->flags & FPGAD_SENSOR_DC_LOW_WARN_VALID) && \
 ((__sens)->value < (__sens)->low_warn))

#define LOW_NORMAL(__sens) \
(((__sens)->flags & FPGAD_SENSOR_DC_LOW_WARN_VALID) && \
 ((__sens)->value >= (__sens)->low_warn))

STATIC bool dc_threads_running = true;

STATIC void stop_dc_threads(void)
{
	dc_threads_running = false;
}

STATIC void dc_destroy_sensor(dc_sensor *sensor)
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
	fpgaDestroyObject(&sensor->value_object);
	sensor->flags = 0;
	sensor->read_errors = 0;
}

STATIC void dc_destroy_sensors(dc_device *dc)
{
	uint32_t i;

	for (i = 0 ; i < dc->num_sensors ; ++i) {
		dc_destroy_sensor(&dc->sensors[i]);
	}
	dc->num_sensors = 0;
	dc->max_sensor_id = 0;

	fpgaDestroyObject(&dc->group_object);

	if (dc->state_tripped) {
		free(dc->state_tripped);
		dc->state_tripped = NULL;
	}

	if (dc->state_last) {
		free(dc->state_last);
		dc->state_last = NULL;
	}
}

STATIC fpga_result dc_sensor_get_string(dc_sensor *sensor,
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

STATIC fpga_result dc_sensor_get_u64(dc_sensor *sensor,
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
#define DC_PERCENT_ADJUST_PWR 5
// The number of degrees by which we adjust the
// temperature trip points so that we catch anomolies
// before the hw does.
#define DC_DEGREES_ADJUST_TEMP 5
STATIC fpga_result dc_sensor_get(dc_sensor *s)
{
	fpga_result res;
	bool is_temperature;
	int indicator = -1;

	if (s->name) {
		free(s->name);
		s->name = NULL;
	}

	if (s->type) {
		free(s->type);
		s->type = NULL;
	}

	res = dc_sensor_get_string(s, FPGA_SENSOR_NAME, &s->name);
	if (res != FPGA_OK)
		return res;

	res = dc_sensor_get_string(s, FPGA_SENSOR_TYPE, &s->type);
	if (res != FPGA_OK)
		return res;

	res = dc_sensor_get_u64(s, FPGA_SENSOR_ID, &s->id);
	if (res != FPGA_OK)
		return res;

	res = fpgaObjectRead64(s->value_object, &s->value, 0);
	if (res != FPGA_OK)
		return res;

	strcmp_s(s->type, 11, FPGA_SENSOR_TEMP, &indicator);
	is_temperature = (indicator == 0);

	res = dc_sensor_get_u64(s, FPGA_SENSOR_HIGH_FATAL, &s->high_fatal);
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_DC_HIGH_FATAL_VALID;
		if (is_temperature)
			s->high_fatal -= DC_DEGREES_ADJUST_TEMP;
		else
			s->high_fatal -=
				(s->high_fatal * DC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_DC_HIGH_FATAL_VALID;

	res = dc_sensor_get_u64(s, FPGA_SENSOR_HIGH_WARN, &s->high_warn);
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_DC_HIGH_WARN_VALID;
		if (is_temperature)
			s->high_warn -= DC_DEGREES_ADJUST_TEMP;
		else
			s->high_warn -=
				(s->high_warn * DC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_DC_HIGH_WARN_VALID;

	res = dc_sensor_get_u64(s, FPGA_SENSOR_LOW_FATAL, &s->low_fatal);
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_DC_LOW_FATAL_VALID;
		if (is_temperature)
			s->low_fatal += DC_DEGREES_ADJUST_TEMP;
		else
			s->low_fatal +=
				(s->low_fatal * DC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_DC_LOW_FATAL_VALID;

	res = dc_sensor_get_u64(s, FPGA_SENSOR_LOW_WARN, &s->low_warn);
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_DC_LOW_WARN_VALID;
		if (is_temperature)
			s->low_warn += DC_DEGREES_ADJUST_TEMP;
		else
			s->low_warn +=
				(s->low_warn * DC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_DC_LOW_WARN_VALID;



	// Set High fatal Config Values
	if ((s->flags & FPGAD_SENSOR_DC_HIGH_WARN_VALID) &&
		s->high_fatal_cfg != DBL_MAX) {

		if (is_temperature) {
			s->high_fatal = s->high_fatal_cfg * FPGA_SENSOR_MILLI;
			s->high_fatal -= DC_DEGREES_ADJUST_TEMP;
		} else {
			s->high_fatal = s->high_fatal_cfg * FPGA_SENSOR_MILLI;
			s->high_fatal -= (s->high_fatal * DC_PERCENT_ADJUST_PWR) / 100;
		}

	 }

	// Set High warn Config Values
	if ((s->flags & FPGAD_SENSOR_DC_HIGH_WARN_VALID) &&
		s->high_warn_cfg != DBL_MAX) {

		if (is_temperature) {
			s->high_warn = s->high_warn_cfg * FPGA_SENSOR_MILLI;
			s->high_warn -= DC_DEGREES_ADJUST_TEMP;
		} else {
			s->high_warn = s->high_warn_cfg * FPGA_SENSOR_MILLI;
			s->high_warn -= (s->high_warn * DC_PERCENT_ADJUST_PWR) / 100;
		}
	}

	// Set low fatal Config Values
	if ((s->flags & FPGAD_SENSOR_DC_LOW_FATAL_VALID) &&
		s->low_fatal_cfg != DBL_MAX) {

		if (is_temperature) {
			s->low_fatal = s->low_fatal_cfg * FPGA_SENSOR_MILLI;
			s->low_fatal += DC_DEGREES_ADJUST_TEMP;
		} else {
			s->low_fatal = s->low_fatal_cfg * FPGA_SENSOR_MILLI;
			s->low_fatal += (s->low_fatal * DC_PERCENT_ADJUST_PWR) / 100;
		}

	}

	// Set low warn Config Values
	if ((s->flags & FPGAD_SENSOR_DC_LOW_WARN_VALID) &&
		s->low_warn_cfg != DBL_MAX) {

		if (is_temperature) {
			s->low_warn = s->low_warn_cfg * FPGA_SENSOR_MILLI;
			s->low_warn -= DC_DEGREES_ADJUST_TEMP;
		} else {
			s->low_warn = s->low_warn_cfg * FPGA_SENSOR_MILLI;
			s->low_warn -= (s->low_warn * DC_PERCENT_ADJUST_PWR) / 100;
		}

	}

	return FPGA_OK;
}

STATIC fpga_result dc_enum_sensor(dc_device *dc,
				  const char *name)
{
	fpga_result res;
	dc_sensor *s;

	if (dc->num_sensors == MAX_DC_SENSORS) {
		LOG("exceeded max number of sensors.\n");
		return FPGA_EXCEPTION;
	}

	s = &dc->sensors[dc->num_sensors];

	res = fpgaObjectGetObject(dc->group_object,
				  name,
				  &s->sensor_object,
				  0);

	if (res != FPGA_OK)
		return res;

	res = fpgaObjectGetObject(s->sensor_object,
				  FPGA_SENSOR_VALUE,
				  &s->value_object,
				  0);

	if (res != FPGA_OK) {
		LOG("failed to get value object for %s.\n", name);
		fpgaDestroyObject(&s->sensor_object);
		return res;
	}
	res = dc_sensor_get(s);
	if (res == FPGA_OK)
		++dc->num_sensors;
	else {
		LOG("warning: sensor attribute enumeration failed.\n");
		dc_destroy_sensor(s);
	}

	return res;
}

STATIC fpga_result dc_enum_sensors(dc_device *dc)
{
	fpga_result res;
	char name[SYSFS_PATH_MAX];
	int i;

	res = fpgaTokenGetObject(dc->base_device->token,
				 FPGA_DC_SENSOR_PATH,
				 &dc->group_object,
				 FPGA_OBJECT_GLOB);

	if (res)
		return res;

	for (i = 0 ; i < MAX_DC_SENSORS ; ++i) {
		snprintf_s_i(name, sizeof(name), "sensor%d", i);
		dc_enum_sensor(dc, name);
	}

	if (dc->num_sensors > 0) {
		dc_sensor *s = &dc->sensors[dc->num_sensors - 1];

		dc->max_sensor_id = s->id;

		dc->state_tripped = calloc((dc->max_sensor_id + 7) / 8, 1);
		dc->state_last = calloc((dc->max_sensor_id + 7) / 8, 1);

		return (dc->state_tripped && dc->state_last) ?
			FPGA_OK : FPGA_NO_MEMORY;
	}

	return FPGA_NOT_FOUND;
}

STATIC int tripped_count;

STATIC bool dc_monitor_sensors(dc_device *dc)
{
	uint32_t i;
	bool negative_trans = false;
	bool res = true;

	for (i = 0 ; i < dc->num_sensors ; ++i) {
		dc_sensor *s = &dc->sensors[i];

		if (s->flags & FPGAD_SENSOR_DC_IGNORE)
			continue;

		if (fpgaObjectRead64(s->value_object,
				     &s->value,
				     FPGA_OBJECT_SYNC) != FPGA_OK) {
			if (++s->read_errors >=
				FPGAD_SENSOR_DC_MAX_READ_ERRORS)
				s->flags |= FPGAD_SENSOR_DC_IGNORE;
			continue;
		}

		if (HIGH_FATAL(s) || LOW_FATAL(s)) {
			opae_api_send_EVENT_POWER_THERMAL(dc->base_device);
			BIT_SET_SET(dc->state_tripped, s->id);
			if (!BIT_IS_SET(dc->state_last, s->id)) {
				LOG("sensor '%s' fatal.\n", s->name);
			}
			res = false;
		} else if (HIGH_WARN(s) || LOW_WARN(s)) {
			opae_api_send_EVENT_POWER_THERMAL(dc->base_device);
			BIT_SET_SET(dc->state_tripped, s->id);
			if (!BIT_IS_SET(dc->state_last, s->id)) {
				LOG("sensor '%s' warning.\n", s->name);
			}
		}

		if (HIGH_NORMAL(s) || LOW_NORMAL(s)) {
			if (BIT_IS_SET(dc->state_last, s->id)) {
				negative_trans = true;
				LOG("sensor '%s' back to normal.\n", s->name);
			}
		}

		if (BIT_IS_SET(dc->state_last, s->id) &&
		    BIT_IS_SET(dc->state_tripped, s->id)) {
			LOG_MOD(tripped_count,
				"sensor '%s' still tripped.\n", s->name);
		}
	}

	if (negative_trans) {
		for (i = 0 ; i < dc->num_sensors ; ++i) {
			if (BIT_IS_SET(dc->state_tripped, dc->sensors[i].id))
				break;
		}

		if (i == dc->num_sensors) {
			// no remaining tripped sensors
			tripped_count = 0;
		}
	}

	for (i = 0 ; i < dc->num_sensors ; ++i) {
		dc_sensor *s = &dc->sensors[i];
		if (BIT_IS_SET(dc->state_tripped, s->id))
			BIT_SET_SET(dc->state_last, s->id);
		else
			BIT_SET_CLR(dc->state_last, s->id);
	}

	memset_s(dc->state_tripped, (dc->max_sensor_id + 7) / 8, 0);

	return res;
}

STATIC fpga_result dc_unload_driver(dc_device *dc)
{
	fpga_token token;
	fpga_result res;
	fpga_properties prop = NULL;
	char path[SYSFS_PATH_MAX];
	uint16_t seg = 0;
	uint8_t bus = 0;
	uint8_t dev = 0;
	uint8_t fn = 0;

	token = dc->base_device->token;

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
		return res;
	}

	fpgaDestroyProperties(&prop);

	//           11111111112222222222333
	// 012345678901234567890123456789012
	// /sys/bus/pci/devices/ssss:bb:dd.f

	snprintf_s_i(path, SYSFS_PATH_MAX,
		     "/sys/bus/pci/devices/%04x:",
		     (int)seg);

	snprintf_s_i(&path[26], SYSFS_PATH_MAX - 26,
		     "%02x:", (int)bus);

	snprintf_s_i(&path[29], SYSFS_PATH_MAX - 29,
		     "%02x.", (int)dev);

	snprintf_s_i(&path[32], SYSFS_PATH_MAX - 32,
		     "%d/remove", (int)fn);

	LOG("writing 1 to %s to remove driver.\n", path);
	printf("writing 1 to %s to remove driver.\n", path);

	if (file_write_string(path, "1\n", 2)) {
		LOG("failed to write \"%s\"\n", path);
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

STATIC fpga_result dc_force_pci_rescan(void)
{
	const char *path = "/sys/bus/pci/rescan";
	LOG("writing 1 to %s to rescan the device.\n", path);
	if (file_write_string(path, "1\n", 2)) {
		LOG("failed to write \"%s\"\n", path);
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

STATIC pthread_mutex_t cool_down_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
STATIC int cool_down = 30;


fpga_result program_null_gbs(fpgad_monitored_device *d)
{

	fpga_result res  = FPGA_OK;
	fpga_handle fme_h = NULL;
	int tries = 100;
	// Wiat for PCI scan and Load driver
	while (((res = fpgaOpen(d->token, &fme_h, 0)) != FPGA_OK) && (tries >= 0)) {
		if (0 == (tries % 20)) {
			LOG(" waiting for pci rescan.\n");
		}
		tries--;
		usleep(500 * 1000);
	}

	if (FPGA_OK != res) {
		LOG(" failed to fpga driver.\n");
		return res;
	}


	// reconfigure NULL GBS
	if (d->bitstr) {

		const uint32_t slot = 0;

		LOG("programming \"%s\": ", d->bitstr->filename);

		res = fpgaReconfigureSlot(fme_h,
			slot,
			d->bitstr->data,
			d->bitstr->data_len,
			FPGA_RECONF_FORCE);
		if (res != FPGA_OK)
			LOG("SUCCESS\n");
		else
			LOG("FAILED : %s\n", fpgaErrStr(res));


	}  else {
		LOG("no bitstream to program for MAX10 Tripp!\n");
	}

	fpgaClose(fme_h);

	return res;
}

STATIC void *monitor_fme_dc_thread(void *arg)
{
	fpgad_monitored_device *d =
		(fpgad_monitored_device *)arg;

	dc_device *dc = (dc_device *)d->thread_context;

	uint32_t enum_retries = 0;
	uint8_t *save_state_last = NULL;

	int i;
	errno_t err;

	tripped_count = 0;

	while (dc_threads_running) {

		while (dc_enum_sensors(dc) != FPGA_OK) {
			LOG_MOD(enum_retries, "waiting to enumerate sensors.\n");
			if (!dc_threads_running)
				return NULL;
			sleep(1);
		}

		if (save_state_last) {
			free(dc->state_last);
			dc->state_last = save_state_last;
			save_state_last = NULL;
		}

		while (dc_monitor_sensors(dc)) {
			usleep(d->config->poll_interval_usec);

			if (!dc_threads_running) {
				dc_destroy_sensors(dc);
				return NULL;
			}
		}

		save_state_last = dc->state_last;
		dc->state_last = NULL;

		dc_destroy_sensors(dc);

		if (dc->remove_driver) {

			fpgad_mutex_lock(err, &cool_down_lock);

			if (dc_unload_driver(dc) == FPGA_OK) {

				for (i = 0; i < cool_down; ++i) {

					if (!dc_threads_running) {
						fpgad_mutex_unlock(err,
							&cool_down_lock);
						return NULL;
					}
					sleep(1);
				}

				if (dc_force_pci_rescan() != FPGA_OK) {
					LOG("failed to force PCI bus rescan.\n");
				}
			}

			fpgad_mutex_unlock(err, &cool_down_lock);

		} //End of Remove driver

		if (program_null_gbs(d) != FPGA_OK) {
			LOG("failed to reconfigure null gbs.\n");
		}
	}

	return NULL;
}


STATIC int dc_parse_config(const char *cfg, dc_device *dc)
{
	json_object *root;
	enum json_tokener_error j_err = json_tokener_success;
	json_object *j_cool_down = NULL;
	json_object *j_sensor = NULL;
	json_object *j_remove_driver = NULL;
	int sensor_count = 0;
	int sensor_id = 0;
	int res = 1;
	int i = 0;

	dc->remove_driver = false;


	for (i = 0; i < MAX_DC_SENSORS; i++) {
		dc->sensors[i].high_fatal_cfg = DBL_MAX;
		dc->sensors[i].high_warn_cfg = DBL_MAX;
		dc->sensors[i].low_fatal_cfg = -DBL_MAX;
		dc->sensors[i].low_warn_cfg = -DBL_MAX;
	}

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

	if (!json_object_object_get_ex(root,
		"driver-remove",
		&j_remove_driver)) {
		LOG("failed to find remove driver key in config.\n");
		goto out_put;
	}

	if (!json_object_is_type(j_remove_driver, json_type_boolean)) {
		LOG("remove driver not true.\n");
		goto out_put;
	}

	dc->remove_driver = json_object_get_boolean(j_remove_driver);

	LOG("remove driver in case of tripp %d \n", dc->remove_driver);

	//  sensor array
	if (!json_object_object_get_ex(root,
		"senors",
		&j_sensor)) {
		LOG("failed to find sensor array in config\n");
		goto out_put;
	}

	if (!json_object_is_type(j_sensor, json_type_array)) {
		LOG("failed to find sensor array in config\n");
		goto out_put;
	}

	sensor_count = json_object_array_length(j_sensor);
	LOG("number of threshold sensor array :%d.\n", sensor_count);

	for (i = 0; i < sensor_count; ++i) {

		json_object *sensor_num_obj;
		json_object *high_fatal_obj;
		json_object *high_warn_obj;
		json_object *low_fatal_obj;
		json_object *low_warn_obj;

		json_object *j_sensor_index_obj = json_object_array_get_idx(j_sensor, i);

		if (j_sensor_index_obj == NULL) {
			LOG("Invalid threshold sensor array index :%d.\n", i);
			continue;
		}

		if (json_object_object_get_ex(j_sensor_index_obj, "sensor-id", &sensor_num_obj)) {
			sensor_id = json_object_get_int(sensor_num_obj);
			dc->sensors[sensor_id - 1].id_cfg = sensor_id;
			LOG("sensor_id :%d.\n", sensor_id);
		}

		if (sensor_id > MAX_DC_SENSORS) {
			continue;
		}

		if (json_object_object_get_ex(j_sensor_index_obj, "high-fatal", &high_fatal_obj)) {
			dc->sensors[sensor_id -1].high_fatal_cfg = json_object_get_double(high_fatal_obj);
			LOG("high-fatal :%f.\n", dc->sensors[sensor_id -1].high_fatal_cfg);

		}


		if (json_object_object_get_ex(j_sensor_index_obj, "high-warn", &high_warn_obj)) {
			dc->sensors[sensor_id -1].high_warn_cfg = json_object_get_double(high_warn_obj);
			LOG("high_warn :%f.\n", dc->sensors[sensor_id -1].high_warn_cfg);
		}

		if (json_object_object_get_ex(j_sensor_index_obj, "low-fatal", &low_fatal_obj)) {
			dc->sensors[sensor_id -1].low_fatal_cfg = json_object_get_double(low_fatal_obj);
			LOG("low-fatal :%f.\n", dc->sensors[sensor_id -1].low_fatal_cfg);
		}

		if (json_object_object_get_ex(j_sensor_index_obj, "low-warn", &low_warn_obj)) {
			dc->sensors[sensor_id -1].low_warn_cfg = json_object_get_double(high_fatal_obj);
			LOG("low-warn :%f.\n", dc->sensors[sensor_id -1].low_warn_cfg);
		}


	} // end for loop

	res = 0;

out_put:
	json_object_put(root);
	return res;
}

int fpgad_plugin_configure(fpgad_monitored_device *d,
			   const char *cfg)
{
	int res = 1;
	dc_device *dc;

	dc_threads_running = true;

	d->type = FPGAD_PLUGIN_TYPE_THREAD;

	if (d->object_type == FPGA_DEVICE) {

		d->thread_fn = monitor_fme_dc_thread;
		d->thread_stop_fn = stop_dc_threads;

		dc = calloc(1, sizeof(dc_device));
		if (!dc)
			return res;

		dc->base_device = d;
		d->thread_context = dc;

		LOG("monitoring vid=0x%04x did=0x%04x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");

		if (!dc_parse_config(cfg, dc))
			res = 0;
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
		free(d->thread_context);
		d->thread_context = NULL;
	}
}
