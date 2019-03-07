// Copyright(c) 2017-2019, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   andor other materials provided with the distribution.
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
//****************************************************************************
//
//****************************************************************************
#ifndef __USE_GNU
   #define __USE_GNU
#endif
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>

#include "safe_string/safe_string.h"
#include <opae/fpga.h>

// FIXME
#define FPGA_BBS_MIN_POWER               30  // watts

// MSR
#define MSR_CORE_COUNT                    0x35
#define MSR_PKG_RAPL_POWER_LIMIT          0x610
#define MSR_RAPL_POWER_UNIT               0x606

#define RDMSR_CMD_PATH                    "rdmsr -c0 -p %d 0x%lx"
#define SYFS_PID_MAX_PATH                 "/proc/sys/kernel/pid_max"

#define MSR_MAX_BUF_SIZE                  1024
#define SYSFS_PATH_MAX                    256
#define XEON_PWR_LIMIT                    "power_mgmt/xeon_limit"
#define FPGA_PWR_LIMIT                    "power_mgmt/fpga_limit"
#define FPGA_SYSFS_SOCKET_ID              "socket_id"


fpga_result get_package_power(int split_point, long double *pkg_power);
fpga_result cpuset_setaffinity(int socket, int split_point, uint64_t max_cpu_count);


fpga_result sysfs_read_u64(const char *path, uint64_t *u)
{
	int fd = -1;
	int res = 0;
	char buf[SYSFS_PATH_MAX] = { 0 };
	int b = 0;

	if (path == NULL) {
		OPAE_ERR("Invalid input path");
		return FPGA_INVALID_PARAM;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		FPGA_MSG("open(%s) failed", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t) -1 == lseek(fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		goto out_close;
	}

	do {
		res = read(fd, buf + b, sizeof(buf) -b);
		if (res <= 0) {
			FPGA_MSG("Read from %s failed", path);
			goto out_close;
		}
		b += res;
		if (((unsigned) b > sizeof(buf)) || (b <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b - 1] != '\n' && buf[b - 1] != '\0' && (unsigned) b < sizeof(buf));

	// erase \n
	buf[b - 1] = 0;

	*u = strtoull(buf, NULL, 0);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}


int readmsr(int split_point, uint64_t msr, uint64_t *value)
{
	FILE *fp                        = NULL;
	char data[MSR_MAX_BUF_SIZE]     = {0};
	char msr_cmd[MSR_MAX_BUF_SIZE]  = {0};

	if(value == NULL) {
		return -1;
	}

	snprintf_s_il(msr_cmd, sizeof(msr_cmd),
			RDMSR_CMD_PATH, split_point, msr);

	fp = popen(msr_cmd, "r");
	if (fp == NULL) {
		OPAE_ERR("Failed to run command\n");
		return -1;
	}

	while (fgets(data, sizeof(data) -1, fp) != NULL) {
		OPAE_DBG("\t : %s", data);
	}

	*value = strtoll(data, NULL, 16);

	pclose(fp);

	return 0;
}

// idle cpu cores
fpga_result set_cpu_core_idle(fpga_handle handle,
				uint64_t gbs_power)
{
	int socket_num                       = -1;
	int cpu_num                          = -1;
	int threads_num                      = -1;
	int cores_num                        = -1;
	uint64_t  msrvalue                   = 0;
	int split_point                      = 0;
	int threads_per_core                 = -1;
	long double total_power              = 0;
	long double available_cpu_pwr        = 0;
	uint64_t max_available_cpu           = 0;
	fpga_result result                   = FPGA_OK;
	uint64_t socketid                    = 0;
	uint64_t value                       = 0;
	long double xeon_pwr_limit           = 0;
	long double fpga_pwr_limit           = 0;
	long double core_power               = 0;

	fpga_object fpga_object;

	// XEON PWR LIMIT
	result = fpgaHandleGetObject(handle, XEON_PWR_LIMIT, &fpga_object, 0);
	if (result != FPGA_OK) {
		fprintf(stderr, "Failed to get Handle Object \n");
		return result;
	}

	result = fpgaObjectRead64(fpga_object, &value, 0);
	if (result != FPGA_OK) {
		fprintf(stderr, "Failed to Read Object \n");
		fpgaDestroyObject(&fpga_object);
		return result;
	}

	xeon_pwr_limit = value / 8;

	result = fpgaDestroyObject(&fpga_object);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object \n");
		return result;
	}

	// FPGA PWR LIMIT
	result = fpgaHandleGetObject(handle, FPGA_PWR_LIMIT, &fpga_object, 0);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Handle Object \n");
		return result;
	}

	result = fpgaObjectRead64(fpga_object, &value, 0);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Read Object \n");
		fpgaDestroyObject(&fpga_object);
		return result;
	}

	fpga_pwr_limit = value / 8;

	result = fpgaDestroyObject(&fpga_object);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object \n");
		return result;
	}


	// Socket id
	result = fpgaHandleGetObject(handle, FPGA_SYSFS_SOCKET_ID, &fpga_object, 0);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Handle Object \n");
		return result;
	}

	result = fpgaObjectRead64(fpga_object, &socketid, 0);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Read Object \n");
		fpgaDestroyObject(&fpga_object);
		return result;
	}

	result = fpgaDestroyObject(&fpga_object);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy Object \n");
		return result;
	}

	printf("Socket id        : %ld \n", socketid);
	printf("XEON Power limit : %Lf watts \n", xeon_pwr_limit);
	printf("FPGA pwr limit   : %Lf watts \n", fpga_pwr_limit);

	if (readmsr(socketid, MSR_CORE_COUNT, &msrvalue) != 0) {
		OPAE_ERR("Failed to read MSR");
		result = FPGA_EXCEPTION;
		return result;
	}

	OPAE_DBG("msrvalue : %lx", msrvalue);

	// Threads count in a socket
	threads_num = msrvalue & 0xff;

	// Cores count in a socket
	cores_num = ((msrvalue >> 16) & 0xff);

	// Threads per core
	if (cores_num > 0) {
		threads_per_core = threads_num / cores_num;
	} else {
		OPAE_ERR("Invalid core count");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	// CPU count
	cpu_num = sysconf(_SC_NPROCESSORS_ONLN);

	// Socket count
	if (cores_num > 0  && threads_per_core >0 ) {
		socket_num = cpu_num / threads_per_core / cores_num;
	} else  {
		OPAE_ERR("Invalid socket count");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	// Split point
	if (socketid == 0) {
		split_point = 0;
	}else {
		split_point = cpu_num / socket_num;
	}

	printf("Threads_num        : %d \n", threads_num);
	printf("CoreCount          : %d \n", cores_num);
	printf("Socket_num         : %d \n", socket_num);
	printf("Threads per core   : %d \n", threads_per_core);
	printf("CPU_num            : %d \n", cpu_num);
	printf("Split_point        : %d \n", split_point);

	// Get Package power
	result = get_package_power(split_point, &total_power);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to read Package power");
		return result;
	}

	// per core power
	core_power = xeon_pwr_limit / cores_num;

	printf("Total Power : %Lf \n", total_power);
	printf("Core Power  : %Lf \n", core_power);

	// Set to maximum gbs power if power setting is zero in metadata.
	if (gbs_power == 0) {
		gbs_power = fpga_pwr_limit - FPGA_BBS_MIN_POWER;
	}

	if ((gbs_power + FPGA_BBS_MIN_POWER) > fpga_pwr_limit) {
		OPAE_ERR("Invalid Input FPGA GBS Power");
		result = FPGA_INVALID_PARAM;
		return result;
	}

	//Shared TDP SKU 
	if (xeon_pwr_limit + fpga_pwr_limit > total_power) {

		printf("Shared TDP SKU power shared between XEON and FPGA \n");

		// Available power to CPU
		available_cpu_pwr = (int)total_power -
			(gbs_power + FPGA_BBS_MIN_POWER);

		printf("Available CPU power: %Lf \n", available_cpu_pwr);

		// Max number of CPU available
		max_available_cpu = 2 * ((int) available_cpu_pwr / core_power);

		printf("Online CPU count: %ld \n", max_available_cpu);

		result = cpuset_setaffinity(socketid, split_point,
				max_available_cpu);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to idle cores");
			return result;
		}

	} else if (xeon_pwr_limit + fpga_pwr_limit <= total_power) {
		// TDP+ SKU
		printf("TDP+ SKU XEON and FPGA each can run maximum allowed TDP \n");
		result = FPGA_INVALID_PARAM;
		return result;

	} else {
		OPAE_ERR("Invalid Socket Power");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	return result;
}

