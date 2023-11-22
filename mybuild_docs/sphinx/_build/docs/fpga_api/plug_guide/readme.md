# Plugin Developer's Guide #

```eval_rst
.. toctree::
```

## Overview ##

Beginning with OPAE C library version 1.2.0, OPAE implements a plugin-centric
model. This guide serves as a reference to define the makeup of an OPAE C API
plugin and to describe a sequence of steps that one may follow when constructing
an OPAE C API plugin.

## Plugin Required Functions ##

An OPAE C API plugin is a runtime-loadable shared object library, also known as
a module. On Linux systems, the *dl* family of APIs from libdl are used to
interact with shared objects. Refer to "man dlopen" and "man dlsym" for examples
of using the libdl API.

An OPAE C API plugin implements one required function. This function is required
to have C linkage, so that its name is not mangled.

```c
    int opae_plugin_configure(opae_api_adapter_table *table, const char *config);
```

During initialization, the OPAE plugin manager component loads each plugin,
searching for its `opae_plugin_configure` function. If none is found, then
the plugin manager rejects that plugin. When it is found, `opae_plugin_configure`
is called passing a pointer to a freshly-created `opae_api_adapter_table` and
a buffer consisting of configuration data for the plugin.

The job of the `opae_plugin_configure` function is to populate the given adapter
table with each of the plugin's API entry points and to consume and comprehend
the given configuration data in preparation for initialization.

## OPAE API Adapter Table ##

The adapter table is a data structure that contains function pointer entry points
for each of the OPAE APIs implemented by a plugin. In this way, it adapts the
plugin-specific behavior to the more general case of a flat C API. Note that
OPAE applications are only required to link with opae-c. In other words, the
name of the plugin library should not appear on the linker command line. In this
way, plugins are truly decoupled from the OPAE C API, and they are required to
adapt to the strict API specification by populating the adapter table only. No
other linkage is required nor recommended.

`adapter.h` contains the definition of the `opae_api_adapter_table`. An abbreviated
version is depicted below, along with supporting type `opae_plugin`:

```c
    typedef struct _opae_plugin {
        char *path;
        void *dl_handle;
    } opae_plugin;

    typedef struct _opae_api_adapter_table {

        struct _opae_api_adapater_table *next;
        opae_plugin plugin;

        fpga_result (*fpgaOpen)(fpga_token token, fpga_handle *handle,
                                int flags);

        fpga_result (*fpgaClose)(fpga_handle handle);

        ...

        fpga_result (*fpgaEnumerate)(const fpga_properties *filters,
                                     uint32_t num_filters, fpga_token *tokens,
                                     uint32_t max_tokens,
                                     uint32_t *num_matches);

        ...

        // configuration functions
        int (*initialize)(void);
        int (*finalize)(void);

        // first-level query
        bool (*supports_device)(const char *device_type);
        bool (*supports_host)(const char *hostname);

    } opae_api_adapter_table;
```

Some points worth noting are that the adapter tables are organized in memory by
adding them to a linked list data structure. This is the use of the `next`
structure member. (The list management is handled by the plugin manager.)
The `plugin` structure member contains the handle to the shared object instance,
as created by `dlopen`. This handle is used in the plugin's `opae_plugin_configure`
to load plugin entry points. A plugin need only implement the portion of the
OPAE C API that a target application needs. Any API entry points that are not
supported should be left as NULL pointers (the default) in the adapter table.
When an OPAE API that has no associated entry point in the adapter table is
called, the result for objects associated with that plugin will be
`FPGA_NOT_SUPPORTED`.

The following code illustrates a portion of the `opae_plugin_configure` for
a theoretical OPAE C API plugin libfoo.so:

```c
    /* foo_plugin.c */

    int opae_plugin_configure(opae_api_adapter_table *table, const char *config)
    {
        adapter->fpgaOpen = dlsym(adapter->plugin.dl_handle, "foo_fpgaOpen");
        adapter->fpgaClose =
                dlsym(adapter->plugin.dl_handle, "foo_fpgaClose");

        ...

        adapter->fpgaEnumerate =
                dlsym(adapter->plugin.dl_handle, "foo_fpgaEnumerate");

        ...

        return 0;
    }
```

