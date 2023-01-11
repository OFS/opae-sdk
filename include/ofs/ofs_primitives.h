// Copyright(c) 2021-2023, Intel Corporation
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
#ifndef OFS_PRIMITIVES_H
#define OFS_PRIMITIVES_H

#include <stdint.h>
#include <errno.h>
#include <time.h>

#define SEC2NSEC 1000000000
#define SEC2USEC 1000000
#define SEC2MSEC 1000
#define USEC2NSEC 1000
#define NSEC2SEC 1E-9
#define USEC2SEC 1E-6
#define MSEC2SEC 1E-3
#define NSEC2USEC 1E-3

#define OFS_TIMESPEC_USEC(_ts, _usec)                           \
  struct timespec _ts = {                                       \
    .tv_sec = (long)(_usec*USEC2SEC),                           \
    .tv_nsec = _usec*USEC2NSEC-(long)(_usec*USEC2SEC)*SEC2NSEC  \
  }

#define OFS_WAIT_FOR_EQ(_bit, _value, _timeout_usec, _sleep_usec)           \
({                                                                          \
	int _status = 0;                                                    \
	OFS_TIMESPEC_USEC(ts, _sleep_usec);                                 \
	struct timespec begin, now, save, rem;                              \
	save = ts;                                                          \
	clock_gettime(CLOCK_MONOTONIC, &begin);                             \
	while(_bit != _value) {                                             \
		if (_sleep_usec) {                                          \
			ts = save;                                          \
			while ((nanosleep(&ts, &rem) == -1) &&              \
			       (errno == EINTR))                            \
				ts = rem;                                   \
		}                                                           \
		clock_gettime(CLOCK_MONOTONIC, &now);                       \
		struct timespec delta;                                      \
		ofs_diff_timespec(&delta, &now, &begin);                    \
		uint64_t delta_nsec = delta.tv_nsec + delta.tv_sec*SEC2NSEC;\
		if (delta_nsec > _timeout_usec*USEC2NSEC) {                 \
			_status = 1;                                        \
			break;                                              \
		}                                                           \
	}                                                                   \
	_status;                                                            \
})                                                                          \

