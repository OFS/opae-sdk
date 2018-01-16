Platform description databases.

Each JSON database is a dictionary, supporting the following keys:

  version:
    Integer [required]

    Indicates the JSON database version.  Currently only version 1 is supported.

  platform-name: 
    String [required]

    The name of the platform.  Typically, this will match the name of
    the JSON file.

  parent:
    String [optional]

    The name of a parent platform JSON database.  The parent will be loaded
    and fields from the child will be merged into the parent.  For most fields,
    the merge overwrites any existing parent data.  For module-arguments-offered,
    the merge completely overwrites entries with matching class/interface
    pairs.  Keys within a module-arguments-offered class/interface entry aren't
    merged.  The parent's entry is removed and replaced with the child's
    entry.

  module-arguments-offered:
    List [required]

    The set of arguments offered by the platform to AFUs.  Entries in the
    list have the same format as the module-arguments list found in AFU
    top-level interface databases.

    module-arguments-offered differs from the AFU's module-arguments in
    the following ways:

      - For vector classes, max-entries must be defined.

      - There may be multiple instances of the same class.  In this case,
        all instances must be optional.  At most one instance of the
        class will be instantiated, since the AFU is allowed to request
        only one instance of a class.

      - Some classes require configuration parameters.  The full list of
        parameters and their default values are found in
        platform_defaults.json.  Individual platforms may override these
        defaults by setting updated values in a "params" dictionary
        within module-arguments-offered classes.  Setting a parameter's
        value to null will keep it from being defined.

    See ../afu_top_ifc_db/README for both the schema and a list of valid
    class/interface pairs.

---------------------------------------------------------------------------
---------------------------------------------------------------------------

platform_defaults.json is a special database, holding default values for
module argument classes that are configurable.  By convention, all
configuration parameters available for each class should be set in
platform_defaults.json in order to have a central repository of
parameters.  Particular platforms can override these default values
by specifying platform-specific values in a "params" dictionary within
an offered class.  For example, a PCIe-only platform can indicate the
set of supported VCs with:

{
   ...
   "description": "PCIe-only platform",
   ...
   "module-arguments-offered" :
      [
         ...
         {
            "class": "cci-p",
            "interface": "struct",
            "params":
               {
                  "vc-supported": "{ 1, 0, 1, 0 }"
               }
         }
      ]
}

Like other configuration databases, platform parameters are consumed by
the afu_platform_config script.  The script emits parameter values as
preprocessor variables of the form PLATFORM_PARAM_<class>_<param>.
Consistent with the other variables, all characters are converted to
upper case and dashes become underscores.

The platform database is tagged with a version.  This version could be
used in the canonicalizePlatformDefaultsDb function within
afu_platform_config to maintain compatibility with old databases as
interface classes change over time.
