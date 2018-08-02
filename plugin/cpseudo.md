### OPAE API Data Structures ###
`fpga_token` and `fpga_handle` are opaque types. This allows them to be
easily wrapped at any level of the plugin stack.

```c
#ifndef __OPAE_TYPES_H__
#define __OPAE_TYPES_H__

typedef void *fpga_token;
typedef void *fpga_handle;

#endif
```

### Plugin Manager ###
The plugin manager tracks each loaded plugin in its list of OPAE
API adpater tables.

```c
#ifndef __OPAE_PLUGIN_MGR_H__
#define __OPAE_PLUGIN_MGR_H__

int opae_plugin_mgr_initialize(const char *cfg_file)
{
	opae_plugin_mgr_parse_config(json_open(cfg_file));
}

int opae_plugin_mgr_parse_config(json_object *jobj)
{
	(parse jobj to plugins section of config file)

	return opae_plugin_ldr_load_plugins(jobj);
}

int opae_plugin_mgr_register_adapter(api_adapter_table *adapter)
{
	(add adapter to plugin mgr's adapter table list)
}

#endif
```

### Plugin Loader ###
The plugin loader provides the basic facilities for locating and
loading OPAE plugins, given a description of the desired plugins
in a formatted configuration file (JSON). The loader registers
each loaded plugin with the plugin manager.


```c
#ifndef __OPAE_PLUGIN_LDR_H__
#define __OPAE_PLUGIN_LDR_H__

typedef struct _opae_plugin {
	char *path;     // location on file system
	void *dlhandle; // handle to the loaded library instance
} opae_plugin;

typedef struct _opae_api_adapter_table {
	opae_plugin plugin;

	fpga_result (*fpgaEnumerate)(const fpga_properties *filters,
				     uint32_t num_filters,
				     fpga_token *tokens,
				     uint32_t max_tokens,
				     uint32_t *num_matches);

	fpga_result (*fpgaOpen)(fpga_token token,
				fpga_handle *handle,
				int flags);
        ...

} opae_api_adapter_table;

typedef struct _opae_wrapped_token {
	fpga_token opae_token;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_token;

typedef struct _opae_wrapped_handle {
	opae_wrapped_token *wtok;
	fpga_handle opae_handle;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_handle;

int opae_plugin_ldr_load_plugins(json_object *jobj)
{
	opae_plugin pl;
	opae_api_adapter_table *adapter;

	for (each plugin pl described in jobj) {

		jobj_for_pl = (plugin section of jobj describing
                               the current plugin, pl)

		(open pl)

		(load function pointer for 'opaePluginConfigure')
		
		if (!opaePluginConfigure(serialize(jobj_for_pl))) {

			(load function pointer for 'opaePluginInitialize')

			if (opaePluginInitialize()) {
				(fail the current plugin)
			}

			(allocate and init adapter to 0)

			for (each OPAE API api) {

				(load function pointer for api)
				(place api in adapter)

			}

			if (OPAE API count > 0) {
				opae_plugin_mgr_register_adapter(adapter);
			}

		}

	}
}

#endif
```

### OPAE Stack ###
The OPAE API consists of the plugin manager, the plugin loader,
and 'shell' implementations of the superset of library calls.
Each 'shell' library call uses the adapter table(s) to call through
to the appropriate plugin implementation.

