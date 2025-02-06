# Changelog

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [2.14.0-2]

### Added

- Add High Bandwidth Memory (HBM) support to `mem_tg` ([#3149]).

### Fixed

- Fix user clock configuration sequence numbers ([#3156]).
- Fix host exerciser memory calibration check as non-root user ([#3157]).

[2.14.0-2]: https://github.com/OFS/opae-sdk/compare/2.14.0-1...2.14.0-2
[#3149]: https://github.com/OFS/opae-sdk/pull/3149
[#3156]: https://github.com/OFS/opae-sdk/pull/3156
[#3157]: https://github.com/OFS/opae-sdk/pull/3157

## [2.14.0-1]

### Added

- Add tool `qsfpinfo` to print QSFP EEPROM information ([#3119]).

### Fixed

- Support more than 8 memory channels in `mem_tg` ([#3138]).
- Add bridge reset when device is unplugged ([#3143], [#3144]).
- Relax OpenSSL version check in `pacsign` ([#3147]).

[2.14.0-1]: https://github.com/OFS/opae-sdk/compare/2.13.0-3...2.14.0-1
[#3119]: https://github.com/OFS/opae-sdk/pull/3119
[#3138]: https://github.com/OFS/opae-sdk/pull/3138
[#3143]: https://github.com/OFS/opae-sdk/pull/3143
[#3144]: https://github.com/OFS/opae-sdk/pull/3144
[#3147]: https://github.com/OFS/opae-sdk/pull/3147

## [2.13.0-3]

### Fixed

- Increate hssi poll timeout from 500 us to 1 s ([#3132]).

[2.13.0-3]: https://github.com/OFS/opae-sdk/compare/2.13.0-2...2.13.0-3
[#3132]: https://github.com/OFS/opae-sdk/pull/3132

## [2.13.0-2]

### Added

- Add a mechanism for AFUs to set platform macros ([#3130]).

### Fixed

- Fix buffer comparison in interrupt test mode of host exerciser ([#3131]).

[2.13.0-2]: https://github.com/OFS/opae-sdk/compare/2.13.0-1...2.13.0-2
[#3130]: https://github.com/OFS/opae-sdk/pull/3130
[#3131]: https://github.com/OFS/opae-sdk/pull/3131

## [2.13.0-1]

### Added

- Add support for varying HE LB data bus widths to host exerciser ([#3110]).
- Add board plugin for JTAG-only PCI development kits ([#3120], [#3125]).

### Changed

- Suggest hssiloopback and hssistats for Agilex-based FPGA cards ([#3117]).

### Removed

- Remove unsupported SDM hashes and cancel keys from CMC board module ([#3099]).

### Fixed

- Limit progress bar to 100% in fpgasupdate ([#3104]).
- Fix memory use-after-free of static logger object ([#3116]).
- Do not check EMIF calibration when simulating host exerciser ([#3121]).
- Fix multiplier for board power sensor ([#3122]).
- Correctly document status register value in copy engine ([#3124]).
- Correctly handle local memory width in host exerciser ([#3126]).

[2.13.0-1]: https://github.com/OFS/opae-sdk/compare/2.12.0-1...2.13.0-1
[#3099]: https://github.com/OFS/opae-sdk/pull/3099
[#3104]: https://github.com/OFS/opae-sdk/pull/3104
[#3110]: https://github.com/OFS/opae-sdk/pull/3110
[#3116]: https://github.com/OFS/opae-sdk/pull/3116
[#3117]: https://github.com/OFS/opae-sdk/pull/3117
[#3120]: https://github.com/OFS/opae-sdk/pull/3120
[#3125]: https://github.com/OFS/opae-sdk/pull/3125
[#3121]: https://github.com/OFS/opae-sdk/pull/3121
[#3122]: https://github.com/OFS/opae-sdk/pull/3122
[#3124]: https://github.com/OFS/opae-sdk/pull/3124
[#3126]: https://github.com/OFS/opae-sdk/pull/3126

## [2.12.0-5]

### Fixed

- Bump release number of binary packages to match release tag ([#3114], [#3115]).

[2.12.0-5]: https://github.com/OFS/opae-sdk/compare/2.12.0-4...2.12.0-5
[#3114]: https://github.com/OFS/opae-sdk/pull/3114
[#3115]: https://github.com/OFS/opae-sdk/pull/3115

## [2.12.0-4]

### Fixed

- Support host exerciser on devices without FPGA management PF ([#3108], [#3109]).

[2.12.0-4]: https://github.com/OFS/opae-sdk/compare/2.12.0-3...2.12.0-4
[#3108]: https://github.com/OFS/opae-sdk/pull/3108
[#3109]: https://github.com/OFS/opae-sdk/pull/3109

## [2.12.0-3]

### Fixed

- Set PCI SBDF to enumerate FPGA device and accelerator in afu_test ([#3103], [#3105])

[2.12.0-3]: https://github.com/OFS/opae-sdk/compare/2.12.0-2...2.12.0-3
[#3103]: https://github.com/OFS/opae-sdk/pull/3103
[#3105]: https://github.com/OFS/opae-sdk/pull/3105

## [2.12.0-2]

### Changed

- Improved error message for buffer allocation failure in host exerciser memory test ([#3102]).

### Fixed

- Fix Partial Reconfiguration on boards without BMC interface ([#3101]).

[2.12.0-2]: https://github.com/OFS/opae-sdk/compare/2.12.0-1...2.12.0-2
[#3101]: https://github.com/OFS/opae-sdk/pull/3101
[#3102]: https://github.com/OFS/opae-sdk/pull/3102

## [2.12.0-1]

### Added

- Add loop count command line input to CXL host exerciser ([#3051]).
- Add running pointer and ping-pong tests to CXL host exerciser ([#3056]).
- Add opae-mem tool as a replacement for ofs.uio ([#3055]).
- Add cxl_hello_fpga sample for CMC ([#3052]).
- Add parent/child AFU management to the libopae-c shell ([#3072]).
- Add --force and --enable-sriov options to opae.io ([#3079]).
- Add support for multi-port AFUs in OPAE vfio plugin ([#3080]).

### Changed

- Remove latency iterations from write cache hit/miss scenario tests ([#3043]).
- Support 16GB memory for MEM_TG in cxl_mem_tg sample ([#3058]).
- Remove unsupported loopback command from CXL host exerciser ([#3059]).
- Remove redundant host read and write cache miss tests from CXL host exerciser ([#3064]).
- Improve fpgametrics output readability ([#3075]).
- Replace Bitstream Version with Image Info in bitstreaminfo output ([#3076]).
- Improve VFIO device enumeration performance ([#3090]).
- Hide confusing log message in pacsign ([#3092]).

### Fixed

- Fix CXL host exerciser read latency output ([#3042]).
- Correct issue pointed out by static analysis ([#3044], [#3046]).
- Fix uninitialized variable errors in fpgabist ([#3053]).
- Fix CXL traffic generator read/write bandwidth and argument checks ([#3054]).
- Disallow factory image update if boot page is also factory ([#3049]).
- Set FPGA buffer read-only for device bias mode ([#3057]).
- Define PCI_STD_NUM_BARS when not found ([#3061]).
- Set continuous mode bit for write cache hit/miss tests ([#3062]).
- Skip check for factory boot page in fpgasupdate under certain conditions ([#3063]).
- Fix segmentation fault in CXL host exerciser in non-root user mode ([#3067]).
- Fix running pointer test output in CXL host exerciser ([#3068]).
- Only clear AER errors when AER is available ([#3073]).
- Fix SEGV when passing NULL buf_addr to fpgaPrepareBuffer() ([#3077]).
- Update test completion timeout value in cxl_mem_tg sample ([#3081]).
- Fix progress output in fpgasudpate ([#3078]).
- Update test completion timeout to 10 seconds in cxl_mem_tg sample ([#3086], [#3087]).
- Update latency calculations in CXL host exerciser ([#3088]).
- Check memory calibration status before running host exerciser ([#3094]).
- Fix error handling when vfio-pci driver binding fails in opae.io ([#3095]).

### Security

- Address issues pointed out by Python scanners ([#3036], [#3045]).

[2.12.0-1]: https://github.com/OFS/opae-sdk/compare/2.10.0-1...2.12.0-1
[#3036]: https://github.com/OFS/opae-sdk/pull/3036
[#3042]: https://github.com/OFS/opae-sdk/pull/3042
[#3043]: https://github.com/OFS/opae-sdk/pull/3043
[#3044]: https://github.com/OFS/opae-sdk/pull/3044
[#3045]: https://github.com/OFS/opae-sdk/pull/3045
[#3046]: https://github.com/OFS/opae-sdk/pull/3046
[#3049]: https://github.com/OFS/opae-sdk/pull/3049
[#3051]: https://github.com/OFS/opae-sdk/pull/3051
[#3052]: https://github.com/OFS/opae-sdk/pull/3052
[#3053]: https://github.com/OFS/opae-sdk/pull/3053
[#3054]: https://github.com/OFS/opae-sdk/pull/3054
[#3055]: https://github.com/OFS/opae-sdk/pull/3055
[#3056]: https://github.com/OFS/opae-sdk/pull/3056
[#3057]: https://github.com/OFS/opae-sdk/pull/3057
[#3058]: https://github.com/OFS/opae-sdk/pull/3058
[#3059]: https://github.com/OFS/opae-sdk/pull/3059
[#3061]: https://github.com/OFS/opae-sdk/pull/3061
[#3062]: https://github.com/OFS/opae-sdk/pull/3062
[#3063]: https://github.com/OFS/opae-sdk/pull/3063
[#3064]: https://github.com/OFS/opae-sdk/pull/3064
[#3067]: https://github.com/OFS/opae-sdk/pull/3067
[#3068]: https://github.com/OFS/opae-sdk/pull/3068
[#3072]: https://github.com/OFS/opae-sdk/pull/3072
[#3073]: https://github.com/OFS/opae-sdk/pull/3073
[#3075]: https://github.com/OFS/opae-sdk/pull/3075
[#3076]: https://github.com/OFS/opae-sdk/pull/3076
[#3077]: https://github.com/OFS/opae-sdk/pull/3077
[#3078]: https://github.com/OFS/opae-sdk/pull/3078
[#3079]: https://github.com/OFS/opae-sdk/pull/3079
[#3080]: https://github.com/OFS/opae-sdk/pull/3080
[#3081]: https://github.com/OFS/opae-sdk/pull/3081
[#3086]: https://github.com/OFS/opae-sdk/pull/3086
[#3087]: https://github.com/OFS/opae-sdk/pull/3087
[#3088]: https://github.com/OFS/opae-sdk/pull/3088
[#3090]: https://github.com/OFS/opae-sdk/pull/3090
[#3092]: https://github.com/OFS/opae-sdk/pull/3092
[#3094]: https://github.com/OFS/opae-sdk/pull/3094
[#3095]: https://github.com/OFS/opae-sdk/pull/3095
