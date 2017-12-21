ASE Windows support:

Windows System Requirements:  
 
	- Simualtor: Modelsim	 
		- Tested with Modelsim SE 10.6b (64 bit) 
		- Download link: https://support.mentor.com/en/product/852852093/downloads (Sign up for https://support.mentor.com) 
		- Download gcc for windows along with modelsim 
		- Add the LICENSE server path using the specified environment variables below  
 
	- Quartus Pro 
		- Tested with Quartus Pro 16 
		- Download link: http://dl.altera.com/16.0/ (Sign up) 
		- Do a complete download 
		- Ignore the license server issues with Quartus pro. Add the appropriate environment variables mentioned below 
		- Python Versions 2.7 and 3.3 for running generate_ase_environment.py
		
	- Environment Variables: 
 
		ALTERAOCLSDKROOT C:\altera_pro\16.0\hld 
		LM_LICENSE_FILE  1717@mentor04p.elic.intel.com 
		LM_PROJECT ATP-PLAT-DEV 
		MGLS_LICENSE_FILE 1717@mentor04p.elic.intel.com 
		MTI_HOME C:\modeltech64_10.6b 
		QSYS_ROOTDIR C:\altera_pro\16.0\quartus\sopc_builder\bin 
		QUARTUS_64BIT 1 QUARTUS_HOME C:\altera_pro\16.0\quartus 
		QUARTUS_ROOTDIR %QUARTUS_HOME% 
		QUARTUS_ROOTDIR_OVE RRIDE %QUARTUS_HOME% 
		SNPSLMD_LICENSE_FILE 26586@plxs0402.pdx.intel.com:26586@plxs0405.pdx.intel.com:26586@ plxs0406.pdx.intel.com:26586@plxs0414.pdx.intel.com:26586@plxs041 5.pdx.intel.com:26586@plxs0416.pdx.intel.com:26586@plxs0418.pdx.in tel.com 
		PATH C:\modeltech64_10.6b\win64 C:\altera_pro\16.0\modelsim_ase\win32aloem  %QUARTUS_HOME%\bin64 C:\modeltech64_10.6b\gcc-4.5.0mingw64vc12\bin 
		 
Steps to run ASE on Windows
	Two terminals: one for simulator process and the other one for application process. 
	1). Go to ASE_folder\ase, run scripts\generate_ase_environment.py from 1st terminal:	   
	   scripts\generate_ase_environment.py sample_config/intg_xeon_nlb/rtl -t QUESTA -p intg_xeon 
	   This generates ase_sources.bat and vlog_files.list.
	   TODO: this python script doesn't work on Windows yet. Need to enable it soon. 
	 
	2). ASE_folder\ase\server.bat script is used to start the server 
		a. server clean : deletes files for new session 
		b. server compile : compiles c files and sv files of the server 
		c. server sim : starts simulation 
	 
	3). Run ASE_folder\ase\client.bat from 2nd terminal : script for the client side and the application 
		a. At ase folder, run client.bat to compile client
		b. Set environment variable: set ASE_WORKDIR=C:\ase_Windows\ase
		c. Goto ase\work folder, run hello_fpga.exe	