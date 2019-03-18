# Feature Plugin Developer's Guide #

```eval_rst
.. toctree::
```

## Overview ##

Beginning with OPAE C library version 1.2.0, OPAE implements a plugin-centric
model. Feature API plugin is a plugin under OPAE's xfpga plgins. This guide serves
as a reference to define the makeup of an OPAE C Feature API plugin and to describe
a sequence of steps that one may follow when constructing an OPAE C Feature API
plugin.

## Feature Plugin Required Functions ##

An OPAE C Feature API plugin is a runtime-loadable shared object library, also known
as a module. On Linux systems, the *dl* family of APIs from libdl are used to
interact with shared objects. Refer to "man dlopen" and "man dlsym" for examples
of using the libdl API.

An OPAE C Feature API plugin implements one required function. This function is
required to have C linkage, so that its name is not mangled.

```c
    int feature_plugin_configure(feature_adapter_table *table, const char *config);
```

During initialization, the feature plugin manager component loads each feature
plugin, searching for its `feature_plugin_configure` function. If none is found,
then the feature plugin manager rejects that plugin. When it is found,
`feature_plugin_configure` is called passing a pointer to a freshly-created
`feature_adapter_table` and a buffer consisting of configuration data for the plugin.

The job of the `feature_plugin_configure` function is to populate the given adapter
table with each of the plugin's API entry points and to consume and comprehend
the given configuration data in preparation for initialization.

## Feature API Adapter Table ##

The feature adapter table is a data structure that contains function pointer entry
points for each of the Feature APIs implemented by a plugin. In this way, it adapts
the plugin-specific behavior to the more general case of a C API in xfpga plugin.
Note that OPAE applications are only required to link with opae-c. In other words, the
name of the plugin library should not appear on the linker command line. In this
way, plugins are truly decoupled from the xfpga API, and they are required to
adapt to the strict API specification by populating the adapter table only. No
other linkage is required nor recommended.

`feature_adapter.h` contains the definition of the `feature_adapter_table`. An
abbreviated version is depicted below, along with supporting type `opae_plugin`:

```c
    typedef struct _feature_plugin {
        char *path;
        void *dl_handle;
    } feature_plugin;

    typedef struct _feature_adapter_table {

        struct _feature_adapater_table *next;
        feature_plugin plugin;
        fpga_guid guid;

        fpga_result(*fpgaDMAPropertiesGet)(fpga_feature_token token,
                        fpga_dma_properties *prop);

        fpga_result (*fpgaDMATransferSync)(fpga_feature_handle dma_handle,
                        dma_transfer_list *dma_xfer);

        fpga_result (*fpgaDMATransferAsync)(fpga_feature_handle dma,
                        dma_transfer_list *dma_xfer, fpga_dma_cb cb, void *context);

        fpga_result (*fpgaFeatureOpen)(fpga_feature_token token, int flags,
                        void *priv_config, fpga_feature_handle *handle);

        fpga_result (*fpgaFeatureClose)(fpga_feature_handle *_dma_h);

        // configuration functions
        int (*initialize)(void);
        int (*finalize)(void);

    } feature_adapter_table;
```

Some points worth noting are that the feature adapter tables are organized in memory by
adding them to a linked list data structure. This is the use of the `next`
structure member. (The list management is handled by the feature plugin manager.)
The `plugin` structure member contains the handle to the shared object instance,
as created by `dlopen`. This handle is used in the plugin's `feature_plugin_configure`
to load plugin entry points. A plugin need only implement the portion of the
OPAE C Feature API that a target application needs. Any API entry points that are not
supported should be left as NULL pointers (the default) in the adapter table.
When an OPAE Feature API that has no associated entry point in the adapter table is
called, the result for objects associated with that plugin will be
`FPGA_NOT_SUPPORTED`.

The following code illustrates a portion of the `feature_plugin_configure` for
a theoretical OPAE C Feature API plugin libfeatfoo.so:

```c
    /* foofeat_plugin.c */

    int feature_plugin_configure(feature_adapter_table *adapter, const char *config)
    {
        adapter->fpgaFeatureOpen =
                dlsym(adapter->plugin.dl_handle, "foo_featureOpen");
        adapter->fpgaFeatureClose =
                dlsym(adapter->plugin.dl_handle, "foo_featureClose");

        ...

        adapter->fpgaDMAPropertiesGet =
                dlsym(adapter->plugin.dl_handle, "foo_propertiesGet");

        ...

        return 0;
    }
```