```c
#ifndef __OPAE_API_H__
#define __OPAE_API_H__

fpga_result fpgaEnumerate(const fpga_properties *filters,
			  uint32_t num_filters,
			  fpga_token *tokens,
			  uint32_t max_tokens,
			  uint32_t *num_matches)
{
	*num_matches = 0;
	fpga_token *pl_tokens = NULL;

	if (tokens) {
		pl_tokens = (allocate local tokens array)
	}

	for (each adapter in plugin_mgr adapter list) {
		uint32_t pl_matches = 0;

		adapter->fpgaEnumerate(filters, num_filters,
					pl_tokens, max_tokens - *num_matches,
					&pl_matches);

		*num_matches += pl_matches;

		if (!pl_tokens) { // request for num_matches only
			continue;
		}

		for (each fpga_token tok in pl_tokens) {

			opae_wrapped_token *wtok = alloc_wrapped_token();

			wtok->opae_token = tok;
			wtok->adapter_table = adapter; (map wtok to plugin)

			tokens[i] = wtok;
		}

		if (*num_matches == max_tokens) {
			break;
		}
	}
	
	free(pl_tokens);
}

fpga_result fpgaOpen(fpga_token token,
		     fpga_handle *handle,
		     int flags)
{
	fpga_handle h;
	opae_wrapped_token *wtok = (opae_wrapped_token *) token;

	wtok->adapter_table->fpgaOpen(wtok->opae_token, &h, flags);

	opae_wrapped_handle *whandle = alloc_wrapped_handle();

	whandle->wtok = wtok;
	whandle->opae_handle = h;
	whandle->adapter_table = wtok->adapter_table;
	
	*handle = whandle;
}

#endif
```
### Proxy Plugins ###
A TCP/IP OPAE plugin uses network sockets to implement the control
protocol exchange, but may utilize facilities such as RDMA for data
exchange.

```c
#ifndef __MY_TCP_IP_PLUGIN_H__
#define __MY_TCP_IP_PLUGIN_H__

typedef struct _my_tcp_ip_plugin_host_list {
	char *hostname;
	int port;
	struct addrinfo *addr_info;
	int conn_socket;
	struct _my_tcp_ip_plugin_host_list *next;
} my_tcp_ip_plugin_host_list;

static my_tcp_ip_plugin_host_list *host_list = NULL;

typedef struct _my_tcp_ip_enumerate_response {
	...
} my_tcp_ip_enumerate_response;

typedef struct _my_tcp_ip_plugin_token {
	my_tcp_ip_enumerate_response *response_token;
	my_tcp_ip_plugin_host_list *host;
} my_tcp_ip_plugin_token;

typedef struct _my_tcp_ip_open_response {
	...
} my_tcp_ip_open_response;

typedef struct _my_tcp_ip_plugin_handle {
	my_tcp_ip_plugin_token *token;
	my_tcp_ip_plugin_host_list *host;
	my_tcp_ip_open_response *response_handle;
} my_tcp_ip_plugin_handle;

int opaePluginConfigure(const char *jsonConfig)
{
	json_object *jobj;

	jobj = deserialize(jsonConfig);

	for (each host in jobj) {

		add_list(&host_list, alloc_host_entry(host.name, host.port));

	}
}

int opaePluginInitialize(void)
{
	for (each host in host_list) {

		connect_host(host);

	}
}

int opaePluginFinalize(void)
{
	for (each host in host_list) {

		disconnect_host(host);
		free(host);
	
	}
}

fpga_result fpgaEnumerate(const fpga_properties *filters,
			  uint32_t num_filters,
			  fpga_token *tokens,
			  uint32_t max_tokens,
			  uint32_t *num_matches)
{
	my_tcp_ip_enumerate_response *response_tokens;
	uint32_t num_responses = 0;

	for (each host in host_list) {

		send_enumerate_request(host, filters, ...);

		recv_enumerate_response(&response_tokens, &num_responses);

		for (each resp in response_tokens) {

			my_tcp_ip_plugin_token *tok = my_tcp_ip_alloc_token();

			tok->response_token = resp;
			tok->host = host;

			tokens[i] = tok;
		}
	}
}

fpga_result fpgaOpen(fpga_token token,
		     fpga_handle *handle,
		     int flags)
{
	my_tcp_ip_plugin_token *plugin_token = (my_tcp_ip_plugin_token *) token;
	my_tcp_ip_open_response *open_response;

	send_open_request(plugin_token->host, plugin_token->response_token, flags);

	open_response = recv_open_response(plugin_token->host);

	my_tcp_ip_plugin_handle *plugin_handle = my_tcp_ip_alloc_handle();

	plugin_handle->token = plugin_token;
	plugin_handle->host = plugin_token->host;
	plugin_handle->response_handle = open_response;

	*handle = plugin_handle;
}

#endif
```

An RDMA OPAE plugin uses RDMA for both control protocol and
data exchange.

