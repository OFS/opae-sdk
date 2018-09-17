//
// Internal parameters and structs for ccip_avmm
//

package ccip_avmm_pkg;

	//18bit byte address for avalon, 16bit word address for ccip
	parameter CCIP_AVMM_MMIO_ADDR_WIDTH = 18;
	parameter CCIP_AVMM_MMIO_DATA_WIDTH = 64;

  // MMIO struct as packet comes off CCI-P
	typedef struct packed { 
		logic is_read;
		logic is_32bit;
		logic [CCIP_AVMM_MMIO_ADDR_WIDTH-1:0] addr;
		logic [CCIP_AVMM_MMIO_DATA_WIDTH-1:0] write_data;
	} t_ccip_avmm_mmio_cmd;

	parameter CCIP_AVMM_REQUESTOR_DATA_WIDTH = 512;
	parameter CCIP_AVMM_REQUESTOR_WR_ADDR_WIDTH = 49;
	parameter CCIP_AVMM_REQUESTOR_RD_ADDR_WIDTH = 48;
	parameter CCIP_AVMM_REQUESTOR_BURST_WIDTH = 3;
	parameter CCIP_AVMM_REQUESTOR_CONTROL_WIDTH = 1;
	
	parameter CCIP_AVMM_REQUESTOR_ID_BITS = 2;
	parameter CCIP_AVMM_NUM_INTERRUPT_LINES = 4;
  
  // Avalon-MM struct used to format fields in the command queue that are exposed to NoC
  typedef struct packed {
    logic [CCIP_AVMM_MMIO_ADDR_WIDTH-1:0] address;
    logic [(CCIP_AVMM_MMIO_DATA_WIDTH/8)-1:0] byteenable;
    logic [CCIP_AVMM_MMIO_DATA_WIDTH-1:0] writedata;
    logic read;
    logic write;
  } t_master_cmd_queue;

endpackage // ccip_feature_list_pkg
