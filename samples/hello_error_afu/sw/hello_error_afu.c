#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <poll.h>
#include <errno.h>

int usleep(unsigned);

#define HELLO_ERROR_ID              "5C1EA3C4-2BFB-4E2A-ABF4-13C1E12541B0"
#define MMIO_RD_TIMEOUT_REG      0X100//0x40
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
   fpga_event_handle  port_ehandle;
   fpga_result        res = FPGA_OK;
   uint32_t           num_matches;
   uint64_t           count;
   uint64_t           val = 0;
   pid_t              pid;
   struct pollfd      pfd;
   int timeout = 10000;
   int poll_ret = 0;

   if (uuid_parse(HELLO_ERROR_ID, guid) < 0) {
      fprintf(stderr, "Error parsing guid '%s'\n", HELLO_ERROR_ID);
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
 
   pid = fork();
   if (pid == -1) {
      printf("Could not create a thread to cause port error");
      goto out_unmap;
   } else if (pid == 0) {
      usleep(5000000);
      // Read from MMIO_RD_TIMEOUT register to trigger a Port Error -> Port Error IRQ
      res = fpgaReadMMIO64(afc_handle, 0, MMIO_RD_TIMEOUT_REG, &val);
      ON_ERR_GOTO(res, out_unmap, "doing MMIO read");

      goto out_unmap;
   } else {
      /* Create event */
      res = fpgaCreateEventHandle(&port_ehandle);
      ON_ERR_GOTO(res, out_destroy_handles, "error creating event handle`");

      /* Register user interrupt with event handle */
      res = fpgaRegisterEvent(afc_handle, FPGA_EVENT_ERROR, port_ehandle, 0);
      ON_ERR_GOTO(res, out_destroy_handles, "error registering event");
      
      /* Poll event handle*/
      res = fpgaGetOSObjectFromEventHandle(port_ehandle, &pfd.fd);
      ON_ERR_GOTO(res, out_destroy_handles, "getting file descriptor");

      pfd.events = POLLIN;            
      poll_ret = poll(&pfd, 1, timeout);
      if(poll_ret < 0) {
         fprintf( stderr, "Poll error errno = %s\n",strerror(errno));
         s_error_count += 1;
         res = FPGA_EXCEPTION;
      } 
      else if(poll_ret == 0) {
         fprintf( stderr, "Poll timeout \n");
         s_error_count += 1;
         res = FPGA_EXCEPTION;
      } else {
         printf("Port interrupt occured. Return = %d\n",res);
         ssize_t bytes_read = read(pfd.fd, &count, sizeof(count));
         if(bytes_read <= 0) {
            fprintf( stderr, "read error: %s\n", bytes_read < 0 ?
               strerror(errno) : "zero bytes read");
         }
      }

      /* cleanup */
      res = fpgaUnregisterEvent(afc_handle, FPGA_EVENT_ERROR, port_ehandle);   
      ON_ERR_GOTO(res, out_destroy_handles, "error fpgaUnregisterEvent");

      printf("Done Running Test\n");
   }

/*destroy event handles*/
out_destroy_handles:
   res = fpgaDestroyEventHandle(&port_ehandle);
   ON_ERR_GOTO(res, out_unmap, "error fpgaDestroyEventHandle");
   
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