```c
#ifndef __MY_RDMA_PLUGIN_H__
#define __MY_RDMA_PLUGIN_H__

typedef struct _my_rdma_plugin_host_list {
	enum rdma_conn_type conn_type;

	struct ibv_context *ctx;
	struct ibv_pd *pd;

	...

	struct _my_rdma_plugin_host_list *next;
} my_rdma_plugin_host_list;

static my_rdma_plugin_host_list *host_list = NULL;

typedef struct _my_rdma_enumerate_response {
	...
} my_rdma_enumerate_response;

typedef struct _my_rdma_plugin_token {
	my_rdma_enumerate_response *response_token;
	my_rdma_plugin_host_list *host;
} my_rdma_plugin_token;

typedef struct _my_rdma_open_response {
	...
} my_rdma_open_response;

typedef struct _my_rdma_plugin_handle {
	my_rdma_plugin_token *token;
	my_rdma_plugin_host_list *host;
	my_rdma_open_response *response_handle;
} my_rdma_plugin_handle;

int opaePluginConfigure(const char *jsonConfig)
{
	json_object *jobj;

	jobj = deserialize(jsonConfig);

	for (each host in jobj) {

		add_list(&host_list, alloc_host_entry(host.name, host.port));

	}
}

int opaePluginInitialize(void)
{
	for (each host in host_list) {

		init_host_rdma(host);

	}
}

int opaePluginFinalize(void)
{
	for (each host in host_list) {

		deinit_host_rdma(host);
		free(host);
	
	}
}

fpga_result fpgaEnumerate(const fpga_properties *filters,
			  uint32_t num_filters,
			  fpga_token *tokens,
			  uint32_t max_tokens,
			  uint32_t *num_matches)
{
	my_rdma_enumerate_response *response_tokens;
	uint32_t num_responses = 0;

	for (each host in host_list) {

		send_rdma_enumerate_request(host, filters, ...);

		recv_rdma_enumerate_response(&response_tokens, &num_responses);

		for (each resp in response_tokens) {

			my_rdma_plugin_token *tok = my_rdma_alloc_token();

			tok->response_token = resp;
			tok->host = host;

			tokens[i] = tok;
		}
	}
}

fpga_result fpgaOpen(fpga_token token,
		     fpga_handle *handle,
		     int flags)
{
	my_rdma_plugin_token *plugin_token = (my_rdma_plugin_token *) token;
	my_rdma_open_response *open_response;

	send_rdma_open_request(plugin_token->host, plugin_token->response_token);

	open_response = recv_rdma_open_response(plugin_token->host);

	my_rdma_plugin_handle *plugin_handle = my_rdma_alloc_handle();

	plugin_handle->token = plugin_token;
	plugin_handle->host = plugin_token->host;
	plugin_handle->response_handle = open_response;

	*handle = plugin_handle;
}

#endif
```

The 'local' OPAE plugin communicates with the kernel device driver
via memory-mapped IO and sysfs attributes.

```c
#ifndef __MY_LOCAL_PLUGIN_H__
#define __MY_LOCAL_PLUGIN_H__

int opaePluginConfigure(const char *jsonConfig)
{
	return 0;
}

int opaePluginInitialize(void)
{
	return 0;
}

int opaePluginFinalize(void)
{
	return 0;
}

fpga_result fpgaEnumerate(const fpga_properties *filters,
			  uint32_t num_filters,
			  fpga_token *tokens,
			  uint32_t max_tokens,
			  uint32_t *num_matches)
{
	(traditional enumerate searches local sysfs)
}

fpga_result fpgaOpen(fpga_token token,
		     fpga_handle *handle,
		     int flags)
{
	struct _fpga_token *_tok = (struct _fpga_token *) token;
	struct _fpga_handle *_handle;

	(traditional open uses character device node from _tok)

	_handle = malloc(sizeof(struct _fpga_handle));

	...

	_handle->fddev = open(_tok.dev_path ...);

	...

	*handle = _handle;
}

#endif
```

Other plugins:
* AFU Simulation Environment (ASE)
* virtio-vsock (pool of accelerators assigned to VM's)
* RSD