Notice that the implementations of the API entry points for plugin libfoo.so
are prefixed with `foo_`. This is the recommended practice to avoid name
collisions and to enhance the debugability of the application. Upon successful
configuration, `opae_plugin_configure` returns 0 to indicate success. A
non-zero return value indicates failure and causes the plugin manager to
reject the plugin from futher consideration.

## Plugin Optional Functions ##

Once the plugin manager loads and configures each plugin, it uses the adapter
table to call back into the plugin so that it can be made ready for runtime.
This is the job of the `opae_plugin_initialize` entry point, whose signature
is defined as:

```c
    int opae_plugin_initialize(void);
```

The function takes no parameters, as the configuration data was already given
to the plugin by `opae_plugin_configure`. `opae_plugin_initialize` returns 0
if no errors were encountered during initialization. A non-zero return code
indicates that plugin initialization failed. A plugin makes its
`opae_plugin_initialize` available to the plugin manager by populating the
adapter table's `initialize` entry point as shown:

```c
    /* foo_plugin.c */

    int foo_plugin_initialize(void)
    {
        ...

        return 0; /* success */
    }

    int opae_plugin_configure(opae_api_adapter_table *table, const char *config)
    {
        ... 

        adapter->initialize =
                dlsym(adapter->plugin.dl_handle, "foo_plugin_initialize");

        ...

        return 0;
    }
```

If a plugin does not implement an `opae_plugin_initialize` entry point, then
the `initialize` member of the adapter table should be left uninitialized.
During plugin initialization, if a plugin has no `opae_plugin_initialize`
entry in its adapter table, the plugin initialization step will be skipped,
and the plugin will be considered to have initialized successfully.

Once plugin initialization is complete for all loaded plugins, the system
is considered to be running and fully functional.

During teardown, the plugin manager uses the adapter table to call into each
plugin's `opae_plugin_finalize` entry point, whose signature is defined as:

```c
    int opae_plugin_finalize(void);
```

`opae_plugin_finalize` returns 0 if no errors were encountered during teardown.
A non-zero return code indicates that plugin teardown failed. A plugin makes
its `opae_plugin_finalize` available to the plugin manager by populating the
adapter table's `finalize` entry point as shown:

```c
    /* foo_plugin.c */

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

If a plugin does not implement an `opae_plugin_finalize` entry point, then
the `finalize` member of the adapter table should be left uninitialized.
During plugin cleanup, if a plugin has no `opae_plugin_finalize` entry
point in its adapter table, the plugin finalize step will be skipped, and
the plugin will be considered to have finalized successfully.

In addition to `initialize` and `finalize`, an OPAE C API plugin has two
further optional entry points that relate to device enumeration. During
enumeration, when a plugin is being considered for a type of device, the
plugin may provide input on that decision by exporting an
`opae_plugin_supports_device` entry point in the adapter table:

```c
    bool opae_plugin_supports_device(const char *device_type);
```

`opae_plugin_supports_device` returns true if the given device type is
supported and false if it is not. A false return value from
`opae_plugin_supports_device` causes device enumeration to skip the
plugin.

Populating the `opae_plugin_supports_device` is done as:

```c
    /* foo_plugin.c */

    bool foo_plugin_supports_device(const char *device_type)
    {
        if (/* device_type is supported */)
            return true;

        ...

        return false;
    }

    int opae_plugin_configure(opae_api_adapter_table *table, const char *config)
    {
        ... 

        adapter->supports_device =
                dlsym(adapter->plugin.dl_handle, "foo_plugin_supports_device");

        ...

        return 0;
    }
```

```eval_rst
.. note::
    The `opae_plugin_supports_device` mechanism serves as a placeholder only.
    It is not implemented in the current version of the OPAE C API.
```

Similarly to determining whether a plugin supports a type of device, a plugin
may also answer questions about network host support by populating an
`opae_plugin_supports_host` entry point in the adapter table:

```c
    bool opae_plugin_supports_host(const char *hostname);