#define OFS_WAIT_FOR_NE(_bit, _value, _timeout_usec, _sleep_usec)           \
({                                                                          \
	int _status = 0;                                                    \
	OFS_TIMESPEC_USEC(ts, _sleep_usec);                                 \
	struct timespec begin, now, save, rem;                              \
	save = ts;                                                          \
	clock_gettime(CLOCK_MONOTONIC, &begin);                             \
	while(_bit == _value) {                                             \
		if (_sleep_usec) {                                          \
			ts = save;                                          \
			while ((nanosleep(&ts, &rem) == -1) &&              \
			       (errno == EINTR))                            \
				ts = rem;                                   \
		}                                                           \
		clock_gettime(CLOCK_MONOTONIC, &now);                       \
		struct timespec delta;                                      \
		ofs_diff_timespec(&delta, &now, &begin);                    \
		uint64_t delta_nsec = delta.tv_nsec + delta.tv_sec*SEC2NSEC;\
		if (delta_nsec > _timeout_usec*USEC2NSEC) {                 \
			_status = 1;                                        \
			break;                                              \
		}                                                           \
	}                                                                   \
	_status;                                                            \
})                                                                          \

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Get timespec difference
 *
 *  Given two timespec structures, calculate their difference as a timespec
 *  structure.
 *
 *  This takes care of cases when a simple subtraction of nanosecond portion
 *  might result in an overflow (per guidance offered in gnu libc
 *  documentation:
 *  https://www.gnu.org/software/libc/manual/html_node/Calculating-Elapsed-Time.html
 *
 *  @param[out] result Result of subraction as a 'struct timespec' structure
 *  @param[in] lhs     Left hand side of operation
 *  @param[in] rhs     Right hand side of operation
 *  @returns 0 if lhs > rhs, 1 otherwise
 */
inline int ofs_diff_timespec(struct timespec *result,
			     struct timespec *lhs, struct timespec *rhs)
{

	long rhs_sec = rhs->tv_sec;
	long rhs_nsec = rhs->tv_nsec;
	if (lhs->tv_nsec < rhs_nsec) {
		int sec = (rhs_nsec - lhs->tv_nsec)*NSEC2SEC + 1;
		rhs_nsec -= sec * SEC2NSEC;
		rhs_sec += sec;
	}

	if (lhs->tv_nsec - rhs_nsec > SEC2NSEC) {
		int sec = (lhs->tv_nsec - rhs_nsec) * NSEC2SEC;
		rhs_nsec += sec * SEC2NSEC;
		rhs_sec -= sec;
	}

	result->tv_sec = lhs->tv_sec - rhs_sec;
	result->tv_nsec = lhs->tv_nsec - rhs_nsec;

	return lhs->tv_sec < rhs_sec;
}


/**
 *  Wait for a 32-bit variable to equal a given value
 *
 *  Helper function to poll on a variable given its pointer.
 *  This is helpful for pointers to MMIO registers that can be
 *  changed by hardware they are mapped to.
 *
 *  @param[in] var          Pointer to a variable that may change
 *  @param[in] value        Value to compare to var
 *  @param[in] timeout_usec Timeout value in usec
 *  @param[in] sleep_usec   Time (in usec) to sleep while waiting
 *  @returns 0 if variable changed to value while waiting, 1 otherwise
 */
inline int ofs_wait_for_eq32(volatile uint32_t *var, uint32_t value,
			     uint64_t timeout_usec, uint32_t sleep_usec)
{
	OFS_TIMESPEC_USEC(ts, sleep_usec);
	struct timespec begin, now, save, rem;
	save = ts;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	while(*var != value) {
		if (sleep_usec) {
			ts = save;
			while((nanosleep(&ts, &rem) == -1) &&
			      (errno == EINTR))
				ts = rem;
		}
		clock_gettime(CLOCK_MONOTONIC, &now);
		struct timespec delta;
		ofs_diff_timespec(&delta, &now, &begin);
  		uint64_t delta_nsec = delta.tv_nsec + delta.tv_sec*SEC2NSEC;
		if (delta_nsec > timeout_usec*USEC2NSEC) {
			return 1;
		}
	}
	return 0;
}

/**
 *  Wait for a 64-bit variable to equal a given value
 *
 *  Helper function to poll on a variable given its pointer.
 *  This is helpful for pointers to MMIO registers that can be
 *  changed by hardware they are mapped to.
 *
 *  @param[in] var          Pointer to a variable that may change
 *  @param[in] value        Value to compare to var
 *  @param[in] timeout_usec Timeout value in usec
 *  @param[in] sleep_usec   Time (in usec) to sleep while waiting
 *  @returns 0 if variable changed to value while waiting, 1 otherwise
 */
inline int ofs_wait_for_eq64(volatile uint64_t *var, uint64_t value,
			     uint64_t timeout_usec, uint32_t sleep_usec)
{
	OFS_TIMESPEC_USEC(ts, sleep_usec);
	struct timespec begin, now, save, rem;
	save = ts;
	clock_gettime(CLOCK_MONOTONIC, &begin);
	while(*var != value) {
		if (sleep_usec) {
			ts = save;
			while ((nanosleep(&ts, &rem) == -1) &&
			       (errno == EINTR))
				ts = rem;
		}
		clock_gettime(CLOCK_MONOTONIC, &now);
		struct timespec delta;
		ofs_diff_timespec(&delta, &now, &begin);
  		uint64_t delta_nsec = delta.tv_nsec + delta.tv_sec*SEC2NSEC;
		if (delta_nsec > timeout_usec*USEC2NSEC) {
			return 1;
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* !OFS_PRIMITIVES_H */
