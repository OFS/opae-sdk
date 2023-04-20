//
// Debug output of the Avalon memory interface
//

`ifndef __AVALON_MEM_IF_DBG_VH__
`define __AVALON_MEM_IF_DBG_VH__

    initial
    begin : logger_proc
        if (avalon_mem_if_log_fd == -1)
        begin
            avalon_mem_if_log_fd = $fopen(LOG_NAME, "w");
        end

        // Watch traffic
        if (ENABLE_LOG != 0)
        begin
            forever @(posedge clk)
            begin
                // Read request
                if (! reset && read && ! waitrequest)
                begin
                    $fwrite(avalon_mem_if_log_fd, "%m: %t bank %0d read 0x%x burst 0x%x\n",
                            $time,
                            bank_number,
                            address,
                            burstcount);
                end

                // Read response
                if (! reset && readdatavalid)
                begin
                    $fwrite(avalon_mem_if_log_fd, "%m: %t bank %0d resp 0x%x\n",
                            $time,
                            bank_number,
                            readdata);
                end

                // Write request
                if (! reset && write && ! waitrequest)
                begin
                    $fwrite(avalon_mem_if_log_fd, "%m: %t bank %0d write 0x%x burst 0x%x mask 0x%x data 0x%x\n",
                            $time,
                            bank_number,
                            address,
                            burstcount,
                            byteenable,
                            writedata);
                end
            end
        end
    end

`endif
