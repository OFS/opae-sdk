#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/enum.h>
#include <opae/access.h>
#include <opae/utils.h>
#include <poll.h>
#include <errno.h>
#include <opae/fpga.h>

#define MAX_USR_INTRS            4

#define HELLO_AFU_ID              "850ADCC2-6CEB-4B22-9722-D43375B61C66"
#define INTR_REG                 0XA0 //0x28
#define USR_INTR_ID_REG          0XC0 //0x30

static int s_error_count = 0;

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc) \
   do { \
      if ((res) != FPGA_OK) { \
         print_err((desc), (res));  \
         s_error_count += 1; \
         goto label; \
      } \
   } while (0)

/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ASSERT_GOTO(condition, label, desc) \
   do { \
      if (condition == 0) { \
         fprintf(stderr, "Error %s\n", desc); \
         s_error_count += 1; \
         goto label; \
      } \
   } while (0)
      
void print_err(const char *s, fpga_result res)
{
   fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

int main(int argc, char *argv[])
{
   fpga_properties    filter = NULL;
   fpga_token         afc_token;
   fpga_handle        afc_handle;
   fpga_guid          guid;
   uint32_t           num_matches;

   fpga_result     res = FPGA_OK;

   if (uuid_parse(HELLO_AFU_ID, guid) < 0) {
      fprintf(stderr, "Error parsing guid '%s'\n", HELLO_AFU_ID);
      goto out_exit;
   }

   /* Look for AFC with MY_AFC_ID */
   res = fpgaGetProperties(NULL, &filter);
   ON_ERR_GOTO(res, out_exit, "creating properties object");

   res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
   ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

   res = fpgaPropertiesSetGUID(filter, guid);
   ON_ERR_GOTO(res, out_destroy_prop, "setting GUID");

   res = fpgaEnumerate(&filter, 1, &afc_token, 1, &num_matches);
   ON_ERR_GOTO(res, out_destroy_prop, "enumerating AFCs");

   if (num_matches < 1) {
      fprintf(stderr, "AFC not found.\n");
      res = fpgaDestroyProperties(&filter);
      return FPGA_INVALID_PARAM;
   }

   /* Open AFC and map MMIO */
   res = fpgaOpen(afc_token, &afc_handle, 0);
   ON_ERR_GOTO(res, out_destroy_tok, "opening AFC");

   res = fpgaMapMMIO(afc_handle, 0, NULL);
   ON_ERR_GOTO(res, out_close, "mapping MMIO space");

   printf("Running Test\n");

   /* Reset AFC */
   res = fpgaReset(afc_handle);
   ON_ERR_GOTO(res, out_unmap, "resetting AFC");
      
   struct pollfd pfd;
   
   /* Create event */
   fpga_event_handle ehandle[MAX_USR_INTRS];
   for(uint64_t usr_intr_id = 0; usr_intr_id < MAX_USR_INTRS; usr_intr_id++) {
      res = fpgaCreateEventHandle(&ehandle[usr_intr_id]);
      ON_ERR_GOTO(res, out_destroy_handles, "error creating event handle`");

      /* Register user interrupt with event handle */
      res = fpgaRegisterEvent(afc_handle, FPGA_EVENT_INTERRUPT, ehandle[usr_intr_id], usr_intr_id);
      ON_ERR_GOTO(res, out_destroy_handles, "error registering event");
   }
   
   // Test if we can trigger an interrupt for each user interrupt ID
   for(uint64_t usr_intr_id = 0; usr_intr_id < MAX_USR_INTRS; usr_intr_id++) {
      /* Program the user interrupt id register */
      printf("Setting user interrupt id register (Byte Offset=%08x) = %08lx\n", USR_INTR_ID_REG, usr_intr_id);
      res = fpgaWriteMMIO64(afc_handle, 0, USR_INTR_ID_REG, usr_intr_id);
      ON_ERR_GOTO(res, out_destroy_handles, "writing to USR_INTR_ID_REG MMIO");

      /* Trigger interrupt by writing to INTR_REG */
      printf("Setting Interrupt register (Byte Offset=%08x) = %08x\n", INTR_REG, 1);
      res = fpgaWriteMMIO64(afc_handle, 0, INTR_REG, 1);
      ON_ERR_GOTO(res, out_destroy_handles, "writing to INTR_REG MMIO");
   
      /* Poll event handle*/
      res = fpgaGetOSObjectFromEventHandle(ehandle[usr_intr_id], &pfd.fd);
      ON_ERR_GOTO(res, out_destroy_handles, "getting file descriptor");

      pfd.events = POLLIN;            
      int poll_res;
      poll_res = poll(&pfd, 1, -1);
      if(poll_res < 0) {
         fprintf( stderr, "Poll error errno = %s\n",strerror(errno));
         s_error_count += 1;
      } 
      else if(poll_res == 0) {
         fprintf( stderr, "Poll timeout \n");
         s_error_count += 1;
      } else {
         printf("Poll success. Return = %d\n",poll_res);
         uint64_t count;
         ssize_t bytes_read = read(pfd.fd, &count, sizeof(count));          
         if(bytes_read <= 0) {
            fprintf( stderr, "read error: %s\n", bytes_read < 0 ?
               strerror(errno) : "zero bytes read");
         }
      }
   }
   /* cleanup */
   for(uint64_t usr_intr_id = 0; usr_intr_id < MAX_USR_INTRS; usr_intr_id++) {
      res = fpgaUnregisterEvent(afc_handle, FPGA_EVENT_INTERRUPT, ehandle[usr_intr_id]);   
      ON_ERR_GOTO(res, out_destroy_handles, "error fpgaUnregisterEvent");
   }

   printf("Done Running Test\n");

/*destroy event handles*/
out_destroy_handles:
   for(uint64_t usr_intr_id = 0; usr_intr_id < MAX_USR_INTRS; usr_intr_id++) {
      res = fpgaDestroyEventHandle(&ehandle[usr_intr_id]);
      ON_ERR_GOTO(res, out_unmap, "error fpgaDestroyEventHandle");
   }
   
   /* Unmap MMIO space */
out_unmap:
   res = fpgaUnmapMMIO(afc_handle, 0);
   ON_ERR_GOTO(res, out_close, "unmapping MMIO space");
   
   /* Release accelerator */
out_close:
   res = fpgaClose(afc_handle);
   ON_ERR_GOTO(res, out_destroy_tok, "closing AFC");

   /* Destroy token */
out_destroy_tok:
#ifndef USE_ASE
   res = fpgaDestroyToken(&afc_token);
   ON_ERR_GOTO(res, out_destroy_prop, "destroying token");
#endif

   /* Destroy properties object */
out_destroy_prop:
   res = fpgaDestroyProperties(&filter);
   ON_ERR_GOTO(res, out_exit, "destroying properties object");

out_exit:
   if(s_error_count > 0)
      printf("Test FAILED!\n");

   return s_error_count;

}
