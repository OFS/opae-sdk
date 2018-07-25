# Plugin Architecture #
An OPAE plugin is a software library that can be loaded dynamically at runtime and is either specific to a given platform or is a proxy for one or more remote endpoints.
While it is not required that a plugin implements the complete OPAE API, it is required, however, to adhere to the plugin interface. Futhermore, any OPAE API functions implemented by a plugin must follow their corresponding function interfaces as defined in the OPAE API specification.

## Objective ##
The objective of this document is to provide architectural details about the plugin interface as well as the plugin manager. While it is possible to use the plugin manager to design a framework for pooling of OPAE resources, it is outside of the scope of this document. Details of how to manage and connect to remote endpoints are also out of scope for the plugin architecture although proxy or remote resources may be mentioned.

## Components ##

### Plugin Interface ###
The following list describes features that are compatible with the Plugin Loader:
* It must implement at least one OPAE API function
* Any OPAE API functions it implements must use the same function signature as defined by the OPAE API specification
* It may define an optional configuration routine in a function called `opaePluginConfigure` to provide a mechanism for configuration of the plugin from external configuration data. It must follow the following function signature:
  * The function takes one argument of type `const char*` used to pass in any configuration related data.
  
    The configuration data will be encoded in a JSON structure. In order to avoid introducing dependencies on other libraries, it will be extected that the JSON structure be serialized. It is up to the plugin on how it will deserialize it.
  * The function must return zero (0) upon successful configuration and a non-zero value otherwise. It is up to the plugin developer to define and document return codes.

  The following is an example of the configuration function declaration:
  ```C
  int opaePluginConfigure(const char* jsonConfig);
  ```

* It may define an optional initialization routine in a function called `opaePluginInitialize` to provide a mechanism for initialization of the plugin. It must follow the function signature:
  * The function takes no arguments
  * The function must return zero (0) upon successful initialization and a non-zero value otherwise. It is up to the plugin developer to define and document return codes.

* It may define an optional finalization routine in a function called `opaePluginFinalize` to provide a mechanism for plugin finalization (or any cleanup routines). It must follow the function signature:
  * The function takes no arguments
  * The function must return zero (0) upon successful initialization and a non-zero value otherwise. It is up to the plugin developer to define and document return codes.

* Any plugin interfaces implemented must have the ABI vislibility set to default. This is implicitly set by not setting the visibility attribute or by explicitly setting it to default as listed in the example below:
  ```C
  #define DLL_PUBLIC __attribute__((visibility ("default")))

  int DLL_PUBLIC opaePluginConfigure(const char* c);
  ```
### Plugin Loader ###

#### Loading a Plugin ####


#### Plugin Manager ####


#### Calling a Function ####

#### Fault Tolerance ####


```
# pluginLoader
load_plugin(name, config, plugin_struct):
    h = dlopen(name)
    c = dlsym(h, "configure")
    if c:
        c(config)
    for each plugin_API_func:
        fp = dlsym(h, plugin_API_func)
        plugin_struct.plugin_API_func = fp



class pluginManger
    init(config)
        cd = parse_config(config)
        for each plugin in cd["plugins"]:
            plugin_struct ps
            load_plugin(plugin["name"], config, ps)
            myplugins.add(ps)

fpgaEnumerate(filter, tokens)
    for each plugin in resourcemanager:
        plugin.fpgaEnumerate(filter, plugin_tokens)
        for each plugin_token:
            tag_token(plugin_token, plugin)
            tokens.append(plugin_token)
    return FPGA_OK

fpgaOpen(t, h):
    p = rm.find_plugin(t.plugin)
    p.fpgaOpen(t, h)
    tag_handle(h, p)




```

```
so_plugin (libopae-c):
    configure(config): NA
    init(): NA

    fpgaEnumerate()

so_plugin ()


proxy_plugin:
    configure(config)
        addr = config[address]
        c = connect(addr)
        store connection info
    init()
        discover()

    fpgaEnumerate(filter, tokens):
        c.send_message({"enumerate", [filters]})
        data = c.recv()

        process_data(data, tokens)

        return FPGA_OK

    fpgaOpen(t, h):
        c.send_message({"open", toke})
        data = c.recv()
        make_handle(data)
        return FPGA_OK
```