Notice that the implementations of the API entry points for plugin libfoofeat.so
are prefixed with `foo_`. This is the recommended practice to avoid name
collisions and to enhance the debugability of the application. Upon successful
configuration, `feature_plugin_configure` returns 0 to indicate success. A
non-zero return value indicates failure and causes the feature plugin manager to
reject the plugin from futher consideration.

## Feature Plugin Optional Functions ##

Once the plugin manager loads and configures each plugin, it uses the adapter
table to call back into the plugin so that it can be made ready for runtime.
This is the job of the `feature_plugin_initialize` entry point, whose signature
is defined as:

```c
    int feature_plugin_initialize(void)
```

The function takes fpga_handle as parameter. `feature_plugin_initialize` returns 0
if no errors were encountered during initialization. A non-zero return code
indicates that feature plugin initialization failed. A plugin makes its
`feature_plugin_initialize` available to the feature plugin manager by populating the
feature adapter table's `initialize` entry point as shown:

```c
    /* foofeat_plugin.c */

    int foo_plugin_initialize(void)
    {
        ...

        return 0; /* success */
    }

    int feature_plugin_configure(feature_adapter_table *adapter, const char *config)
    {
        ... 

        adapter->initialize =
                dlsym(adapter->plugin.dl_handle, "foo_plugin_initialize");

        ...

        return 0;
    }
```

If a plugin does not implement an `feature_plugin_initialize` entry point, then
the `initialize` member of the adapter table should be left uninitialized.
During feature plugin initialization, if a plugin has no `feature_plugin_initialize`
entry in its adapter table, the plugin initialization step will be skipped,
and the plugin will be considered to have initialized successfully.

Once feature plugin initialization is complete for all loaded plugins, the system
is considered to be running and fully functional.

During teardown, the feature plugin manager uses the feature adapter table to call into each
plugin's `feature_plugin_finalize` entry point, whose signature is defined as:

```c
    int feature_plugin_finalize(void);
```

`feature_plugin_finalize` returns 0 if no errors were encountered during teardown.
A non-zero return code indicates that feature plugin teardown failed. A plugin makes
its `feature_plugin_finalize` available to the plugin manager by populating the
adapter table's `finalize` entry point as shown:

```c
    /* featfoo_plugin.c */

    int foo_plugin_finalize(void)
    {
        ...

        return 0; /* success */
    }

    int opae_plugin_configure(opae_api_adapter_table *table, const char *config)
    {
        ... 

        adapter->finalize =
                dlsym(adapter->plugin.dl_handle, "foo_plugin_finalize");

        ...

        return 0;
    }
```

If a plugin does not implement an `feature_plugin_finalize` entry point, then
the `finalize` member of the adapter table should be left uninitialized.
During plugin cleanup, if a plugin has no `feature_plugin_finalize` entry
point in its adapter table, the plugin finalize step will be skipped, and
the plugin will be considered to have finalized successfully.

## Feature Plugin Construction ##

The steps required to implement an OPAE C Feature API plugin, libfeatfoo.so, are:

* Create foofeat\_plugin.c: implements `feature_plugin_configure`,
`feature_plugin_initialize` and `feature_plugin_finalize` as described in the previous sections.
* Create foofeat\_plugin.h: implements function prototypes for each of the
feature plugin-specific OPAE C Feature APIs.

```c
    /* foofeat_plugin.h */

    fpga_result foo_fpgaFeatureOpen(fpga_token token, fpga_handle *handle,
                             int flags);

    fpga_result foo_fpgaFeatureClose(fpga_handle handle);

    ...

    fpga_result foo_fpgaFeatureEnumerate(const fpga_properties *filters,
                                  uint32_t num_filters, fpga_token *tokens,
                                  uint32_t max_tokens,
                                  uint32_t *num_matches);
    ...
```

* Create foofeat\_types.h: implements plugin-specific types for opaque data
structures.

```c
    /* foofeat_types.h */

    struct _foo_feature_handle {
        ...
    };

    struct _foo_feature_event_handle {
        ...
    };
```

* Modify feature_pluginmgr.c at line 60: modify the feature id and feature plugin library name