```

`opae_plugin_supports_host` returns true if the given hostname is supported
and false if it is not. A false return value from `opae_plugin_supports_host`
causes device enumeration to skip the plugin.

Populating the `opae_plugin_supports_host` is done as:

```c
    /* foo_plugin.c */

    bool foo_plugin_supports_host(const char *hostname)
    {
        if (/* hostname is supported */)
            return true;

        ...

        return false;
    }

    int opae_plugin_configure(opae_api_adapter_table *table, const char *config)
    {
        ... 

        adapter->supports_host =
                dlsym(adapter->plugin.dl_handle, "foo_plugin_supports_host");

        ...

        return 0;
    }
```

```eval_rst
.. note::
    The `opae_plugin_supports_host` mechanism serves as a placeholder only.
    It is not implemented in the current version of the OPAE C API.
```

## Plugin Construction ##

The steps required to implement an OPAE C API plugin, libfoo.so, are:

* Create foo\_plugin.c: implements `opae_plugin_configure`,
`opae_plugin_initialize`, `opae_plugin_finalize`, `opae_plugin_supports_device`,
and `opae_plugin_supports_host` as described in the previous sections.
* Create foo\_plugin.h: implements function prototypes for each of the
plugin-specific OPAE C APIs.

```c
    /* foo_plugin.h */

    fpga_result foo_fpgaOpen(fpga_token token, fpga_handle *handle,
                             int flags);

    fpga_result foo_fpgaClose(fpga_handle handle);

    ...

    fpga_result foo_fpgaEnumerate(const fpga_properties *filters,
                                  uint32_t num_filters, fpga_token *tokens,
                                  uint32_t max_tokens,
                                  uint32_t *num_matches);
    ...
```

* Create foo\_types.h: implements plugin-specific types for opaque data
structures.

```c
    /* foo_types.h */

    struct _foo_token {
        ...
    };

    struct _foo_handle {
        ...
    };

    struct _foo_event_handle {
        ...
    };

    struct _foo_object {
        ...
    };
```

* Create foo\_enum.c: implements `foo_fpgaEnumerate`,
`foo_fpgaCloneToken`, and `foo_fpgaDestroyToken`.
* Create foo\_open.c: implements `foo_fpgaOpen`.
* Create foo\_close.c: implements `foo_fpgaClose`.
* Create foo\_props.c: implements `foo_fpgaGetProperties`,
`foo_fpgaGetPropertiesFromHandle`, `foo_fpgaUpdateProperties`
* Create foo\_mmio.c: implements `foo_fpgaMapMMIO`, `foo_fpgaUnmapMMIO`
`foo_fpgaWriteMMIO64`, `foo_fpgaReadMMIO64`, `foo_fpgaWriteMMIO32`,
`foo_fpgaReadMMIO32`.
* Create foo\_buff.c: implements `foo_fpgaPrepareBuffer`,
`foo_fpgaReleaseBuffer`, `foo_fpgaGetIOAddress`.
* Create foo\_error.c: implements `foo_fpgaReadError`, `foo_fpgaClearError`,
`foo_fpgaClearAllErrors`, `foo_fpgaGetErrorInfo`.
* Create foo\_event.c: implements `foo_fpgaCreateEventHandle`,
`foo_fpgaDestroyEventHandle`, `foo_fpgaGetOSObjectFromEventHandle`,
`foo_fpgaRegisterEvent`, `foo_fpgaUnregisterEvent`.
* Create foo\_reconf.c: implements `foo_fpgaReconfigureSlot`.
* Create foo\_obj.c: implements `foo_fpgaTokenGetObject`,
`foo_fpgaHandleGetObject`, `foo_fpgaObjectGetObject`,
`foo_fpgaDestroyObject`, `foo_fpgaObjectGetSize`, `foo_fpgaObjectRead`,
`foo_fpgaObjectRead64`, `foo_fpgaObjectWrite64`.
* Create foo\_clk.c: implements `foo_fpgaSetUserClock`,
`foo_fpgaGetUserClock`.