// get Package power
fpga_result get_package_power( int split_point,
			long double *pkg_power)
{
	uint64_t pkg_pwr_limit          = 0;
	long double power_unit_value    = 0;
	uint64_t msrvalue               = 0;
	long double total_watts         = 0;
	fpga_result result              = FPGA_OK;

	if (pkg_power == NULL) {
		OPAE_ERR("Invalid input pkg power.\n");
		return FPGA_INVALID_PARAM;
	}

	// Read PKG Power limit MSR
	if (readmsr(split_point, MSR_PKG_RAPL_POWER_LIMIT, &msrvalue) != 0) {
		OPAE_ERR("Failed to read MSR.\n");
		result = FPGA_NOT_SUPPORTED;
		return result ;
	}

	pkg_pwr_limit = msrvalue & 0x07fff;
	OPAE_DBG("Power Limit converted: %lx\n", pkg_pwr_limit);

	// Read power units
	if (readmsr(split_point, MSR_RAPL_POWER_UNIT, &msrvalue) != 0) {
		OPAE_ERR("Failed to read MSR.\n");
		result = FPGA_NOT_SUPPORTED;
		return result ;
	}

	power_unit_value = msrvalue & 0x0f;
	power_unit_value = pow(2, power_unit_value);
	OPAE_DBG("power_unit_value :%Lf\n", power_unit_value);

	total_watts = ((double) pkg_pwr_limit) / ((double) power_unit_value);
	OPAE_DBG("Total Watts: %Lf\n", total_watts);

	*pkg_power = total_watts;

	return result;
}

