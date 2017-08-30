// Copyright(c) 2014-2017, Intel Corporation
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
// **************************************************************************

#include "ase_common.h"


/*
 * Generate seed
 */
uint32_t generate_ase_seed(void)
{
	uint32_t seed;

	seed = (uint32_t) time(NULL);
	seed = seed & 0x0000FFFF;

	return seed;
}


/*
 * Write simulation seed to file
 */
void ase_write_seed(uint32_t seed)
{
	FILE *fp_seed = (FILE *) NULL;

	// Open seed file
	fp_seed = fopen(ASE_SEED_FILE, "w");

	// Use no more than 31-bits of seed
	seed = seed & 0x0000FFFF;

	// Write to file
	if (fp_seed == NULL) {
		ASE_ERR("ASE Seed file could not be written\n");
		ase_error_report("fopen", errno, ASE_OS_FOPEN_ERR);
	} else {
		fprintf(fp_seed, "%u", seed);
		fclose(fp_seed);
	}
}


/*
 * Readback simulation seed - used if ENABLE_REUSE_SEED is enabled
 */
uint32_t ase_read_seed(void)
{
	FILE *fp_seed = (FILE *) NULL;
	uint32_t new_seed;
	uint32_t readback_seed;

	// Check if file already exists (FALSE)
	if (access(ASE_SEED_FILE, F_OK) == -1) {
		ASE_ERR("ASE Seed file could not be read\n");
		ASE_ERR("Old seed unusable --- creating a new seed\n");

		// Generate seed
		new_seed = generate_ase_seed();

		// Write seed to file
		ase_write_seed(new_seed);

		// Return seed
		return new_seed;
	}
	// If TRUE, read seed file
	else {
		// Open file (known to exist)
		fp_seed = fopen(ASE_SEED_FILE, "r");
		if (fp_seed == NULL) {
			ASE_ERR
			    ("ASE Seed file could not be read (NULL seed fileptr) \n");
			ASE_ERR
			    ("Old seed unusable --- creating a new seed\n");

			// Generate seed
			new_seed = generate_ase_seed();

			// Write seed to file
			ase_write_seed(new_seed);

			// Return seed
			return new_seed;
		} else {
			// Read conents, post on log, close, return
			if (fscanf(fp_seed, "%u", &readback_seed) <= 0) {
				ASE_ERR("Seed readback failed !\n");
			}
			// Close seed file
			fclose(fp_seed);

			// Return seed
			return readback_seed;
		}
	}
}


/*
 * Generate 64-bit random number
 */
uint64_t ase_rand64(void)
{
	uint64_t random;
	random = rand();
	random = (random << 32) | rand();
	return random;
}

/*
 * Shuffle an array of numbers
 * USAGE: For setting up latency_scoreboard
 */
void shuffle_int_array(int *array, int num_items)
{
	int i, j;
	int tmp;

	if (num_items > 1) {
		for (i = 0; i < num_items - 1; i++) {
			j = i + rand() / (RAND_MAX / (num_items - i) + 1);
			tmp = array[j];
			array[j] = array[i];
			array[i] = tmp;
		}
	}
}
