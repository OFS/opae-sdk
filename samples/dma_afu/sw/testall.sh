#!/bin/bash

sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -n -y
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -a -y
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -n -y
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -a -y

sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -n -y -C
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -a -y -C
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -n -y -C
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -a -y -C

sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -n -y -M
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -a -y -M
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -n -y -M
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -a -y -M

sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -n -y -C -M
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -a -y -C -M
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -n -y -C -M
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -a -y -C -M

sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -n
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -m -c -a
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -n
sudo LD_LIBRARY_PATH=.:/usr/local/lib perf stat -d -d -d -D 20 ./fpga_dma_test 0 -p -c -a