// sets cpu affinity
fpga_result setaffinity(cpu_set_t * idle_set,
			int socket,
			int pid,
			int split_point)
{
	cpu_set_t current_set;
	cpu_set_t full_mask_set;
	int i              = 0;
	fpga_result result = FPGA_OK;

	CPU_ZERO(&current_set);
	CPU_ZERO(&full_mask_set);

	if (idle_set == NULL) {
		OPAE_ERR("Invalid input parm. \n");
		result = FPGA_INVALID_PARAM;
		goto setafy_exit;
	}

	if (sched_getaffinity(pid, sizeof(current_set), &current_set) != 0){

		// PID may not exists in system
		if(pid >2) {
			goto setafy_exit;
		}

		OPAE_ERR("sched_getaffinity failure for pid: %d\n", pid);
		result = FPGA_NOT_SUPPORTED;
		goto setafy_exit;
	}

	for (i = 0; i < split_point; i++) {
		if (socket == 0) {
			CPU_CLR(i, &current_set);
		}else {
			CPU_CLR(i + split_point, &current_set);
		}
	}

	CPU_OR(&full_mask_set, &current_set, idle_set);

	if (sched_setaffinity(pid, sizeof(full_mask_set), &full_mask_set) != 0){

		if(pid >2) {
			goto setafy_exit;
		}

		OPAE_ERR("sched_setaffinity failure for pid: %d\n", pid);
		result = FPGA_NOT_SUPPORTED;
		goto setafy_exit;
	}

setafy_exit:
	return result;
}
// set threads's CPU affinity
fpga_result cpuset_setaffinity(int socket,
				int split_point,
				uint64_t max_cpu_count)
{
	cpu_set_t idle_set;
	uint64_t i                     = 0;
	uint64_t pid                   = 0;
	uint64_t max_pid_index         = 0;
	fpga_result result             = FPGA_OK;


	// Set affinity for pid 1 and pid 2, first.
	// All children of pids created after these call inherent affinity.
	// Pids that already exist will get affinity from max_pid loop.
	//
	// Clear all bits in cpu affinity mask per chosen socket.
	// Next step after this one will be to set full mask for both
	// sockets by idle mask calculated above with
	// the remainder of the mask from this step.


	// set Idle CPU set
	CPU_ZERO(&idle_set);

	if (socket == 0) {
		for (i = 0; i < max_cpu_count; i++) {
		CPU_SET(i, &idle_set);
		}
	} else if (socket == 1) {
		for (i = 0; i < max_cpu_count; i++) {
			CPU_SET(i + split_point, &idle_set);
		}
	} else {
		OPAE_ERR("Invalid socket id\n");
		result = FPGA_NOT_SUPPORTED;
		return result ;
	}

	i = CPU_COUNT_S(sizeof(cpu_set_t), &idle_set);
	OPAE_DBG("CPU_COUNT_S : %d\n", i);

	// Get affinity of pid 1
	// Change CPU set
	// Set affinity of pid 1
	result = setaffinity(&idle_set,socket,1,split_point);
	if (result != FPGA_OK) {
		OPAE_ERR(" sched_setaffinity failure for pid: 1\n");
		return result;
	}

	// Get affinity of pid 2
	// Change CPU set
	// Set affinity of pid 2
	result = setaffinity(&idle_set,socket,2,split_point);
	if (result != FPGA_OK) {
		OPAE_ERR(" sched_setaffinity failure for pid: 1\n");
		return result;
	}

	// Find max pid number
	result = sysfs_read_u64(SYFS_PID_MAX_PATH, &max_pid_index);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed  to read max pid count.\n");
		return result;
	}

	OPAE_DBG("max_pid_index: %d\n",max_pid_index);

	// Get affinity from pid 3 to Max pid count
	// Change CPU set
	// Set affinity of pid 3 to Max pid count

	//  Set affinity for all possible pids to mask in cpuset.
	// App cannot set cpu set for process like kworker,ksoftirqd,watchdog etc
	OPAE_DBG("Set affinity for all possible pids to mask in cpuset ");

	for (pid = 3; pid < max_pid_index; pid++) {

		setaffinity(&idle_set,socket,pid,split_point);
	}

	return result;
}
