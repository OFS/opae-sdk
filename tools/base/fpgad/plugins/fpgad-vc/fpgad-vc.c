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

#define MAX_VC_SENSORS 128
typedef struct _vc_device {
	fpgad_monitored_device *base_device;
	fpga_object group_object;
	vc_sensor sensors[MAX_VC_SENSORS];
	uint32_t num_sensors;
	uint64_t max_sensor_id;
	uint8_t *state_tripped; // bit set
	uint8_t *state_last;    // bit set
	char drv_rm_path[SYSFS_PATH_MAX];
	char drv_rescan_path[SYSFS_PATH_MAX];
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
	fpgaDestroyObject(&sensor->value_object);
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

	fpgaDestroyObject(&vc->group_object);

	if (vc->state_tripped) {
		free(vc->state_tripped);
		vc->state_tripped = NULL;
	}

	if (vc->state_last) {
		free(vc->state_last);
		vc->state_last = NULL;
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
STATIC fpga_result vc_sensor_get(vc_sensor *s)
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
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_VC_HIGH_WARN_VALID;
		if (is_temperature)
			s->high_warn -= VC_DEGREES_ADJUST_TEMP;
		else
			s->high_warn -=
				(s->high_warn * VC_PERCENT_ADJUST_PWR) / 100;
	} else
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
	if (res == FPGA_OK) {
		s->flags |= FPGAD_SENSOR_VC_LOW_WARN_VALID;
		if (is_temperature)
			s->low_warn += VC_DEGREES_ADJUST_TEMP;
		else
			s->low_warn +=
				(s->low_warn * VC_PERCENT_ADJUST_PWR) / 100;
	} else
		s->flags &= ~FPGAD_SENSOR_VC_LOW_WARN_VALID;

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

	res = vc_sensor_get(s);
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

STATIC int tripped_count;

STATIC bool vc_monitor_sensors(vc_device *vc)
{
	uint32_t i;
	bool negative_trans = false;
	bool res = true;

	for (i = 0 ; i < vc->num_sensors ; ++i) {
		vc_sensor *s = &vc->sensors[i];

		if (s->flags & FPGAD_SENSOR_VC_IGNORE)
			continue;

		if (fpgaObjectRead64(s->value_object,
				     &s->value,
				     FPGA_OBJECT_SYNC) != FPGA_OK) {
			if (++s->read_errors >=
				FPGAD_SENSOR_VC_MAX_READ_ERRORS)
				s->flags |= FPGAD_SENSOR_VC_IGNORE;
			continue;
		}

		if (HIGH_FATAL(s) || LOW_FATAL(s)) {
			opae_api_send_EVENT_POWER_THERMAL(vc->base_device);
			BIT_SET_SET(vc->state_tripped, s->id);
			if (!BIT_IS_SET(vc->state_last, s->id)) {
				LOG("sensor '%s' fatal.\n", s->name);
			}
			res = false;
		} else if (HIGH_WARN(s) || LOW_WARN(s)) {
			opae_api_send_EVENT_POWER_THERMAL(vc->base_device);
			BIT_SET_SET(vc->state_tripped, s->id);
			if (!BIT_IS_SET(vc->state_last, s->id)) {
				LOG("sensor '%s' warning.\n", s->name);
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
			LOG_MOD(tripped_count,
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
			tripped_count = 0;
		}
	}

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

STATIC fpga_result vc_unload_driver(const char *path)
{
	LOG("writing 1 to %s to remove driver.\n", path);
	if (file_write_string(path, "1\n", 2)) {
		LOG("failed to write \"%s\"\n", path);
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

STATIC fpga_result vc_force_pci_rescan(const char *path)
{
	LOG("writing 1 to %s to rescan the device.\n", path);
	if (file_write_string(path, "1\n", 2)) {
		LOG("failed to write \"%s\"\n", path);
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

STATIC pthread_mutex_t cool_down_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
STATIC int cool_down = 30;

STATIC void *monitor_fme_vc_thread(void *arg)
{
	fpgad_monitored_device *d =
		(fpgad_monitored_device *)arg;

	vc_device *vc = (vc_device *)d->thread_context;

	uint32_t enum_retries = 0;
	uint8_t *save_state_last = NULL;

	int i;
	errno_t err;

	tripped_count = 0;

	while (vc_threads_running) {

		while (vc_enum_sensors(vc) != FPGA_OK) {
			LOG_MOD(enum_retries, "waiting to enumerate sensors.\n");
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
			usleep(d->config->poll_interval_usec);

			if (!vc_threads_running) {
				vc_destroy_sensors(vc);
				return NULL;
			}
		}

		save_state_last = vc->state_last;
		vc->state_last = NULL;

		vc_destroy_sensors(vc);

		fpgad_mutex_lock(err, &cool_down_lock);

		if (vc_unload_driver(vc->drv_rm_path) == FPGA_OK) {

			for (i = 0 ; i < cool_down ; ++i) {
				if (!vc_threads_running) {
					fpgad_mutex_unlock(err,
							   &cool_down_lock);
					return NULL;
				}
				sleep(1);
			}

			if (vc_force_pci_rescan(vc->drv_rescan_path) != FPGA_OK)
				LOG("failed to force PCI bus rescan.\n");
		}

		fpgad_mutex_unlock(err, &cool_down_lock);
	}

	return NULL;
}

STATIC int vc_parse_config(const char *cfg)
{
	json_object *root;
	enum json_tokener_error j_err = json_tokener_success;
	json_object *j_cool_down = NULL;
	int res = 1;

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
		uint16_t seg = 0;
		uint8_t bus = 0;
		uint8_t dev = 0;
		uint8_t fn = 0;
		fpga_properties prop = NULL;
		fpga_result fpga_res;

		d->thread_fn = monitor_fme_vc_thread;
		d->thread_stop_fn = stop_vc_threads;

		vc = calloc(1, sizeof(vc_device));
		if (!vc)
			return res;

		vc->base_device = d;
		d->thread_context = vc;

		fpga_res = fpgaGetProperties(d->token, &prop);
		if (fpga_res != FPGA_OK) {
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

		snprintf_s_i(vc->drv_rm_path, SYSFS_PATH_MAX,
				"/sys/bus/pci/devices/%04x:",
				(int)seg);

		snprintf_s_i(&vc->drv_rm_path[26], SYSFS_PATH_MAX - 26,
				"%02x:", (int)bus);

		snprintf_s_i(&vc->drv_rm_path[29], SYSFS_PATH_MAX - 29,
				"%02x.", (int)dev);

		snprintf_s_i(&vc->drv_rm_path[32], SYSFS_PATH_MAX - 32,
				"%d/remove", (int)fn);


		snprintf_s_i(vc->drv_rescan_path, SYSFS_PATH_MAX,
				"/sys/bus/pci/devices/%04x:",
				(int)seg);

		snprintf_s_i(&vc->drv_rescan_path[26], SYSFS_PATH_MAX - 26,
				"%02x:", (int)bus);

		snprintf_s_i(&vc->drv_rescan_path[29], SYSFS_PATH_MAX - 29,
				"%02x.", (int)dev);

		snprintf_s_i(&vc->drv_rescan_path[32], SYSFS_PATH_MAX - 32,
				"%d/rescan", (int)fn);

		LOG("monitoring vid=0x%04x did=0x%04x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");

		if (!vc_parse_config(cfg))
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
