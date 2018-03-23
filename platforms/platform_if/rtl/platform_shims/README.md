# Platform Shims

Shims here are invoked by the top-level code that manages the partial reconfiguration
boundary.  Instead of directly instantiating an AFU (e.g. ccip_std_afu), these
intermediate shims will be mapped first.  The shims transform the interface and then
connect to the AFU.  **These shims and sub-directories are internal to the platform
interface manager.  Their interfaces or semantics may change from release to release.**

Shims names are defined in the JSON field *platform-shim-module-name* in
AFU top-level interface descriptions: [afu\_top\_ifc\_db](../../../afu_top_ifc_db/).

Shims typically offer automatic clock crossing and automatic platform-specific
register stage insertion to aid in timing closure.  See the various instances of
*clock* and *add-timing-reg-stages* in
[platform\_defaults.json](../../../platform_db/platform_defaults.json).

Some modules here are sub-shims, instantiated by larger shims.  For example,
platform\_shim\_ccip() manages only CCI-P clock crossing and register stage insertion.
It is instantiated by platform\_shim\_ccip\_std\_afu().
