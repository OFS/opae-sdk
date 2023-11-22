<?xml version='1.0' encoding='UTF-8' standalone='yes' ?>
<tagfile doxygen_version="1.9.1">
  <compound kind="file">
    <name>access.h</name>
    <path>/root/include/opae/</path>
    <filename>access_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaOpen</name>
      <anchorfile>access_8h.html</anchorfile>
      <anchor>addde6b2bafcd6632a2c0b595c6bc0ef3</anchor>
      <arglist>(fpga_token token, fpga_handle *handle, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaClose</name>
      <anchorfile>access_8h.html</anchorfile>
      <anchor>ac83789ebb65dc6b2adeae3d7e7fa3e79</anchor>
      <arglist>(fpga_handle handle)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReset</name>
      <anchorfile>access_8h.html</anchorfile>
      <anchor>aa4addba9b864dbc614a1680dfc29dc59</anchor>
      <arglist>(fpga_handle handle)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>buffer.h</name>
    <path>/root/include/opae/</path>
    <filename>buffer_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPrepareBuffer</name>
      <anchorfile>buffer_8h.html</anchorfile>
      <anchor>aac3ed0146bc42c35f99610a319e87303</anchor>
      <arglist>(fpga_handle handle, uint64_t len, void **buf_addr, uint64_t *wsid, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReleaseBuffer</name>
      <anchorfile>buffer_8h.html</anchorfile>
      <anchor>a3d2302d336bbe5fe05a08a8f534d296b</anchor>
      <arglist>(fpga_handle handle, uint64_t wsid)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetIOAddress</name>
      <anchorfile>buffer_8h.html</anchorfile>
      <anchor>aed20b8768e38a5414a331dd09a2aa221</anchor>
      <arglist>(fpga_handle handle, uint64_t wsid, uint64_t *ioaddr)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaBindSVA</name>
      <anchorfile>buffer_8h.html</anchorfile>
      <anchor>afd9f338886cc152d82fe7d211404771d</anchor>
      <arglist>(fpga_handle handle, uint32_t *pasid)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>core.h</name>
    <path>/root/include/opae/cxx/</path>
    <filename>core_8h.html</filename>
    <includes id="errors_8h" name="errors.h" local="no" imported="no">opae/cxx/core/errors.h</includes>
    <includes id="events_8h" name="events.h" local="no" imported="no">opae/cxx/core/events.h</includes>
    <includes id="except_8h" name="except.h" local="no" imported="no">opae/cxx/core/except.h</includes>
    <includes id="handle_8h" name="handle.h" local="no" imported="no">opae/cxx/core/handle.h</includes>
    <includes id="cxx_2core_2properties_8h" name="properties.h" local="no" imported="no">opae/cxx/core/properties.h</includes>
    <includes id="pvalue_8h" name="pvalue.h" local="no" imported="no">opae/cxx/core/pvalue.h</includes>
    <includes id="shared__buffer_8h" name="shared_buffer.h" local="no" imported="no">opae/cxx/core/shared_buffer.h</includes>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <includes id="cxx_2core_2version_8h" name="version.h" local="no" imported="no">opae/cxx/core/version.h</includes>
  </compound>
  <compound kind="file">
    <name>errors.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>errors_8h.html</filename>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <includes id="types__enum_8h" name="types_enum.h" local="no" imported="no">opae/types_enum.h</includes>
    <class kind="class">opae::fpga::types::error</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>events.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>events_8h.html</filename>
    <includes id="handle_8h" name="handle.h" local="no" imported="no">opae/cxx/core/handle.h</includes>
    <includes id="types__enum_8h" name="types_enum.h" local="no" imported="no">opae/types_enum.h</includes>
    <class kind="class">opae::fpga::types::event</class>
    <class kind="struct">opae::fpga::types::event::type_t</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>except.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>except_8h.html</filename>
    <includes id="types__enum_8h" name="types_enum.h" local="no" imported="no">opae/types_enum.h</includes>
    <class kind="class">opae::fpga::types::src_location</class>
    <class kind="class">opae::fpga::types::except</class>
    <class kind="class">opae::fpga::types::invalid_param</class>
    <class kind="class">opae::fpga::types::busy</class>
    <class kind="class">opae::fpga::types::exception</class>
    <class kind="class">opae::fpga::types::not_found</class>
    <class kind="class">opae::fpga::types::no_memory</class>
    <class kind="class">opae::fpga::types::not_supported</class>
    <class kind="class">opae::fpga::types::no_driver</class>
    <class kind="class">opae::fpga::types::no_daemon</class>
    <class kind="class">opae::fpga::types::no_access</class>
    <class kind="class">opae::fpga::types::reconf_error</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
    <namespace>opae::fpga::types::detail</namespace>
    <member kind="define">
      <type>#define</type>
      <name>OPAECXX_HERE</name>
      <anchorfile>except_8h.html</anchorfile>
      <anchor>a2abf016a3c51c9fcc40ac9468a897592</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ASSERT_FPGA_OK</name>
      <anchorfile>except_8h.html</anchorfile>
      <anchor>a0daeed5d9e795aa1e0cce4d5469cdc94</anchor>
      <arglist>(r)</arglist>
    </member>
    <member kind="typedef">
      <type>bool(*</type>
      <name>exception_fn</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>a27ea035002c36a1c80c601a99b9fa3db</anchor>
      <arglist>)(fpga_result, const opae::fpga::types::src_location &amp;loc)</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>is_ok</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>ad6802666a800e1d9b4ffe647a816083f</anchor>
      <arglist>(fpga_result result, const opae::fpga::types::src_location &amp;loc)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>assert_fpga_ok</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>afb71b5ca2216303a26cd91e01da63bc2</anchor>
      <arglist>(fpga_result result, const opae::fpga::types::src_location &amp;loc)</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static exception_fn</type>
      <name>opae_exceptions</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>aaf5c7cfe3e7998add9471462d43ee1ff</anchor>
      <arglist>[12]</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>handle.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>handle_8h.html</filename>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <includes id="enum_8h" name="enum.h" local="no" imported="no">opae/enum.h</includes>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <class kind="class">opae::fpga::types::handle</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>properties.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>cxx_2core_2properties_8h.html</filename>
    <includes id="pvalue_8h" name="pvalue.h" local="no" imported="no">opae/cxx/core/pvalue.h</includes>
    <includes id="properties_8h" name="properties.h" local="no" imported="no">opae/properties.h</includes>
    <class kind="class">opae::fpga::types::properties</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>properties.h</name>
    <path>/root/include/opae/</path>
    <filename>properties_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetPropertiesFromHandle</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a027ca0f8c45573c378179d7f57971128</anchor>
      <arglist>(fpga_handle handle, fpga_properties *prop)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetProperties</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a010d3b6839fc8763e66e4f953682a489</anchor>
      <arglist>(fpga_token token, fpga_properties *prop)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaUpdateProperties</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a56a1c04d0c8f8eb1785969934f31a41a</anchor>
      <arglist>(fpga_token token, fpga_properties prop)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaClearProperties</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a9ef3dc600c085ecf513b9e3f2867ffd6</anchor>
      <arglist>(fpga_properties prop)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaCloneProperties</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>ac51c19bdd763a98e143a938c47963905</anchor>
      <arglist>(fpga_properties src, fpga_properties *dst)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaDestroyProperties</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a6f83533f996cf6bd25274e0b7b9c3050</anchor>
      <arglist>(fpga_properties *prop)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetParent</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>aa3d6d131a85e9c7cff4844c0931c1591</anchor>
      <arglist>(const fpga_properties prop, fpga_token *parent)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetParent</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>aa395756270c64e2d2e4e03ad37931f2d</anchor>
      <arglist>(fpga_properties prop, fpga_token parent)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetObjectType</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a5e4e034d981c29a65bbeafdd05d2fa87</anchor>
      <arglist>(const fpga_properties prop, fpga_objtype *objtype)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetObjectType</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a3724caa06200e0d3f0b56611c379d75b</anchor>
      <arglist>(fpga_properties prop, fpga_objtype objtype)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetSegment</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a5a086b105275b0506e6338e9797d0439</anchor>
      <arglist>(const fpga_properties prop, uint16_t *segment)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetSegment</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a2ff66f524bfbd8f84b0b657d92edb07f</anchor>
      <arglist>(fpga_properties prop, uint16_t segment)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetBus</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a29eda789a7322042dfa8d558d13205a1</anchor>
      <arglist>(const fpga_properties prop, uint8_t *bus)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetBus</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a3a10650ea23b050090d1b2759c9ec8f7</anchor>
      <arglist>(fpga_properties prop, uint8_t bus)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetDevice</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>abf2d527c4625bfd1558bb722c3c89380</anchor>
      <arglist>(const fpga_properties prop, uint8_t *device)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetDevice</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a73ea87129bcea24df9aaabcc2bd40760</anchor>
      <arglist>(fpga_properties prop, uint8_t device)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetFunction</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a789a2548aaea9b2069b964f58a931bf6</anchor>
      <arglist>(const fpga_properties prop, uint8_t *function)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetFunction</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a14045d2b80968dda5af4987b6246d17a</anchor>
      <arglist>(fpga_properties prop, uint8_t function)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetSocketID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a9978cb8bc354d2bbd644ed1a96395c8c</anchor>
      <arglist>(const fpga_properties prop, uint8_t *socket_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetSocketID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a51707174f2df674af9bc1171bde7f1ba</anchor>
      <arglist>(fpga_properties prop, uint8_t socket_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetDeviceID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>ac3237ae88feff35ba0c285e27cac58a5</anchor>
      <arglist>(const fpga_properties prop, uint16_t *device_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetDeviceID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>acfac3eea769b7872c1d6d7102de708aa</anchor>
      <arglist>(fpga_properties prop, uint16_t device_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetNumSlots</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>ad1a50fcb3a74c0acf89d2f4e75989085</anchor>
      <arglist>(const fpga_properties prop, uint32_t *num_slots)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetNumSlots</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a2338b49011195241a3ba5ad16f85dde8</anchor>
      <arglist>(fpga_properties prop, uint32_t num_slots)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetBBSID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>aa387edbec93d9588d6338d11aace653f</anchor>
      <arglist>(const fpga_properties prop, uint64_t *bbs_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetBBSID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a456be39e014b2fb03b160ab010ddb189</anchor>
      <arglist>(fpga_properties prop, uint64_t bbs_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetBBSVersion</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a08186263b04a5193471469e9d70ae372</anchor>
      <arglist>(const fpga_properties prop, fpga_version *bbs_version)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetBBSVersion</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a9e3bfec10b26500718ae0afef146f06a</anchor>
      <arglist>(fpga_properties prop, fpga_version version)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetVendorID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a03408aebf73915938507c529bba6aeb3</anchor>
      <arglist>(const fpga_properties prop, uint16_t *vendor_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetVendorID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a1393cac49b4c34fa23e95367bc14d811</anchor>
      <arglist>(fpga_properties prop, uint16_t vendor_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetModel</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a8c00a2915132284aea5faa0d72dfd5e5</anchor>
      <arglist>(const fpga_properties prop, char *model)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetModel</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>acd4fc992cbb34efe619e387f923c4c05</anchor>
      <arglist>(fpga_properties prop, char *model)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetLocalMemorySize</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>aeff04e09cc045f314695b4aac0e5516a</anchor>
      <arglist>(const fpga_properties prop, uint64_t *lms)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetLocalMemorySize</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>af491fd7a2621aa66de5059168afe3792</anchor>
      <arglist>(fpga_properties prop, uint64_t lms)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetCapabilities</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a9b80ec35fbe0498f332782ca5d8ec2ab</anchor>
      <arglist>(const fpga_properties prop, uint64_t *capabilities)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetCapabilities</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a45bad0a5baa2cb1f582af571d6ed3917</anchor>
      <arglist>(fpga_properties prop, uint64_t capabilities)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetGUID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a5728ceaf03173772069df0610b4646d7</anchor>
      <arglist>(const fpga_properties prop, fpga_guid *guid)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetGUID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a71ad1e59994b2c8b04ea036a64515117</anchor>
      <arglist>(fpga_properties prop, fpga_guid guid)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetNumMMIO</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a7b70efbf6892e9daaf3700b21cc50f0c</anchor>
      <arglist>(const fpga_properties prop, uint32_t *mmio_spaces)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetNumMMIO</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a1695e0bf61ee5188401bd230db022314</anchor>
      <arglist>(fpga_properties prop, uint32_t mmio_spaces)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetNumInterrupts</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a33ee7ad6994f3df0a044d9052627f8c0</anchor>
      <arglist>(const fpga_properties prop, uint32_t *num_interrupts)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetNumInterrupts</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>ace3b67e6acb69bef8d8d8b32dab7a044</anchor>
      <arglist>(fpga_properties prop, uint32_t num_interrupts)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetAcceleratorState</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a1e18fdf7c532bf564576b361874fccf3</anchor>
      <arglist>(const fpga_properties prop, fpga_accelerator_state *state)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetAcceleratorState</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a1a3f5725be24b3fc10625dfd604ed86d</anchor>
      <arglist>(fpga_properties prop, fpga_accelerator_state state)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetObjectID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a409e32267db6f5040d881d5d533c36fb</anchor>
      <arglist>(const fpga_properties prop, uint64_t *object_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetObjectID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a022ad3dc80ab2fba7e512c6c2215258a</anchor>
      <arglist>(const fpga_properties prop, uint64_t object_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetNumErrors</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a6cfb80c2a4fe8fd7be1ac4ecd72985b5</anchor>
      <arglist>(const fpga_properties prop, uint32_t *num_errors)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetNumErrors</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>ac8be299c378aff53410dda6081975038</anchor>
      <arglist>(const fpga_properties prop, uint32_t num_errors)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetInterface</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a8fe0f0d91d844aeaced5b761c22940b0</anchor>
      <arglist>(const fpga_properties prop, fpga_interface *interface)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetInterface</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a29e51f74386de3ecedde125157b24057</anchor>
      <arglist>(const fpga_properties prop, fpga_interface interface)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetSubsystemVendorID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a8e58481bfe2c97de9a4582d8acbbc356</anchor>
      <arglist>(const fpga_properties prop, uint16_t *subsystem_vendor_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetSubsystemVendorID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a26296be1fc4c0f53d37e84c1d7741963</anchor>
      <arglist>(fpga_properties prop, uint16_t subsystem_vendor_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesGetSubsystemDeviceID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>ac5eb7e22334b5dff483a6542b12644c9</anchor>
      <arglist>(const fpga_properties prop, uint16_t *subsystem_device_id)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaPropertiesSetSubsystemDeviceID</name>
      <anchorfile>properties_8h.html</anchorfile>
      <anchor>a66d64cd10bb9b834e389f6d4d618e556</anchor>
      <arglist>(fpga_properties prop, uint16_t subsystem_device_id)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>pvalue.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>pvalue_8h.html</filename>
    <includes id="except_8h" name="except.h" local="no" imported="no">opae/cxx/core/except.h</includes>
    <includes id="properties_8h" name="properties.h" local="no" imported="no">opae/properties.h</includes>
    <includes id="utils_8h" name="utils.h" local="no" imported="no">opae/utils.h</includes>
    <class kind="struct">opae::fpga::types::guid_t</class>
    <class kind="struct">opae::fpga::types::pvalue</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>shared_buffer.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>shared__buffer_8h.html</filename>
    <includes id="buffer_8h" name="buffer.h" local="no" imported="no">opae/buffer.h</includes>
    <includes id="except_8h" name="except.h" local="no" imported="no">opae/cxx/core/except.h</includes>
    <includes id="handle_8h" name="handle.h" local="no" imported="no">opae/cxx/core/handle.h</includes>
    <class kind="class">opae::fpga::types::shared_buffer</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>sysobject.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>cxx_2core_2sysobject_8h.html</filename>
    <includes id="handle_8h" name="handle.h" local="no" imported="no">opae/cxx/core/handle.h</includes>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <class kind="class">opae::fpga::types::sysobject</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>sysobject.h</name>
    <path>/root/include/opae/</path>
    <filename>sysobject_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaTokenGetObject</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>aabea869646f2a612efc878e35d25c352</anchor>
      <arglist>(fpga_token token, const char *name, fpga_object *object, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaHandleGetObject</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a80ccae112e76764fe9aa258b06a2672e</anchor>
      <arglist>(fpga_handle handle, const char *name, fpga_object *object, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectGetObject</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a9854745736c2bd3dc41fbbb3de873ccd</anchor>
      <arglist>(fpga_object parent, const char *name, fpga_object *object, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectGetObjectAt</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a9f166aaada5bae7a80472c3a016f01d1</anchor>
      <arglist>(fpga_object parent, size_t idx, fpga_object *object)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectGetType</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a77103f7bd53c12b56e47922aea36dbf0</anchor>
      <arglist>(fpga_object obj, enum fpga_sysobject_type *type)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaDestroyObject</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a5829c9ba73a939da7faa71e7dca442b3</anchor>
      <arglist>(fpga_object *obj)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectGetSize</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a29a136634ad8f8b741ae00a52bca5dd4</anchor>
      <arglist>(fpga_object obj, uint32_t *value, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectRead</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a7ccef34c42dbddc7cd534aa598767d08</anchor>
      <arglist>(fpga_object obj, uint8_t *buffer, size_t offset, size_t len, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectRead64</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a03de9a8f3a530abc6918c5c6682d92cc</anchor>
      <arglist>(fpga_object obj, uint64_t *value, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaObjectWrite64</name>
      <anchorfile>sysobject_8h.html</anchorfile>
      <anchor>a6dbc79fd4660f2fc576e8b7eb64d27fd</anchor>
      <arglist>(fpga_object obj, uint64_t value, int flags)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>token.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>token_8h.html</filename>
    <includes id="access_8h" name="access.h" local="no" imported="no">opae/access.h</includes>
    <includes id="cxx_2core_2properties_8h" name="properties.h" local="no" imported="no">opae/cxx/core/properties.h</includes>
    <includes id="enum_8h" name="enum.h" local="no" imported="no">opae/enum.h</includes>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <class kind="class">opae::fpga::types::token</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>version.h</name>
    <path>/root/include/opae/cxx/core/</path>
    <filename>cxx_2core_2version_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <class kind="class">opae::fpga::types::version</class>
    <namespace>opae</namespace>
    <namespace>opae::fpga</namespace>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="file">
    <name>version.h</name>
    <path>/root/include/opae/</path>
    <filename>version_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_VERSION_STR_MAX</name>
      <anchorfile>version_8h.html</anchorfile>
      <anchor>a32aa7a5e5d7cfcc9e1058c2d74718140</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_BUILD_STR_MAX</name>
      <anchorfile>version_8h.html</anchorfile>
      <anchor>a76b0f7badf1367c51275cd1cadc16146</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetOPAECVersion</name>
      <anchorfile>version_8h.html</anchorfile>
      <anchor>a1af8e881dda1b5f823336fe984665519</anchor>
      <arglist>(fpga_version *version)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetOPAECVersionString</name>
      <anchorfile>version_8h.html</anchorfile>
      <anchor>aade1f86d5d30658a7ccddaa54db74ab8</anchor>
      <arglist>(char *version_str, size_t len)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetOPAECBuildString</name>
      <anchorfile>version_8h.html</anchorfile>
      <anchor>a4291b6b91fc85bff7d342109ac0e05ab</anchor>
      <arglist>(char *build_str, size_t len)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>enum.h</name>
    <path>/root/include/opae/</path>
    <filename>enum_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaEnumerate</name>
      <anchorfile>enum_8h.html</anchorfile>
      <anchor>a277ba17f2377895855545bd82c1f901d</anchor>
      <arglist>(const fpga_properties *filters, uint32_t num_filters, fpga_token *tokens, uint32_t max_tokens, uint32_t *num_matches)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaCloneToken</name>
      <anchorfile>enum_8h.html</anchorfile>
      <anchor>a43a84795de460e8288070b672ef90a59</anchor>
      <arglist>(fpga_token src, fpga_token *dst)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaDestroyToken</name>
      <anchorfile>enum_8h.html</anchorfile>
      <anchor>a150a7a8f46e0d6df12cc329ff7030e21</anchor>
      <arglist>(fpga_token *token)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>error.h</name>
    <path>/root/include/opae/</path>
    <filename>error_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReadError</name>
      <anchorfile>error_8h.html</anchorfile>
      <anchor>a4c79f1152fc283dc1afaceca37f57352</anchor>
      <arglist>(fpga_token token, uint32_t error_num, uint64_t *value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaClearError</name>
      <anchorfile>error_8h.html</anchorfile>
      <anchor>aef9338fcc0cceb8ca40925bf13163d14</anchor>
      <arglist>(fpga_token token, uint32_t error_num)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaClearAllErrors</name>
      <anchorfile>error_8h.html</anchorfile>
      <anchor>a0cfcecd4e2e79f296ad51fa37e4bf6cf</anchor>
      <arglist>(fpga_token token)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetErrorInfo</name>
      <anchorfile>error_8h.html</anchorfile>
      <anchor>a4c0197e1ceff1d6fa15e319396fa3e43</anchor>
      <arglist>(fpga_token token, uint32_t error_num, struct fpga_error_info *error_info)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>event.h</name>
    <path>/root/include/opae/</path>
    <filename>event_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaCreateEventHandle</name>
      <anchorfile>event_8h.html</anchorfile>
      <anchor>a54fb1847300ff886b4ad857716075083</anchor>
      <arglist>(fpga_event_handle *event_handle)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaDestroyEventHandle</name>
      <anchorfile>event_8h.html</anchorfile>
      <anchor>ab8e748d1c491717d677a96c23dee987b</anchor>
      <arglist>(fpga_event_handle *event_handle)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetOSObjectFromEventHandle</name>
      <anchorfile>event_8h.html</anchorfile>
      <anchor>a9c38c9cf434a896e7cf02a7df8dc5c2e</anchor>
      <arglist>(const fpga_event_handle eh, int *fd)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaRegisterEvent</name>
      <anchorfile>event_8h.html</anchorfile>
      <anchor>acee9793072cfe2d18a9603339cf5e8a7</anchor>
      <arglist>(fpga_handle handle, fpga_event_type event_type, fpga_event_handle event_handle, uint32_t flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaUnregisterEvent</name>
      <anchorfile>event_8h.html</anchorfile>
      <anchor>aa9f920468d8ff05e7411c925a176f5cb</anchor>
      <arglist>(fpga_handle handle, fpga_event_type event_type, fpga_event_handle event_handle)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>fpga.h</name>
    <path>/root/include/opae/</path>
    <filename>fpga_8h.html</filename>
    <includes id="log_8h" name="log.h" local="no" imported="no">opae/log.h</includes>
    <includes id="init_8h" name="init.h" local="no" imported="no">opae/init.h</includes>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <includes id="access_8h" name="access.h" local="no" imported="no">opae/access.h</includes>
    <includes id="buffer_8h" name="buffer.h" local="no" imported="no">opae/buffer.h</includes>
    <includes id="enum_8h" name="enum.h" local="no" imported="no">opae/enum.h</includes>
    <includes id="event_8h" name="event.h" local="no" imported="no">opae/event.h</includes>
    <includes id="manage_8h" name="manage.h" local="no" imported="no">opae/manage.h</includes>
    <includes id="include_2opae_2mmio_8h" name="mmio.h" local="no" imported="no">opae/mmio.h</includes>
    <includes id="properties_8h" name="properties.h" local="no" imported="no">opae/properties.h</includes>
    <includes id="umsg_8h" name="umsg.h" local="no" imported="no">opae/umsg.h</includes>
    <includes id="utils_8h" name="utils.h" local="no" imported="no">opae/utils.h</includes>
    <includes id="error_8h" name="error.h" local="no" imported="no">opae/error.h</includes>
    <includes id="version_8h" name="version.h" local="no" imported="no">opae/version.h</includes>
    <includes id="sysobject_8h" name="sysobject.h" local="no" imported="no">opae/sysobject.h</includes>
    <includes id="userclk_8h" name="userclk.h" local="no" imported="no">opae/userclk.h</includes>
    <includes id="metrics_8h" name="metrics.h" local="no" imported="no">opae/metrics.h</includes>
  </compound>
  <compound kind="file">
    <name>hash_map.h</name>
    <path>/root/include/opae/</path>
    <filename>hash__map_8h.html</filename>
    <includes id="types__enum_8h" name="types_enum.h" local="no" imported="no">opae/types_enum.h</includes>
    <class kind="struct">opae_hash_map_item</class>
    <class kind="struct">opae_hash_map</class>
    <member kind="enumeration">
      <type></type>
      <name>opae_hash_map_flags</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a2f858c6198f438f243a9b886e361fa0c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>OPAE_HASH_MAP_UNIQUE_KEYSPACE</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a2f858c6198f438f243a9b886e361fa0ca4089fe07cd9c5d05ea168d2b49beae5d</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>opae_hash_map_init</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a5d8b6eed0eea3e1e74de38cb04c1d4e2</anchor>
      <arglist>(opae_hash_map *hm, uint32_t num_buckets, uint32_t hash_seed, int flags, uint32_t(*key_hash)(uint32_t num_buckets, uint32_t hash_seed, void *key), int(*key_compare)(void *keya, void *keyb), void(*key_cleanup)(void *key, void *context), void(*value_cleanup)(void *value, void *context))</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>opae_hash_map_add</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>ae42574e8d296806a5b395fce978a1338</anchor>
      <arglist>(opae_hash_map *hm, void *key, void *value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>opae_hash_map_find</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a2cf46fb910f75709741009e5f6c42b16</anchor>
      <arglist>(opae_hash_map *hm, void *key, void **value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>opae_hash_map_remove</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a5750c806056b605248bee90399e64e29</anchor>
      <arglist>(opae_hash_map *hm, void *key)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>opae_hash_map_destroy</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>adb709ed616790bfc7fd58d2b3f72b69c</anchor>
      <arglist>(opae_hash_map *hm)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>opae_hash_map_is_empty</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a6d05504a7cf422f506626a68a9aca28e</anchor>
      <arglist>(opae_hash_map *hm)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>opae_u64_key_hash</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a21604f9336e0cff5cacdfa01d0d56b87</anchor>
      <arglist>(uint32_t num_buckets, uint32_t hash_seed, void *key)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_u64_key_compare</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>abccdf1c3216e0b98093641cd44f93a10</anchor>
      <arglist>(void *keya, void *keyb)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>init.h</name>
    <path>/root/include/opae/</path>
    <filename>init_8h.html</filename>
    <includes id="types__enum_8h" name="types_enum.h" local="no" imported="no">opae/types_enum.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaInitialize</name>
      <anchorfile>init_8h.html</anchorfile>
      <anchor>a095c12e79f0fa4ce99512280a1df4aa6</anchor>
      <arglist>(const char *config_file)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaFinalize</name>
      <anchorfile>init_8h.html</anchorfile>
      <anchor>a89e0739f47b2c9b3c9459912adee9cfa</anchor>
      <arglist>(void)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>log.h</name>
    <path>/root/include/opae/</path>
    <filename>log_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="define">
      <type>#define</type>
      <name>__SHORT_FILE__</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a42617cfd690fdcad76942009fb503b2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>OPAE_MSG</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a95b5a4d29214bb4fe7c189dd285b5f2f</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>OPAE_ERR</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a4c65a535e2f271bf6563675181de244c</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>OPAE_DBG</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>ae876bc51560016369ad918326c0ae98b</anchor>
      <arglist>(format,...)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>OPAE_DEFAULT_LOGLEVEL</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>ade66bd6de2fa0408da71936d8945695b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>opae_loglevel</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a1446ee33d55fcd970394399676673140</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>OPAE_LOG_ERROR</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a1446ee33d55fcd970394399676673140a952b12fed7c3961af4b59295d239f085</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>OPAE_LOG_MESSAGE</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a1446ee33d55fcd970394399676673140a3a3d401b3f2271629ac784e073cec6ca</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>OPAE_LOG_DEBUG</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>a1446ee33d55fcd970394399676673140aea6bac5e767d7f06f069a351258ce42d</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>opae_print</name>
      <anchorfile>log_8h.html</anchorfile>
      <anchor>aa5576c3d699afd77372b9443c7bf1bab</anchor>
      <arglist>(int loglevel, const char *fmt,...)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>manage.h</name>
    <path>/root/include/opae/</path>
    <filename>manage_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaAssignPortToInterface</name>
      <anchorfile>manage_8h.html</anchorfile>
      <anchor>aa8b0dd0eba99f99161ad8b726706ce9c</anchor>
      <arglist>(fpga_handle fpga, uint32_t interface_num, uint32_t slot_num, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaAssignToInterface</name>
      <anchorfile>manage_8h.html</anchorfile>
      <anchor>a768c0a5d0f2494a69470170a29a76578</anchor>
      <arglist>(fpga_handle fpga, fpga_token accelerator, uint32_t host_interface, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReleaseFromInterface</name>
      <anchorfile>manage_8h.html</anchorfile>
      <anchor>a1ce3163b0a8c0f5c39e1c5acefc049eb</anchor>
      <arglist>(fpga_handle fpga, fpga_token accelerator)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReconfigureSlot</name>
      <anchorfile>manage_8h.html</anchorfile>
      <anchor>a6e5d00d445c69c94cb122224c47bf735</anchor>
      <arglist>(fpga_handle fpga, uint32_t slot, const uint8_t *bitstream, size_t bitstream_len, int flags)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>mem_alloc.h</name>
    <path>/root/include/opae/</path>
    <filename>mem__alloc_8h.html</filename>
    <class kind="struct">mem_link</class>
    <class kind="struct">mem_alloc</class>
    <member kind="function">
      <type>void</type>
      <name>mem_alloc_init</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>ac37e826046364bcbc938be357d088dda</anchor>
      <arglist>(struct mem_alloc *m)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>mem_alloc_destroy</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>af51e1559cd8eb3aa3cc0c433fcfcaa71</anchor>
      <arglist>(struct mem_alloc *m)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mem_alloc_add_free</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>affae27295ea2cd566c06dba6fb34835f</anchor>
      <arglist>(struct mem_alloc *m, uint64_t address, uint64_t size)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mem_alloc_get</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>afdb4e638fb73f1199f739bdd4cad9b16</anchor>
      <arglist>(struct mem_alloc *m, uint64_t *address, uint64_t size)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>mem_alloc_put</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>a7f633432ea4eea5c671b37645a7270a4</anchor>
      <arglist>(struct mem_alloc *m, uint64_t address)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>metrics.h</name>
    <path>/root/include/opae/</path>
    <filename>metrics_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetNumMetrics</name>
      <anchorfile>metrics_8h.html</anchorfile>
      <anchor>ac4f079f737b78c4e9cd22b0d4dcd91aa</anchor>
      <arglist>(fpga_handle handle, uint64_t *num_metrics)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetMetricsInfo</name>
      <anchorfile>metrics_8h.html</anchorfile>
      <anchor>ac59305e77a52f5568f2eafc35277e1fa</anchor>
      <arglist>(fpga_handle handle, fpga_metric_info *metric_info, uint64_t *num_metrics)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetMetricsByIndex</name>
      <anchorfile>metrics_8h.html</anchorfile>
      <anchor>a3c956af4516f141d95527cdd4d22f786</anchor>
      <arglist>(fpga_handle handle, uint64_t *metric_num, uint64_t num_metric_indexes, fpga_metric *metrics)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetMetricsByName</name>
      <anchorfile>metrics_8h.html</anchorfile>
      <anchor>a2c19333a1eb6e0c51611fecaed2dc3a9</anchor>
      <arglist>(fpga_handle handle, char **metrics_names, uint64_t num_metric_names, fpga_metric *metrics)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetMetricsThresholdInfo</name>
      <anchorfile>metrics_8h.html</anchorfile>
      <anchor>af3cafc3244fdd6e387d5e05d6b58cd13</anchor>
      <arglist>(fpga_handle handle, struct metric_threshold *metric_thresholds, uint32_t *num_thresholds)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>mmio.h</name>
    <path>/root/include/opae/</path>
    <filename>include_2opae_2mmio_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaWriteMMIO64</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>a6df7f745d9b9d47582714fe8e2d1a761</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num, uint64_t offset, uint64_t value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReadMMIO64</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>a011ba900710ddf70c13ca089c4742187</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num, uint64_t offset, uint64_t *value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaWriteMMIO32</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>ae538bfe7158d1911c5e749bbc063aa3d</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num, uint64_t offset, uint32_t value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaReadMMIO32</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>a81dc89da3e94e26efff1af1eeebb7f5d</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num, uint64_t offset, uint32_t *value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaWriteMMIO512</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>a9cf56711df0f234686426c58d8b00cb0</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num, uint64_t offset, const void *value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaMapMMIO</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>a2903267d37ea5c64522b0addce74da5f</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num, uint64_t **mmio_ptr)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaUnmapMMIO</name>
      <anchorfile>include_2opae_2mmio_8h.html</anchorfile>
      <anchor>a8c8db22506e1fbfd16440c660bfee28f</anchor>
      <arglist>(fpga_handle handle, uint32_t mmio_num)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>mmio.h</name>
    <path>/root/samples/dummy_afu/</path>
    <filename>samples_2dummy__afu_2mmio_8h.html</filename>
    <includes id="dummy__afu_8h" name="dummy_afu.h" local="yes" imported="no">dummy_afu.h</includes>
    <class kind="class">dummy_afu::mmio_test</class>
    <namespace>dummy_afu</namespace>
    <member kind="function">
      <type>void</type>
      <name>timeit_wr</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a2331454a35bd7d061543b78f7708db09</anchor>
      <arglist>(std::shared_ptr&lt; spdlog::logger &gt; log, dummy_afu *afu, uint32_t count)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>timeit_rd</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ab7da4ff6c8571efbc707ba26cfba6723</anchor>
      <arglist>(std::shared_ptr&lt; spdlog::logger &gt; log, dummy_afu *afu, uint32_t count)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_verify</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ab2db23ee00af6f322f726f9017f60fdf</anchor>
      <arglist>(dummy_afu *afu, uint32_t addr, T value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_verify</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a8aa59ff4d5d11ffe262b3bcc1524046c</anchor>
      <arglist>(dummy_afu *afu, uint32_t addr, uint32_t i, uint64_t value)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>types.h</name>
    <path>/root/include/opae/</path>
    <filename>types_8h.html</filename>
    <includes id="types__enum_8h" name="types_enum.h" local="no" imported="no">opae/types_enum.h</includes>
    <class kind="struct">fpga_version</class>
    <class kind="struct">fpga_error_info</class>
    <class kind="union">metric_value</class>
    <class kind="struct">fpga_metric_info</class>
    <class kind="struct">fpga_metric</class>
    <class kind="struct">threshold</class>
    <class kind="struct">metric_threshold</class>
    <class kind="struct">fpga_token_header</class>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_ERROR_NAME_MAX</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a6af563bebf37e9c908aa7fe8470efa8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_METRIC_STR_SIZE</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a38d47f983b7adaa90dda28ef60de86ca</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>fpga_is_parent_child</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a8a497e598ca0addd676b0c4719011b47</anchor>
      <arglist>(__parent_hdr, __child_hdr)</arglist>
    </member>
    <member kind="typedef">
      <type>void *</type>
      <name>fpga_properties</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ab868bdeab946a8059abe7e9c114aee56</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void *</type>
      <name>fpga_token</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a252f538a10fb51d0988ed52946516d9e</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void *</type>
      <name>fpga_handle</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a4ad40f31195233b629bcde187b0556d5</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>uint8_t</type>
      <name>fpga_guid</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>af0b6683499be79fab39ac41db02e7abf</anchor>
      <arglist>[16]</arglist>
    </member>
    <member kind="typedef">
      <type>void *</type>
      <name>fpga_event_handle</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>acccb4e3dd49efd2b0999b14bf05d5aad</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>void *</type>
      <name>fpga_object</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ab0d91e42f9f3db11e2d095d3c0f728b2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>types_enum.h</name>
    <path>/root/include/opae/</path>
    <filename>types__enum_8h.html</filename>
    <member kind="enumeration">
      <type></type>
      <name>fpga_result</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OK</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da727cb0690aa450810ffc8f5371401327</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_INVALID_PARAM</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da72e05d22cb414d8a35945301b79302f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_BUSY</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da7785c48a8a9ab76b21bcd1627832fdd4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_EXCEPTION</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da8527745386f7cea8d8aa64f96f2249a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_NOT_FOUND</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da7aee5958cf7f46e9d80c4ee50d42d86c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_NO_MEMORY</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da01318ecc0ead885cee673d213bb4eeab</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_NOT_SUPPORTED</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da51eacd9111ef14917f0b958bfeda4f66</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_NO_DRIVER</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da8b18856fa126c4b659346cf5decda0b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_NO_DAEMON</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6daefcce3ebcef5f12e194f3812b43c7322</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_NO_ACCESS</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da6c7a58c1e0907394fe270a539b104b0e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_RECONF_ERROR</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a27aaa9bd2d94c9b53602b1a7af49fc6da4fe9aead9176152c1e47e600d5415e56</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_event_type</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a6eccf38d4643d14fbc51f34e03131fa6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_EVENT_INTERRUPT</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a6eccf38d4643d14fbc51f34e03131fa6a4927c38c144629dfd0ac7b798853d18f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_EVENT_ERROR</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a6eccf38d4643d14fbc51f34e03131fa6ae06578392a4b6edada24bb375b05d800</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_EVENT_POWER_THERMAL</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a6eccf38d4643d14fbc51f34e03131fa6ac5bbf42f4aadeb692ba00bc87d2a1d63</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_accelerator_state</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a90d0d4dfde2d893585fb72ba51f0bfbe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_ACCELERATOR_ASSIGNED</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a90d0d4dfde2d893585fb72ba51f0bfbeaa1212584ffe16949e0ed25c04a679786</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_ACCELERATOR_UNASSIGNED</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a90d0d4dfde2d893585fb72ba51f0bfbea35c1c5720c92650e289c592abfd5e2b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_objtype</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b2ddb9e533441e79f19d45fa0a24934</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_DEVICE</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b2ddb9e533441e79f19d45fa0a24934a91f784c1c357473adbde7188a244219a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_ACCELERATOR</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b2ddb9e533441e79f19d45fa0a24934a5c3973a253ffaabddd0a083f05abebc7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_interface</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b433f944fba4b80dd796dec286412cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_IFC_DFL</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b433f944fba4b80dd796dec286412cbada6c2b32c7c5e8eba60cebe80b38355c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_IFC_VFIO</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b433f944fba4b80dd796dec286412cba950d486b953b25fc8729fca05ea10962</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_IFC_SIM_DFL</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b433f944fba4b80dd796dec286412cba77bd30a60ce179d221303a4577d9ae17</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_IFC_SIM_VFIO</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b433f944fba4b80dd796dec286412cba6d64205e510545526e3b01ad672293da</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_IFC_UIO</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9b433f944fba4b80dd796dec286412cba47104e3152df9613560b18b507995691</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_buffer_flags</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a99f1b7a7fb02d7fa8e5823749b9005e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_BUF_PREALLOCATED</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a99f1b7a7fb02d7fa8e5823749b9005e5a625a9b51ada35d27da1079ff747d04db</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_BUF_QUIET</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a99f1b7a7fb02d7fa8e5823749b9005e5abc99b8f3e50fb4eac1dbbeae4aca7957</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_BUF_READ_ONLY</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a99f1b7a7fb02d7fa8e5823749b9005e5ae69aa057dde55423090ab4f7b9f3c200</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_open_flags</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a3a32599a1213352a3217f6e068651fc6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OPEN_SHARED</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a3a32599a1213352a3217f6e068651fc6a7eb08309368b559a0ecaf8193053a94c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_reconf_flags</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9a4d903f00f7511b06eea9809d81aae7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_RECONF_FORCE</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9a4d903f00f7511b06eea9809d81aae7a3eb0a410afacbd5b85d308fc51fc8cd8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_RECONF_SKIP_USRCLK</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9a4d903f00f7511b06eea9809d81aae7a4e1616b36eedc9094a04666edcfa36f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_sysobject_flags</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9cacfdee4baf5ca62998913596412afb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_SYNC</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9cacfdee4baf5ca62998913596412afba2d41c9cc1db0d054ac8a8e1afb707231</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_GLOB</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9cacfdee4baf5ca62998913596412afbaa8bfa7ba531a8b610b44f9364201e425</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_RAW</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9cacfdee4baf5ca62998913596412afbacfaa8f881c5a8a3dd212e4fb4d070bf4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_RECURSE_ONE</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9cacfdee4baf5ca62998913596412afbade8c9f9040403d5e7e08e3aede541844</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_RECURSE_ALL</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a9cacfdee4baf5ca62998913596412afba33387b602459cddf06b3eeab96eca47b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_sysobject_type</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a3ac77596a20038a5e0691ec8bb6c6299</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_CONTAINER</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a3ac77596a20038a5e0691ec8bb6c6299a9fdf8108c74c6fd8dec16f3c1bd7b46c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_OBJECT_ATTRIBUTE</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a3ac77596a20038a5e0691ec8bb6c6299ae05d85061319b15fc4fc8fbcbf6cf12c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_metric_type</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a04334291d3fe08ee2385f534629f3094</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_TYPE_POWER</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a04334291d3fe08ee2385f534629f3094a19df4d36912e022021758321d7dfaca1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_TYPE_THERMAL</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a04334291d3fe08ee2385f534629f3094a1b615b52e8ec5f4339139567bcabd9d0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_TYPE_PERFORMANCE_CTR</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a04334291d3fe08ee2385f534629f3094a0090631e9c7498fcb11d73aae843d4c0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_TYPE_AFU</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a04334291d3fe08ee2385f534629f3094afcaecf93ad13bf9558197c925c87d6c2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_TYPE_UNKNOWN</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>a04334291d3fe08ee2385f534629f3094aeccc87ae6dc265c54200f71aad44d43b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>fpga_metric_datatype</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>ab10f51ceeb88998e5d0389cbd3d55bcc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_DATATYPE_INT</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>ab10f51ceeb88998e5d0389cbd3d55bcca7e4011246e4e6ce4c62cbcd54c6b6446</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_DATATYPE_FLOAT</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>ab10f51ceeb88998e5d0389cbd3d55bcca42b7558a3659d44b5291a68329ac59ba</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_DATATYPE_DOUBLE</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>ab10f51ceeb88998e5d0389cbd3d55bcca0ef125e071fb8bb97d89e083055b350a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_DATATYPE_BOOL</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>ab10f51ceeb88998e5d0389cbd3d55bcca42af89cdb2fd1aaf8209950ff080a684</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGA_METRIC_DATATYPE_UNKNOWN</name>
      <anchorfile>types__enum_8h.html</anchorfile>
      <anchor>ab10f51ceeb88998e5d0389cbd3d55bccad5f469dac4b7a5961008918926f399dc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>uio.h</name>
    <path>/root/include/opae/</path>
    <filename>uio_8h.html</filename>
    <class kind="struct">opae_uio_device_region</class>
    <class kind="struct">opae_uio</class>
    <member kind="define">
      <type>#define</type>
      <name>OPAE_UIO_PATH_MAX</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>afad8f59a5be8c5d177a755bf4e56de2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_uio_open</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>af144910b738993ba8e6eb739b79c5a16</anchor>
      <arglist>(struct opae_uio *u, const char *dfl_device)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_uio_region_get</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>af8f4b24fd2828b3477ab657b6f4c4006</anchor>
      <arglist>(struct opae_uio *u, uint32_t index, uint8_t **ptr, size_t *size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>opae_uio_close</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>a0aacf36585d4d608e5b6ae8b74806aa9</anchor>
      <arglist>(struct opae_uio *u)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>umsg.h</name>
    <path>/root/include/opae/</path>
    <filename>umsg_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetNumUmsg</name>
      <anchorfile>umsg_8h.html</anchorfile>
      <anchor>aedba6aade335067f2dafec0ea92f040a</anchor>
      <arglist>(fpga_handle handle, uint64_t *value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaSetUmsgAttributes</name>
      <anchorfile>umsg_8h.html</anchorfile>
      <anchor>aa133e93939c8582925b10516232ef12d</anchor>
      <arglist>(fpga_handle handle, uint64_t value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaTriggerUmsg</name>
      <anchorfile>umsg_8h.html</anchorfile>
      <anchor>a9cc53b7511c056c86425eb8f451ac69d</anchor>
      <arglist>(fpga_handle handle, uint64_t value)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetUmsgPtr</name>
      <anchorfile>umsg_8h.html</anchorfile>
      <anchor>a97cca523fe1142578f3927ae452f4db9</anchor>
      <arglist>(fpga_handle handle, uint64_t **umsg_ptr)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>userclk.h</name>
    <path>/root/include/opae/</path>
    <filename>userclk_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaSetUserClock</name>
      <anchorfile>userclk_8h.html</anchorfile>
      <anchor>af82f255d29eea463f82ac0f1bc885edb</anchor>
      <arglist>(fpga_handle handle, uint64_t high_clk, uint64_t low_clk, int flags)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>fpgaGetUserClock</name>
      <anchorfile>userclk_8h.html</anchorfile>
      <anchor>a5562f9ec92c3dc8bb201587c91ef8352</anchor>
      <arglist>(fpga_handle handle, uint64_t *high_clk, uint64_t *low_clk, int flags)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>utils.h</name>
    <path>/root/include/opae/</path>
    <filename>utils_8h.html</filename>
    <includes id="types_8h" name="types.h" local="no" imported="no">opae/types.h</includes>
    <member kind="function">
      <type>const char *</type>
      <name>fpgaErrStr</name>
      <anchorfile>utils_8h.html</anchorfile>
      <anchor>a2a3fc0d8baf294d2da980ba544368b32</anchor>
      <arglist>(fpga_result e)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>vfio.h</name>
    <path>/root/include/opae/</path>
    <filename>vfio_8h.html</filename>
    <includes id="mem__alloc_8h" name="mem_alloc.h" local="no" imported="no">opae/mem_alloc.h</includes>
    <includes id="hash__map_8h" name="hash_map.h" local="no" imported="no">opae/hash_map.h</includes>
    <class kind="struct">opae_vfio_iova_range</class>
    <class kind="struct">opae_vfio_group</class>
    <class kind="struct">opae_vfio_sparse_info</class>
    <class kind="struct">opae_vfio_device_region</class>
    <class kind="struct">opae_vfio_device_irq</class>
    <class kind="struct">opae_vfio_device</class>
    <class kind="struct">opae_vfio_buffer</class>
    <class kind="struct">opae_vfio</class>
    <member kind="enumeration">
      <type></type>
      <name>opae_vfio_buffer_flags</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>adea46291b4fdd1878def41704f8acb5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>OPAE_VFIO_BUF_PREALLOCATED</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>adea46291b4fdd1878def41704f8acb5ca0ff03bd0152afe6ff864e32e78af2702</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_open</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a3d3ccbf4a24acbe8f3fc4d973ab46b1a</anchor>
      <arglist>(struct opae_vfio *v, const char *pciaddr)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_secure_open</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a8d6d1f473a26bc0fbe83e6894b8fb607</anchor>
      <arglist>(struct opae_vfio *v, const char *pciaddr, const char *token)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_region_get</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>afc30450c9b7ebc22709557cac5ab9181</anchor>
      <arglist>(struct opae_vfio *v, uint32_t index, uint8_t **ptr, size_t *size)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_buffer_allocate</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a63c468f18994a45604b372c31a1e26cf</anchor>
      <arglist>(struct opae_vfio *v, size_t *size, uint8_t **buf, uint64_t *iova)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_buffer_allocate_ex</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a782b8c30e14075d1c69af9c8e2998578</anchor>
      <arglist>(struct opae_vfio *v, size_t *size, uint8_t **buf, uint64_t *iova, int flags)</arglist>
    </member>
    <member kind="function">
      <type>struct opae_vfio_buffer *</type>
      <name>opae_vfio_buffer_info</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a30f0a317cbcfd12e6918dd11ec4c95f3</anchor>
      <arglist>(struct opae_vfio *v, uint8_t *vaddr)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_buffer_free</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ae2a60d829698d793ab9a4ad351d2596d</anchor>
      <arglist>(struct opae_vfio *v, uint8_t *buf)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_irq_enable</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a8fbe20ee37934b7a6bcebdd648a37191</anchor>
      <arglist>(struct opae_vfio *v, uint32_t index, uint32_t subindex, int event_fd)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_irq_unmask</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a97498e4d858a172906abfded4bce5199</anchor>
      <arglist>(struct opae_vfio *v, uint32_t index, uint32_t subindex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_irq_mask</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a94ee809a7040b9bec0a694ca65b0f3f7</anchor>
      <arglist>(struct opae_vfio *v, uint32_t index, uint32_t subindex)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>opae_vfio_irq_disable</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>abcce6f91ff59553d5893c4d3632cb87c</anchor>
      <arglist>(struct opae_vfio *v, uint32_t index, uint32_t subindex)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>opae_vfio_close</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a70fd22e64704c8f0090adb810d51fa83</anchor>
      <arglist>(struct opae_vfio *v)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_he_cache_cmd.h</name>
    <path>/root/samples/cxl_host_exerciser/</path>
    <filename>cxl__he__cache__cmd_8h.html</filename>
    <includes id="cxl__he__cmd_8h" name="cxl_he_cmd.h" local="yes" imported="no">cxl_he_cmd.h</includes>
    <includes id="cxl__host__exerciser_8h" name="cxl_host_exerciser.h" local="yes" imported="no">cxl_host_exerciser.h</includes>
    <includes id="he__cache__test_8h" name="he_cache_test.h" local="yes" imported="no">he_cache_test.h</includes>
    <class kind="class">host_exerciser::he_cache_cmd</class>
    <namespace>host_exerciser</namespace>
    <member kind="define">
      <type>#define</type>
      <name>UNUSED_PARAM</name>
      <anchorfile>cxl__he__cache__cmd_8h.html</anchorfile>
      <anchor>a6c7ba74ad57863d1342878d2c703e660</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_sig_handler</name>
      <anchorfile>cxl__he__cache__cmd_8h.html</anchorfile>
      <anchor>acc3f2f6f7a74a429062c2fdcfd98e86c</anchor>
      <arglist>(int)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_cache_thread</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a63cc59095cdb804e9d969dbd8445b35d</anchor>
      <arglist>(uint8_t *buf_ptr, uint64_t len)</arglist>
    </member>
    <member kind="variable">
      <type>volatile bool</type>
      <name>g_he_exit</name>
      <anchorfile>cxl__he__cache__cmd_8h.html</anchorfile>
      <anchor>a4f532053bdd1e2f076153f1063783b6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static volatile bool</type>
      <name>g_stop_thread</name>
      <anchorfile>cxl__he__cache__cmd_8h.html</anchorfile>
      <anchor>a35a6655d1abb4deeabbe0fdebc1bd68c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_he_cache_lpbk_cmd.h</name>
    <path>/root/samples/cxl_host_exerciser/</path>
    <filename>cxl__he__cache__lpbk__cmd_8h.html</filename>
    <includes id="cxl__host__exerciser_8h" name="cxl_host_exerciser.h" local="yes" imported="no">cxl_host_exerciser.h</includes>
    <includes id="he__cache__test_8h" name="he_cache_test.h" local="yes" imported="no">he_cache_test.h</includes>
    <class kind="class">host_exerciser::he_cache_lpbk_cmd</class>
    <namespace>host_exerciser</namespace>
  </compound>
  <compound kind="file">
    <name>cxl_he_cmd.h</name>
    <path>/root/samples/cxl_host_exerciser/</path>
    <filename>cxl__he__cmd_8h.html</filename>
    <includes id="cxl__he__cmd_8h" name="cxl_he_cmd.h" local="yes" imported="no">cxl_he_cmd.h</includes>
    <includes id="cxl__host__exerciser_8h" name="cxl_host_exerciser.h" local="yes" imported="no">cxl_host_exerciser.h</includes>
    <includes id="he__cache__test_8h" name="he_cache_test.h" local="yes" imported="no">he_cache_test.h</includes>
    <class kind="class">host_exerciser::he_cmd</class>
    <namespace>host_exerciser</namespace>
    <member kind="define">
      <type>#define</type>
      <name>HE_TEST_STARTED</name>
      <anchorfile>cxl__he__cmd_8h.html</anchorfile>
      <anchor>a28365d57577012efff59abf8a50e0d1a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HE_PRTEST_SCENARIO</name>
      <anchorfile>cxl__he__cmd_8h.html</anchorfile>
      <anchor>ae0324905f71f58f134e47401bd79b0e7</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_host_exerciser.cpp</name>
    <path>/root/samples/cxl_host_exerciser/</path>
    <filename>cxl__host__exerciser_8cpp.html</filename>
    <includes id="cxl__he__cache__cmd_8h" name="cxl_he_cache_cmd.h" local="yes" imported="no">cxl_he_cache_cmd.h</includes>
    <includes id="cxl__he__cache__lpbk__cmd_8h" name="cxl_he_cache_lpbk_cmd.h" local="yes" imported="no">cxl_he_cache_lpbk_cmd.h</includes>
    <includes id="cxl__host__exerciser_8h" name="cxl_host_exerciser.h" local="yes" imported="no">cxl_host_exerciser.h</includes>
    <member kind="function">
      <type>void</type>
      <name>he_sig_handler</name>
      <anchorfile>cxl__host__exerciser_8cpp.html</anchorfile>
      <anchor>acc3f2f6f7a74a429062c2fdcfd98e86c</anchor>
      <arglist>(int)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>cxl__host__exerciser_8cpp.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_host_exerciser.h</name>
    <path>/root/samples/cxl_host_exerciser/</path>
    <filename>cxl__host__exerciser_8h.html</filename>
    <includes id="he__cache__test_8h" name="he_cache_test.h" local="yes" imported="no">he_cache_test.h</includes>
    <class kind="union">host_exerciser::he_dfh</class>
    <class kind="union">host_exerciser::he_dsm_base</class>
    <class kind="union">host_exerciser::he_ctl</class>
    <class kind="union">host_exerciser::he_info</class>
    <class kind="union">host_exerciser::he_wr_num_lines</class>
    <class kind="union">host_exerciser::he_wr_byte_enable</class>
    <class kind="union">host_exerciser::he_wr_config</class>
    <class kind="union">host_exerciser::he_wr_addr_table_ctrl</class>
    <class kind="union">host_exerciser::he_wr_addr_table_data</class>
    <class kind="union">host_exerciser::he_rd_num_lines</class>
    <class kind="union">host_exerciser::he_rd_config</class>
    <class kind="union">host_exerciser::he_rd_addr_table_ctrl</class>
    <class kind="union">host_exerciser::he_rd_addr_table_data</class>
    <class kind="union">host_exerciser::he_err_status</class>
    <class kind="struct">host_exerciser::he_cache_dsm_status</class>
    <class kind="class">host_exerciser::host_exerciser</class>
    <class kind="struct">host_exerciser::he_dfh.__unnamed3__</class>
    <class kind="struct">host_exerciser::he_dsm_base.__unnamed6__</class>
    <class kind="struct">host_exerciser::he_ctl.__unnamed9__</class>
    <class kind="struct">host_exerciser::he_info.__unnamed12__</class>
    <class kind="struct">host_exerciser::he_wr_num_lines.__unnamed15__</class>
    <class kind="struct">host_exerciser::he_wr_byte_enable.__unnamed18__</class>
    <class kind="struct">host_exerciser::he_wr_config.__unnamed21__</class>
    <class kind="struct">host_exerciser::he_wr_addr_table_ctrl.__unnamed24__</class>
    <class kind="struct">host_exerciser::he_wr_addr_table_data.__unnamed27__</class>
    <class kind="struct">host_exerciser::he_rd_num_lines.__unnamed30__</class>
    <class kind="struct">host_exerciser::he_rd_config.__unnamed33__</class>
    <class kind="struct">host_exerciser::he_rd_addr_table_ctrl.__unnamed36__</class>
    <class kind="struct">host_exerciser::he_rd_addr_table_data.__unnamed39__</class>
    <class kind="struct">host_exerciser::he_err_status.__unnamed42__</class>
    <namespace>host_exerciser</namespace>
    <member kind="define">
      <type>#define</type>
      <name>MEM_TG_FEATURE_ID</name>
      <anchorfile>cxl__host__exerciser_8h.html</anchorfile>
      <anchor>a4736703a0eb0915314304d527acdf55d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MEM_TG_FEATURE_GUIDL</name>
      <anchorfile>cxl__host__exerciser_8h.html</anchorfile>
      <anchor>adba62009c92f202c4e9fc163301f26c3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MEM_TG_FEATURE_GUIDH</name>
      <anchorfile>cxl__host__exerciser_8h.html</anchorfile>
      <anchor>af5fc626e065828ba08501e0fb894b4c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3396b0a2f5f048347375ed7968bf936f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9047898033282bdb49d6657d7532fe2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa96f5f9ad616abbff98f88fd8d3749eec</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_L</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa994517cdf13de8a08a6fbcebc5d4b7eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_H</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa89326af32d3ac7eb75a59785c9ff1ee3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0547c29b0609234864bd196f04e9e101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaf9a43fa5b1790303e01e463acba370a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa712f6bdeef038f12b2943223e36a3702</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DSM_BASE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a9940ff171541be267a8cf74277844b32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CTL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aabd5379c2a756d7f71b6707abdfc88089</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INFO</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a63ab354d6a895d75d3ea2681c4a73c85</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_NUM_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a3b1b6c8c2fcbf57ad624de1077356fd4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_BYTE_ENABLE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248ab8930a0aeafce2a45089f931c333f205</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_CONFIG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248ab29c859275463170cc2bbb6e2fb0297c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_ADDR_TABLE_CTRL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a1adff58b128d7f57593d15ed67a54fc6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_ADDR_TABLE_DATA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a6d77f005b2d7e53be9d91ef0593eadda</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_NUM_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a344481e3ac1122c5e74bb59a7a934409</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_CONFIG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a8871c897146f0f2eaccd5077f6181472</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_ADDR_TABLE_CTRL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a321b37ff8fc5472a16b46da5631a6357</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_ADDR_TABLE_DATA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a300dc5ac788c901d20f421a24af98d7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_STATUS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248aca954c7b87a79cfff2d4d6f6c8e23eb1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_EXP_DATA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a2845aa2671b2f9a6235667aa6bc76ce0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a76ad387e8d2fb4639563c8e0272aa28c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a8cbbda04e37bbff5290a1ac024909baa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a2daca8866beee6f107128bb3bb23d2cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA3</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248ad8040e2bd695023243cd93bc540b1ae9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a872972b8d10fc0f8c38eb8bffc6a9537</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA5</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248aac19634eecfe2a67e232d483d5b07f7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA6</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a237210f2c6acd07b79645c8196c00d7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA7</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a8253c945a94fbb2dd2a45ae100ae6683</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_rd_opcode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>RD_LINE_I</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983a8615bca5d3aa6d3f844bd4f9c5f07e2e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>RD_LINE_S</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983a383b82e47fbec58b8a1b328d3be3ab35</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>RD_LINE_EM</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983a19aaf26804a3b10760a8347133f35937</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_wr_opcode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34df</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_LINE_I</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa9f9b0eb93b101bd67e617acc0e38a671</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_LINE_M</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfae5bc76f014dcbb30bf9377d595d738c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_PUSH_I</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa56f244dab07fac79a257306c87d88468</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_BARRIER_FRNCE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa8ed760c5fa905aaf07983d90158f4287</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_FLUSH_CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa5b357c8754747c7c1e90b2accb3c656a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_FLUSH_CL_HCOH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa28048131a444e8a31d7eaf6fbe710730</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_FLUSH_CL_DCOH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfad0dd56b280f7707ad776a5bcb48ff068</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_test_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_RD_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ad211a335379d690f23f9561bdd1e4aea</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_WR_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ae25066dd8d70eddfe8b4fb3f9771c92a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_RD_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979afc8a66080da962c0f662ff19ec429a0b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_WR_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ac7679022e0d491948f9439be39524836</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_RD_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979a237b3d2bbc5e51b408936c8879e0ef8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_WR_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979a67f17167a9df28c18e6125573d488bfa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_RD_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ac1f7a621fcee208489af401304507857</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_WR_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ab048697a1ec0a6ef71078d01dab304ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_target</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7e3f43915a1b25c35bb95863a87b1429</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_TARGET_HOST</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7e3f43915a1b25c35bb95863a87b1429a7f8712a5623abfd073edf0859128ab8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_TARGET_FPGA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7e3f43915a1b25c35bb95863a87b1429a1253eee565e44c426c6a34eadfaa0821</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_cxl_latency</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_LATENCY_NONE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4a92dbb5ac604c6744a28687c41d70d2c2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_RD_LATENCY</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4ad7b91477904acff1abcaba47619bf1a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_WR_LATENCY</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4a12ca9c21fbca0f9853b3e22b8578faaf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_RD_WR_LATENCY</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4a2553554934734ab40d12a7b9d72f025c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_bisa_support</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTMEM_BIAS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15aa4d06aaac0f2a27387b5fc9461a75ca3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_BIAS_NA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15a23ba95206a39722ed39c434ba4058846</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGAMEM_HOST_BIAS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15ad7cf526a75d59ecc712f3208c999a816</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGAMEM_DEVICE_BIAS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15a53f78272c191221a7b8d94466d9ee76a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_cxl_dev</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9fb8d62824f4c57e9f79549c21b80592</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_DEVICE0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9fb8d62824f4c57e9f79549c21b80592adab5ca75a104003678a8720b1c8ad694</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_DEVICE1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9fb8d62824f4c57e9f79549c21b80592a8c25ad936358399012804259bdf95788</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_addrtable_size</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE4096</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255ae12fcf27fe2cbc28ad429f8b1d469410</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE2048</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a8714dd9f55a88e0d1c8c5ab0822bb1fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE1024</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a6d094ba063a90590e5e1e753ef740f8c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE512</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a1ceb659f01e34a669743bd97fd793507</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE256</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a96f0b218e77aeaf8c7e78f18a050793d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE128</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a1fa2286f9d42bde78410951d6e70cb4f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE64</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a06c7e799fd21e744e4a83d6741559a0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE32</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a7956f4187e400ed80b82ccac3de88acb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE16</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255aeb0ac8e8b22329ca7c4b25278dd6af95</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a7f88fa903cae0f7c61e174de96b25073</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255ab943ac256680c233c1b70966c3d2077d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255ae665c4520f174df79c45d4d5fb9c5d2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_traffic_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a619d5ce8b001ff40b27522d1be17bc7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ENABLE_TRAFFIC_STAGE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a619d5ce8b001ff40b27522d1be17bc7ead216c542fdaa45ed1cc9e1c71c8df72b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SIP_SEQ_STAGE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a619d5ce8b001ff40b27522d1be17bc7eacbe77aa1bda4e8827b71c6bf5beedf4d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>HE_CACHE_AFU_ID</name>
      <anchorfile>cxl__host__exerciser_8h.html</anchorfile>
      <anchor>a72d02b17946176eac3d194950f7f02c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_TIMEOUT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4d5f99e45d6527c0a2e376d00329612a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_SLEEP_INVL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37f4cd6ba67fd8899f4218bba01b2b96</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3c15318d7b073d9b79b270aab66d37a3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>KB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4f74c9687586e00027a417a41a34d05b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae3548651828dac1b7e2adec934493f98</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>BUFFER_SIZE_2MB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a61b58598a908fe8bbf0db3dcfc8b706f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>BUFFER_SIZE_32KB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a0a736cbba6907deb25688e3082084e56</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>FPGA_32KB_CACHE_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af766e4d826b22e69e7b7ea58d14989dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>FPGA_2MB_CACHE_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a32fc2a41fc6262bcfac663c1e72547d7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>FPGA_512CACHE_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5c57d1dc4cb0cc9df9983da2498ceb0d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const double</type>
      <name>LATENCY_FACTOR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7181c0e391f834ad9cdda3e578176b3a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_test_modes</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9236a519bfc92f2e8a8cf399433bf29b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_targets</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa029adb74bb308968e3ab7ce3036610d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_bias</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a06c6876e392a9a89941d649a7162f8c9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_cxl_device</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7d0aa3fa8a77d912617648e824ab251f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>traffic_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ace4c7dc1522d8416d8c6162641bc8083</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::map&lt; uint32_t, uint32_t &gt;</type>
      <name>addrtable_size</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a55022611ecb6aba94c51e3620f980b7f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>he_cache_test.h</name>
    <path>/root/samples/cxl_host_exerciser/</path>
    <filename>he__cache__test_8h.html</filename>
    <includes id="core_8h" name="core.h" local="no" imported="no">opae/cxx/core.h</includes>
    <class kind="union">opae::afu_test::pcie_address</class>
    <class kind="class">opae::afu_test::command</class>
    <class kind="class">opae::afu_test::afu</class>
    <class kind="struct">opae::afu_test::pcie_address.fields</class>
    <namespace>opae</namespace>
    <namespace>opae::afu_test</namespace>
    <member kind="define">
      <type>#define</type>
      <name>FEATURE_DEV</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>ae526105af48b18cac6b7376a0da3c646</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_SIZE</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a0592dba56693fad79136250c11e5a7fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_HE_CACHE_DEVICE</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>aa1be1a2ea0e3c7d738ca265578184905</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>PROTECTION</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a3edd319ff92fe698be953178dc1cec70</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAP_HUGETLB</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a965bf45190323481c4a9d3256db5bd16</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAP_HUGE_SHIFT</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a5117db836c6b4faa5067a54550a9b85e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAP_2M_HUGEPAGE</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a7d6f3bf8995b6bd11b8748940d4ef87d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAP_1G_HUGEPAGE</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a3f7a3d1c89081e6dc5d7467e30573ddb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ADDR</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>ac9f31f726d2933782e2efda7136a25fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAGS_4K</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>ab255cd279851a87ef34b67cb0af7ae82</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAGS_2M</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>ad53f34fdc14d8d185326174e248dcff7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FLAGS_1G</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>af917db11a484b78ed5135fc6b5c3600d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>KiB</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>ae40428faaf55c32976e3e9bd81aa1f15</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MiB</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a66926c4792f38d56984ea477b5b6f45a</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>GiB</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a133ac64b3644f67e539bd861f3304540</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFL_CXL_CACHE_DSM_BASE</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a8275b12b6b8a1a7b5a02114a5e50619f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFL_CXL_CACHE_WR_ADDR_TABLE_DATA</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>aeb44d29251e2290eab7037feea413dfe</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFL_CXL_CACHE_RD_ADDR_TABLE_DATA</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a49cfbb3c7084eabd3317936a5a9bc908</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MATCHES_SIZE</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>ac205be2172292384dd687b5471a87eddaf55f3a57a0940f0e7803957d4bf84b90</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>buffer_allocate</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a737440f86e0bcdb1e4b6598f3541e75a</anchor>
      <arglist>(void **addr, uint64_t len, uint32_t numa_node)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>buffer_release</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a274404d5e2483f91198c791f3fb35044</anchor>
      <arglist>(void *addr, uint64_t len)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>sysfs_read_u64</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>af913d8c3d40a106a02ea4a7133fbbea9</anchor>
      <arglist>(const char *path, uint64_t *value)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>parse_match_int</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>a51c524d228e9ad342ed2e1a45efe741b</anchor>
      <arglist>(const char *s, regmatch_t m, T &amp;v, int radix=10)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>spdlog_levels</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>a86ee03f14e97bbb40476aacd85cc9868</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>sbdf_pattern</name>
      <anchorfile>he__cache__test_8h.html</anchorfile>
      <anchor>a1e352638b03b00b213cfc2de11524ee4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_mem_tg.cpp</name>
    <path>/root/samples/cxl_mem_tg/</path>
    <filename>cxl__mem__tg_8cpp.html</filename>
    <includes id="cxl__mem__tg_8h" name="cxl_mem_tg.h" local="yes" imported="no">cxl_mem_tg.h</includes>
    <includes id="cxl__tg__test_8h" name="cxl_tg_test.h" local="yes" imported="no">cxl_tg_test.h</includes>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>cxl__mem__tg_8cpp.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_mem_tg.h</name>
    <path>/root/samples/cxl_mem_tg/</path>
    <filename>cxl__mem__tg_8h.html</filename>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <class kind="union">cxl_mem_tg::mem_tg_ctl</class>
    <class kind="union">cxl_mem_tg::mem_tg_status</class>
    <class kind="union">cxl_mem_tg::mem_tg0_count</class>
    <class kind="union">cxl_mem_tg::mem_tg1_count</class>
    <class kind="class">cxl_mem_tg::cxl_mem_tg</class>
    <class kind="struct">cxl_mem_tg::mem_tg_ctl.__unnamed52__</class>
    <class kind="struct">cxl_mem_tg::mem_tg_status.__unnamed55__</class>
    <class kind="struct">cxl_mem_tg::mem_tg0_count.__unnamed58__</class>
    <class kind="struct">cxl_mem_tg::mem_tg1_count.__unnamed61__</class>
    <namespace>cxl_mem_tg</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae21fb3ab55ae8cb59106b87200d95a7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ab54b53736c1e8bd7876a69f2baef17b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ACTIVE</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3a7cbd86dff0180f632d573ef7e5a55057</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_TIMEOUT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3a8e63adddabc288e5498f2f4df66f0b27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ERROR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3ad6d2ca96385ead5241bb420e1e7ae64e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_PASS</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3aa0b11c71d367595e2a12c20197b44ce9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2ae50331c964539f5dd5c26007735d4ed3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_SEQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2ab7e300ecb7930052b948a653ddcaa184</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND_SEQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2af669ed7de6b50d6000674e9427d25182</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_ONE_HOT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2a92b5675ef5ee0396ead56070a486e58e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_FIXED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa7407f4233717e1d98f8a776390eb82af</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS7</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa657e027c0dafddd6b1507f6cce1194ad</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS15</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa17eadd59787408f22df400c34c6a1b0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS31</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaae5e9b7fe639f5d85614b3a1759b7c659</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_ROTATING</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa71564d8355c494d5a258cb5c26b4fc98</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea01597032eab3f760346c82fbc788ec5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eae1b3a54124834078d9db7680c3caa7bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea5debc24069234094ad6ba398106c65fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NEXT_AFU</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eaaa35301298cf8446efb61505e0798fa0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH_RSVD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea0664bd36175dd3d76875ebaa52980af6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SCRATCHPAD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea762eb285e7fc88be1e742b50533c5f9a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CTRL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea4756bce556cb643e0aa2c409f21f0c37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_STAT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eadd994b8040392db4076101038b417fe4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CLK_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea88d8b198bfb44c9dcad46aa0ab207970</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_WR_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eaf6d9bbd396589527f372a7b48f1a4d92</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CLK_FREQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea0fd9d1b39b6d59014ef01787ec3a4eef</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_VERSION</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a21e0b7931a80741eb0c73747a3fe4377</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_START</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9afbeb224f5b1d32e9b55b1ce8fcc9e32c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_LOOP_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad02dea59ee54f68764340c1c233829b1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aab71456eea9b0ccecabf1e0891954a0e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a411d376733b772443344588a2cac4458</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_REPEAT_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aa30e3bd5cec3f3cd992e6121bdbab09e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_REPEAT_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a90a29710d3b2099c1795a61d2622076e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURST_LENGTH</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab821353518367c2fcd2ab57a9c6e7840</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_CLEAR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a88ec11b1fb1a062f02451c50b0016949</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_IDLE_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab64e524dd316d881cc22757087d33439</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_LOOP_IDLE_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aed6f3dfb644b6e070fefcaf16e2e1d11</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_WR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ae286a4c714c99b08b1002ee3ed1a587f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_WR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a7709164257a8f5718596b5ab2c472ad4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RETURN_TO_START_ADDR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad39fc44c8edf2e15dad6d831aa6fe819</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_ADDR_INCR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9acf979f868802b88029d479d18111f465</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_RD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a201705870a4fa204363246058cb2076e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_RD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a9ce33fc8beaf69c76c4ce3776805d1c4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PASS</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9af157488b545676cc548397859997b02a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a0b00917713940ec859e81f0a86dc562f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a6c4409c3d1f07c62bcd75fd3b05467fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a1382e3f82472ef4eb8dcbbb17e377285</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9afaf1344c0006254f6f4947ed9728bee6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aa10c574348ce256ce6987b453ab10f70</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a810379cbef2d3d1791fb853c8a7d3792</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab3188e1e50ec3ed8a6eed8dafd7e3bfe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_COMPLETE</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a397a7f61b4c32a69262d9db33ec35fda</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_INVERT_BYTEEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad69950385bd5a8260e9262d3473d65e9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RESTART_DEFAULT_TRAFFIC</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab7a46ab46b32d143ea74bd68d6e1708c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_USER_WORM_EN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a138084b5a721464cfb312d4d675410bf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_BYTEEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a50c1a9359d982b1f90a076434a82062b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TIMEOUT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9acd7be04dcb9016fd18b247be44487e93</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_DATA_GEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aad2624687ce89631f8307ceb034b86e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_BYTEEN_GEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a6d207aa57db45407a6d02d79c5a0f7e3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RDATA_WIDTH</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a8f351711d6e1137882a8248bb4536fb8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ERROR_REPORT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab50773a7b547928a51e3ebb30a00b0fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PNF</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a75cfc6f2b701f77e83465a606133b241</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_EXPECTED_DATA</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a0452589b12a20397977344288abae82f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_READ_DATA</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a26ef0000b2f002b1fe653aeb6d2677ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_SEED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad19fda5e987a2fae893421a426f6766d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab95729a6c6553c58502a0f1306f1b004</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PPPG_SEL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9adf412af7a4ff3287fe327afee84e348d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a3c528143f8a6d19ca435a0f458cc29b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_FIELD_RELATIVE_FREQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aece950f87a9fd52ec05f2326d00499fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_FIELD_MSB_INDEX</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aeb51b5206be439a4f73216c8737b15f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURSTLENGTH_OVERFLOW_OCCURRED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a4373da92590dd537b2f82603b269fd6f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURSTLENGTH_FAIL_ADDR_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a8382e8133ee151e0c00c1993011deeaa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURSTLENGTH_FAIL_ADDR_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a0e3799ef4889e3dae68d80346fee6d61</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WORM_MODE_TARGETTED_DATA</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a533db7920ef8344cb657bc340e903227</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>AFU_ID</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a7187f3c940e7b0bede33c18321170c26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MEM_TG_TEST_TIMEOUT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a7488ec5ac28f6679e818452cb7b347ec</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TEST_SLEEP_INVL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae2fec4e55f5f8fbfb78d0c38ec1e3289</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TG_CTRL_CLEAR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a7e452dc886e26cc1525c4533efbfd1c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TG_SLEEP</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>adf88f83a1ea74efb4228b2e7de8d8ae8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TG_FREQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ad72ec47242a649d327e0bde858f7f2b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>tg_pattern</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a4adbfbc2d17a8a0c2a818f243385336a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const int</type>
      <name>MEM_TG_CFG_OFFSET</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>afb32111a410e98fdae3118bc9a2c9277</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>cxl_tg_test.h</name>
    <path>/root/samples/cxl_mem_tg/</path>
    <filename>cxl__tg__test_8h.html</filename>
    <includes id="cxl__mem__tg_8h" name="cxl_mem_tg.h" local="yes" imported="no">cxl_mem_tg.h</includes>
    <class kind="class">cxl_mem_tg::cxl_tg_test</class>
    <namespace>cxl_mem_tg</namespace>
    <member kind="define">
      <type>#define</type>
      <name>CXL_TG_BW_FACTOR</name>
      <anchorfile>cxl__tg__test_8h.html</anchorfile>
      <anchor>a89728a0d44ef6bdf3ee05e9109597c8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>cxl__tg__test_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>int_to_hex</name>
      <anchorfile>cxl__tg__test_8h.html</anchorfile>
      <anchor>a6a1f7385a7cb829ffeb95df6b2c234f9</anchor>
      <arglist>(X x)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>ddr.h</name>
    <path>/root/samples/dummy_afu/</path>
    <filename>ddr_8h.html</filename>
    <includes id="dummy__afu_8h" name="dummy_afu.h" local="yes" imported="no">dummy_afu.h</includes>
    <class kind="class">dummy_afu::ddr_test</class>
    <namespace>dummy_afu</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>ddr_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>check_stat</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>aef855a00ce0a89e5c9d10373084438ad</anchor>
      <arglist>(std::shared_ptr&lt; spdlog::logger &gt; logger, const char *name, T stat)</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>dummy_afu.cpp</name>
    <path>/root/samples/dummy_afu/</path>
    <filename>dummy__afu_8cpp.html</filename>
    <includes id="samples_2dummy__afu_2mmio_8h" name="mmio.h" local="yes" imported="no">mmio.h</includes>
    <includes id="lpbk_8h" name="lpbk.h" local="yes" imported="no">lpbk.h</includes>
    <includes id="ddr_8h" name="ddr.h" local="yes" imported="no">ddr.h</includes>
    <includes id="dummy__afu_8h" name="dummy_afu.h" local="yes" imported="no">dummy_afu.h</includes>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>dummy__afu_8cpp.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>AFU_ID</name>
      <anchorfile>dummy__afu_8cpp.html</anchorfile>
      <anchor>a67fa09f58e3e39d9604e1422af718ad5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>dummy_afu.h</name>
    <path>/root/samples/dummy_afu/</path>
    <filename>dummy__afu_8h.html</filename>
    <includes id="events_8h" name="events.h" local="no" imported="no">opae/cxx/core/events.h</includes>
    <includes id="shared__buffer_8h" name="shared_buffer.h" local="no" imported="no">opae/cxx/core/shared_buffer.h</includes>
    <class kind="union">dummy_afu::afu_dfh</class>
    <class kind="union">dummy_afu::mem_test_ctrl</class>
    <class kind="union">dummy_afu::ddr_test_ctrl</class>
    <class kind="union">dummy_afu::ddr_test_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank0_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank1_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank2_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank3_stat</class>
    <class kind="class">dummy_afu::dummy_afu</class>
    <class kind="struct">dummy_afu::afu_dfh.__unnamed65__</class>
    <class kind="struct">dummy_afu::mem_test_ctrl.__unnamed68__</class>
    <class kind="struct">dummy_afu::ddr_test_ctrl.__unnamed71__</class>
    <class kind="struct">dummy_afu::ddr_test_stat.__unnamed74__</class>
    <class kind="struct">dummy_afu::ddr_test_bank0_stat.__unnamed77__</class>
    <class kind="struct">dummy_afu::ddr_test_bank1_stat.__unnamed80__</class>
    <class kind="struct">dummy_afu::ddr_test_bank2_stat.__unnamed83__</class>
    <class kind="struct">dummy_afu::ddr_test_bank3_stat.__unnamed86__</class>
    <namespace>dummy_afu</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ac06b3900bd7a2d4ba2c3f19ea20f622d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>aa01df4dcd37386cbf0800129c6f3183c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba2e87f72819846d7a53092b360755c7cd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_L</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba7bc0113085a1537b87b9533c1d1b93e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_H</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046baa48ec3d788fa335ebc4bb4469bafc48c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NEXT_AFU</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba222f7920be0e34b7907643cef6889fd4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH_RSVD</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba1a9e79d25022f4af20b0fb5347f7a76e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SCRATCHPAD</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bac7cab4d551fa9a04a732ed2d48f121a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MMIO_TEST_SCRATCHPAD</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba2dd1000f0770dcda833f518caaef2139</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_CTRL</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba33b12307658c4d48d2e3e830f31c8d4a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bae4462a80036a5ac3aa3cada2418bc2a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_SRC_ADDR</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba50907b523a57b88d331c2224e057f8dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_DST_ADDR</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba23997995c982f0e6a26fea04efa07c37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_CTRL</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046baf36a9e642b0ddac03c1f9938fb6789c3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046baab48d50e6990ff1a2a76fe12bd189a51</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK0_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bada2d999413f409ffcf8f73006412f96f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK1_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba47ea5964b0401582a714a12a42998c83</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK2_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba0edc832f3a1cb6b2be9e63b044a1570a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK3_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bac7b6c66db9078f216f384bcdc09ae8d5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>lpbk.h</name>
    <path>/root/samples/dummy_afu/</path>
    <filename>lpbk_8h.html</filename>
    <includes id="dummy__afu_8h" name="dummy_afu.h" local="yes" imported="no">dummy_afu.h</includes>
    <class kind="class">dummy_afu::lpbk_test</class>
    <namespace>dummy_afu</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>lpbk_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>afu_json_info.h</name>
    <path>/root/samples/hello_afu/sw/</path>
    <filename>afu__json__info_8h.html</filename>
    <member kind="define">
      <type>#define</type>
      <name>AFU_ACCEL_NAME</name>
      <anchorfile>afu__json__info_8h.html</anchorfile>
      <anchor>accb99da6fc6b79f951fe617df1db52cf</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_ACCEL_UUID</name>
      <anchorfile>afu__json__info_8h.html</anchorfile>
      <anchor>a70bd922abe682ef5aad25f5338e84261</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_IMAGE_POWER</name>
      <anchorfile>afu__json__info_8h.html</anchorfile>
      <anchor>aaa8ec410230ef8bfcfeec6e49f7c4772</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_TOP_IFC</name>
      <anchorfile>afu__json__info_8h.html</anchorfile>
      <anchor>ad62d06d59650b274df8c48f252ca5abb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hello_afu.c</name>
    <path>/root/samples/hello_afu/sw/</path>
    <filename>hello__afu_8c.html</filename>
    <includes id="enum_8h" name="enum.h" local="no" imported="no">opae/enum.h</includes>
    <includes id="access_8h" name="access.h" local="no" imported="no">opae/access.h</includes>
    <includes id="include_2opae_2mmio_8h" name="mmio.h" local="no" imported="no">opae/mmio.h</includes>
    <includes id="properties_8h" name="properties.h" local="no" imported="no">opae/properties.h</includes>
    <includes id="utils_8h" name="utils.h" local="no" imported="no">opae/utils.h</includes>
    <includes id="afu__json__info_8h" name="afu_json_info.h" local="yes" imported="no">afu_json_info.h</includes>
    <class kind="struct">cache_line</class>
    <member kind="define">
      <type>#define</type>
      <name>UNUSED_PARAM</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a6c7ba74ad57863d1342878d2c703e660</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HELLO_AFU_ID</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a607f30325c09b7bf4c02666214c1840c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCRATCH_REG</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>aa214bb53cfaaf6b164ece6df54f30347</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCRATCH_VALUE</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a7bbcd84554dc100a3af3c90866c517a6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>SCRATCH_RESET</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>af5dcd1ca8faac7e95459bbb3324b8b37</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BYTE_OFFSET</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a978eddbb9cac48d7902de5c7a7603e4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_DFH_REG</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a7e11a6bddfa9cc95fe37933f2faf8810</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_ID_LO</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a5020d54a46140b0c7f167ea9f06224ff</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_ID_HI</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a9e32a9ba987300033ecd00f565643407</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_NEXT</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a45a6019e3d2dc27d7db71b00d6dd6089</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_RESERVED</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a475a2a2461469d50e8c5bd20d8b30408</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ON_ERR_GOTO</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a7635d12a96b25dd1760b4bc44130eed9</anchor>
      <arglist>(res, label, desc)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ASSERT_GOTO</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>aff093d1ec7fa753b88889e1a94222d05</anchor>
      <arglist>(condition, label, desc)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>usleep</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a300ec1c0f7befcbb990985d36d56d8c9</anchor>
      <arglist>(unsigned)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_err</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>aa11f434898b86d619a5ceb6a5d7bf066</anchor>
      <arglist>(const char *s, fpga_result res)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static int</type>
      <name>s_error_count</name>
      <anchorfile>hello__afu_8c.html</anchorfile>
      <anchor>a6b8635735dd6b01cbc0524ca3d299d88</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hello_events.c</name>
    <path>/root/samples/hello_events/</path>
    <filename>hello__events_8c.html</filename>
    <includes id="fpga_8h" name="fpga.h" local="no" imported="no">opae/fpga.h</includes>
    <class kind="struct">ras_inject_error</class>
    <class kind="union">ras_inject_error.__unnamed88__</class>
    <class kind="struct">ras_inject_error.__unnamed88__.__unnamed90__</class>
    <member kind="define">
      <type>#define</type>
      <name>FME_SYSFS_INJECT_ERROR</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>aeed061055cfe359757fdacf8a96a0a81</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ON_ERR_GOTO</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a7635d12a96b25dd1760b4bc44130eed9</anchor>
      <arglist>(res, label, desc)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>GETOPT_STRING</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>acc2c9cc08f0f7b8f21f835d3fa0f6435</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>usleep</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a300ec1c0f7befcbb990985d36d56d8c9</anchor>
      <arglist>(unsigned)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_err</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>aa11f434898b86d619a5ceb6a5d7bf066</anchor>
      <arglist>(const char *s, fpga_result res)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>inject_ras_fatal_error</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a21ab990ba21e75dd40821f08f21c9111</anchor>
      <arglist>(fpga_token fme_token, uint8_t err)</arglist>
    </member>
    <member kind="function">
      <type>void *</type>
      <name>error_thread</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a23584cf041247d3b877d3fa37b6d1ca7</anchor>
      <arglist>(void *arg)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>help</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a0bed8474bd33a912769360766f6b10d4</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>parse_args</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a3a1b6f72679286eae6f9eec936cc5533</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>find_fpga</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a4ea1297aa11758c9d70172b2ee05b950</anchor>
      <arglist>(fpga_properties device_filter, fpga_token *fpga, uint32_t *num_matches)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hello_fpga.c</name>
    <path>/root/samples/hello_fpga/</path>
    <filename>hello__fpga_8c.html</filename>
    <includes id="fpga_8h" name="fpga.h" local="no" imported="no">opae/fpga.h</includes>
    <class kind="struct">cache_line</class>
    <class kind="struct">config</class>
    <member kind="define">
      <type>#define</type>
      <name>TEST_TIMEOUT</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a9fd0cdfdda29532a444c52dd9e1c1d1f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CL</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>acda3b44286ccc5e1924198027f28a8c5</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LOG2_CL</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>adf30d050a3289b6d043824dfd97c81f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MB</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a44d2b171cc92225ec0a76ef70fc9b531</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CACHELINE_ALIGNED_ADDR</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>ad3542630f57f4e7073dbd18dd41c9e9a</anchor>
      <arglist>(p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LPBK1_BUFFER_SIZE</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>aaa5caa9ebd221b79ab0d6f8c5568e855</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LPBK1_BUFFER_ALLOCATION_SIZE</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a680d7d8ff687719f3dfd4a1cf7d04cfe</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>LPBK1_DSM_SIZE</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a01a171b6816e2e3681ee0bea0fd74b70</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SRC_ADDR</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a3346cda33cdc35d5d5b42cd489c75aa0</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_DST_ADDR</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a8fc5dce1142497274a335a84489be3d4</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_CTL</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>acbe346f9bf5608488adb1b6ad68809b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATUS1</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>ac514d59efcfed9b7fb07a3c838f09503</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_CFG</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a21f1970d1fcd3dee94bf0ab5aeb54fbd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_NUM_LINES</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>ab1c7049b0e183020bdabcaaec8f20e49</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DSM_STATUS_TEST_COMPLETE</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>ac77542e2d0512964e9a065d647b14639</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_AFU_DSM_BASEL</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a4502164cd63bc6b91df80c619468e479</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NLB0_AFUID</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a326496be663d917e61f22100670026b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>N3000_AFUID</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a782f8715b2fd7e1919759a25ee578d44</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_NLB0_UUID_H</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>acb55d7e03697cb4b95812725b762f653</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_NLB0_UUID_L</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a46ed725f81a748bec841dd6216f8ac21</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ON_ERR_GOTO</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a7635d12a96b25dd1760b4bc44130eed9</anchor>
      <arglist>(res, label, desc)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>GETOPT_STRING</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>acc2c9cc08f0f7b8f21f835d3fa0f6435</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>usleep</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a300ec1c0f7befcbb990985d36d56d8c9</anchor>
      <arglist>(unsigned)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_err</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>aa11f434898b86d619a5ceb6a5d7bf066</anchor>
      <arglist>(const char *s, fpga_result res)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>help</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a0bed8474bd33a912769360766f6b10d4</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>parse_args</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a3a1b6f72679286eae6f9eec936cc5533</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>find_fpga</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>ae703894d2d572a2c40252494470ab1f1</anchor>
      <arglist>(fpga_properties device_filter, fpga_guid afu_guid, fpga_token *accelerator_token, uint32_t *num_matches_accelerators)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>probe_for_ase</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a888b04133611aa43ed28c4e4f9f319ae</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>find_nlb_n3000</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>af5efd740b98ff6882ecbdde289da7b85</anchor>
      <arglist>(fpga_handle accelerator_handle, uint64_t *afu_baddr)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="variable">
      <type>struct config</type>
      <name>config</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a6021fefa0bf488ddb29487b0b9d49979</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>host_exerciser.cpp</name>
    <path>/root/samples/host_exerciser/</path>
    <filename>host__exerciser_8cpp.html</filename>
    <includes id="host__exerciser__lpbk_8h" name="host_exerciser_lpbk.h" local="yes" imported="no">host_exerciser_lpbk.h</includes>
    <includes id="host__exerciser__mem_8h" name="host_exerciser_mem.h" local="yes" imported="no">host_exerciser_mem.h</includes>
    <includes id="host__exerciser_8h" name="host_exerciser.h" local="yes" imported="no">host_exerciser.h</includes>
    <member kind="function">
      <type>void</type>
      <name>he_sig_handler</name>
      <anchorfile>host__exerciser_8cpp.html</anchorfile>
      <anchor>acc3f2f6f7a74a429062c2fdcfd98e86c</anchor>
      <arglist>(int)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>host__exerciser_8cpp.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>host_exerciser.h</name>
    <path>/root/samples/host_exerciser/</path>
    <filename>host__exerciser_8h.html</filename>
    <includes id="events_8h" name="events.h" local="no" imported="no">opae/cxx/core/events.h</includes>
    <includes id="shared__buffer_8h" name="shared_buffer.h" local="no" imported="no">opae/cxx/core/shared_buffer.h</includes>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <class kind="union">host_exerciser::he_dfh</class>
    <class kind="union">host_exerciser::he_dsm_basel</class>
    <class kind="union">host_exerciser::he_dsm_baseh</class>
    <class kind="union">host_exerciser::he_num_lines</class>
    <class kind="union">host_exerciser::he_ctl</class>
    <class kind="union">host_exerciser::he_cfg</class>
    <class kind="union">host_exerciser::he_inact_thresh</class>
    <class kind="union">host_exerciser::he_interrupt0</class>
    <class kind="union">host_exerciser::he_swtest_msg</class>
    <class kind="union">host_exerciser::he_status0</class>
    <class kind="union">host_exerciser::he_status1</class>
    <class kind="union">host_exerciser::he_error</class>
    <class kind="union">host_exerciser::he_stride</class>
    <class kind="struct">host_exerciser::he_dsm_status</class>
    <class kind="struct">host_exerciser::MapKeyComparator</class>
    <class kind="class">host_exerciser::host_exerciser</class>
    <class kind="struct">host_exerciser::he_dfh.__unnamed94__</class>
    <class kind="struct">host_exerciser::he_ctl.__unnamed106__</class>
    <class kind="struct">host_exerciser::he_dsm_basel.__unnamed97__</class>
    <class kind="struct">host_exerciser::he_dsm_baseh.__unnamed100__</class>
    <class kind="struct">host_exerciser::he_num_lines.__unnamed103__</class>
    <class kind="struct">host_exerciser::he_cfg.__unnamed109__</class>
    <class kind="struct">host_exerciser::he_inact_thresh.__unnamed112__</class>
    <class kind="struct">host_exerciser::he_interrupt0.__unnamed115__</class>
    <class kind="struct">host_exerciser::he_swtest_msg.__unnamed118__</class>
    <class kind="struct">host_exerciser::he_status0.__unnamed121__</class>
    <class kind="struct">host_exerciser::he_status1.__unnamed124__</class>
    <class kind="struct">host_exerciser::he_error.__unnamed127__</class>
    <class kind="struct">host_exerciser::he_stride.__unnamed130__</class>
    <namespace>host_exerciser</namespace>
    <member kind="enumvalue">
      <name>HE_DFH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa96f5f9ad616abbff98f88fd8d3749eec</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_L</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa994517cdf13de8a08a6fbcebc5d4b7eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_H</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa89326af32d3ac7eb75a59785c9ff1ee3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0547c29b0609234864bd196f04e9e101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaf9a43fa5b1790303e01e463acba370a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa712f6bdeef038f12b2943223e36a3702</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aadacb6d1d03994d938c33005ab3eef726</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa33453ac7a61a0e1b3ede18bff3cef947</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DSM_BASEL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0f0436ad7cb26fd22fbdf4fd3264f309</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DSM_BASEH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0351bf59f34601c8d81894f0897e6cde</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SRC_ADDR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa7e7c47794d23f20fef4489d3b71c0122</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DST_ADDR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa14be24d91af1512e70b6ca78c7694101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_NUM_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aab5e8a172abefb6c61695f517de783395</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CTL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aabd5379c2a756d7f71b6707abdfc88089</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CFG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaf48cdfa6534904b034ff9641dab6b34f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INACT_THRESH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa12797e3cf3c370e7151ec1b257ad82e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INTERRUPT0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaaf7d586e7208f7ea7b598601d02e3d0e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SWTEST_MSG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa04279c71f1e9c5a535cfe8515bb52f12</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_STATUS0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa196cabd4f8f2a096cf6312f2c719d409</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_STATUS1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa5bd3e4e22deae78be590cf3c51949174</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aad24b45a44d85621225d9f8215457623f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_STRIDE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa878952b31118fec2cb65f1f0d039d8f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INFO0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa2f434e10b654a82eaad847a090e164ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>host_exe_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbca</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_LPBK1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaa6631ee58c265dc79c90c5c2b526b9192</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_READ</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaa1ea63bb1d444f6bed6a46303b90ab7be</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_WRITE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaab65e84c23fdb71a2b385ce3593b78b04</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_TRPUT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaa7a915772549100ded4d3a36df386fbe3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_req_len</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0aab7592a31c4b72cf471ea23e0d8367d9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0ace806856e806e5cc8b9c78b91a328d4c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0a08cc11d8a56708a49ca49a13932cd698</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0aff641659fe8c6a51abd942fae0ccd2e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_16</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0affdb50fd72b4b6b60d754f421f7f35f7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_atomic_func</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_OFF</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84aca03793e71244ba100cbbb44e4523979</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_FADD_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84a9e355a7bceadb9f9ffb0786d26670926</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_FADD_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84af631fb94c0d4adf7d6f3567d3293c3e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_SWAP_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84a4e635662561727a4d1f8903658862503</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_SWAP_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84a9af1bd83b6aca61b4422036cbef2da34</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_CAS_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84abb08f60e74724813b14692184afec582</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_CAS_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84aafb548060eed16af2fd9c9ad87b09e01</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_encoding</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_DEFAULT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456a043d79b53e107d6653db3a845fb66556</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_DM</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456a1d5ec8224b5922cee68164f7e7fbb401</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_PU</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456afff5d203a4ffba51d8d78231ba8c4884</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_RANDOM</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456a05f5799651cb2b0d71bca3615e22802d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_test_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abe4c0db8055b8c2abb6cd96f72bc1890</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_TEST_ROLLOVER</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abe4c0db8055b8c2abb6cd96f72bc1890a3e8ad9fc752d1b50e0f0725b87c410b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_TEST_TERMINATION</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abe4c0db8055b8c2abb6cd96f72bc1890a90f94357ec7daa6c0d2a473deded4a0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_TIMEOUT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4d5f99e45d6527c0a2e376d00329612a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_SLEEP_INVL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37f4cd6ba67fd8899f4218bba01b2b96</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3c15318d7b073d9b79b270aab66d37a3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>KB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4f74c9687586e00027a417a41a34d05b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae3548651828dac1b7e2adec934493f98</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>LOG2_CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aaf2fdab399a1e5df1decd13aaff46648</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const size_t</type>
      <name>LPBK1_DSM_SIZE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc614c0a041ef93b39806f91153b822f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const size_t</type>
      <name>LPBK1_BUFFER_SIZE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab4b46ce5277ad6a137e9d753cc54f7c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const size_t</type>
      <name>LPBK1_BUFFER_ALLOCATION_SIZE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4a48249a14d845c7ad6c2913209cbfe5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_modes</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9dc7a132365a9a2e883166de316d3579</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t, MapKeyComparator &gt;</type>
      <name>he_req_cls_len</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a2ea0bead4a89cf726adea06214201dc3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_req_atomic_func</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3384e6ab4680f2301d6a9db29c196edf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_req_encoding</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5e3a5f37a59e69f383e1d72cae05e9f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_test_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7891643f1c1433756f12022500d93dff</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>interleave_help</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4ca77ad5a2b9a9762842871600fbf1d8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>host_exerciser_cmd.h</name>
    <path>/root/samples/host_exerciser/</path>
    <filename>host__exerciser__cmd_8h.html</filename>
    <includes id="host__exerciser_8h" name="host_exerciser.h" local="yes" imported="no">host_exerciser.h</includes>
    <class kind="class">host_exerciser::host_exerciser_cmd</class>
    <namespace>host_exerciser</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>host__exerciser__cmd_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_sig_handler</name>
      <anchorfile>host__exerciser__cmd_8h.html</anchorfile>
      <anchor>acc3f2f6f7a74a429062c2fdcfd98e86c</anchor>
      <arglist>(int)</arglist>
    </member>
    <member kind="variable">
      <type>volatile bool</type>
      <name>g_he_exit</name>
      <anchorfile>host__exerciser__cmd_8h.html</anchorfile>
      <anchor>a4f532053bdd1e2f076153f1063783b6e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>host_exerciser_lpbk.h</name>
    <path>/root/samples/host_exerciser/</path>
    <filename>host__exerciser__lpbk_8h.html</filename>
    <includes id="host__exerciser_8h" name="host_exerciser.h" local="yes" imported="no">host_exerciser.h</includes>
    <includes id="host__exerciser__cmd_8h" name="host_exerciser_cmd.h" local="yes" imported="no">host_exerciser_cmd.h</includes>
    <class kind="class">host_exerciser::host_exerciser_lpbk</class>
    <namespace>host_exerciser</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>host__exerciser__lpbk_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>LPBK_AFU_ID</name>
      <anchorfile>host__exerciser__lpbk_8h.html</anchorfile>
      <anchor>afd5fda82b5bc6a4e2af9d0cf49445f78</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>host_exerciser_mem.h</name>
    <path>/root/samples/host_exerciser/</path>
    <filename>host__exerciser__mem_8h.html</filename>
    <includes id="host__exerciser_8h" name="host_exerciser.h" local="yes" imported="no">host_exerciser.h</includes>
    <class kind="class">host_exerciser::host_exerciser_mem</class>
    <namespace>host_exerciser</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>host__exerciser__mem_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>MEM_AFU_ID</name>
      <anchorfile>host__exerciser__mem_8h.html</anchorfile>
      <anchor>ac1c0a3bf0dbf8e5ca312768b806c5b6d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi.cpp</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi_8cpp.html</filename>
    <includes id="hssi__afu_8h" name="hssi_afu.h" local="yes" imported="no">hssi_afu.h</includes>
    <includes id="hssi__10g__cmd_8h" name="hssi_10g_cmd.h" local="yes" imported="no">hssi_10g_cmd.h</includes>
    <includes id="hssi__100g__cmd_8h" name="hssi_100g_cmd.h" local="yes" imported="no">hssi_100g_cmd.h</includes>
    <includes id="hssi__200g__400g__cmd_8h" name="hssi_200g_400g_cmd.h" local="yes" imported="no">hssi_200g_400g_cmd.h</includes>
    <includes id="hssi__pkt__filt__10g__cmd_8h" name="hssi_pkt_filt_10g_cmd.h" local="yes" imported="no">hssi_pkt_filt_10g_cmd.h</includes>
    <includes id="hssi__pkt__filt__100g__cmd_8h" name="hssi_pkt_filt_100g_cmd.h" local="yes" imported="no">hssi_pkt_filt_100g_cmd.h</includes>
    <member kind="function">
      <type>void</type>
      <name>sig_handler</name>
      <anchorfile>hssi_8cpp.html</anchorfile>
      <anchor>a83e1379f5adf4452f72fcc0be266a7b5</anchor>
      <arglist>(int signum)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>hssi_8cpp.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="variable">
      <type>hssi_afu</type>
      <name>app</name>
      <anchorfile>hssi_8cpp.html</anchorfile>
      <anchor>a7ccd62f1083f322e0d1dfaa9c23ecb88</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi_100g_cmd.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__100g__cmd_8h.html</filename>
    <includes id="hssi__cmd_8h" name="hssi_cmd.h" local="yes" imported="no">hssi_cmd.h</includes>
    <class kind="class">hssi_100g_cmd</class>
    <class kind="struct">DFH</class>
    <class kind="struct">ctrl_config</class>
    <class kind="struct">perf_data</class>
    <class kind="union">DFH.__unnamed132__</class>
    <class kind="struct">DFH.__unnamed132__.__unnamed134__</class>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SCRATCH</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a05235435e367aa50d75498e0132c976c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_BLOCK_ID</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a602a750c11a9a1d76a93d09e07d5b818</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_PKT_SIZE</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a0a05a8264291bfde19dd9e440fe37c19</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_CTRL0</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>af5c76870891f7c381cfdaa22ffbc35d2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_CTRL1</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a7554dc134c218d9179b00f5b4d7ad325</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_DST_ADDR_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a9e0b4232ed9782703da9fef62fa93e9c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_DST_ADDR_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a8a766b54f6df2c0067d1685249a90014</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SRC_ADDR_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ae7f7c54caf64237c1f718e943aed46de</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SRC_ADDR_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ac216de60fb702c641fbbc4a56f03ebc0</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_COUNT</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a11092ef81e45b10a893afb7d17ffbe08</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_MLB_RST</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a5d96623e1c28ac8f4cc57c9d1b7b7aa2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_COUNT</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a142064f22375847e503fe45dd6be3c62</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_CTRL</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a55fbc642000a43fc2dbbeca1c9b8577d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_TX_CNT_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a094ea00f75f1e5b6ff883969b1156adb</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_TX_CNT_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a7ebebec43c003cbaaa44cc9c41dcdd0f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_RX_CNT_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ad85e6d492f6f8a94ed1c54e7b1063880</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_RX_CNT_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a2628f7d3344635d3561093753cbdbd3a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_RX_GD_CNT_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a102bb1584ecbab4395315317c25a3795</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STATS_RX_GD_CNT_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a3c1e564e8378cdf8c648dd7550d21c22</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_START_TIMESTAMP_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ae58ce65fed70134a4bc5a86e4e69d69f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_START_TIMESTAMP_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>aa8717185f9bf2892b1c1471ddd198587</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_END_TIMESTAMP_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a5b4e27675ba9ac664618878dc1ef3609</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_END_TIMESTAMP_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a06440f4d60f8b2496bf0a0fabb98ef8d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_START_TIMESTAMP_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ac6558a84e1b087648574af0663c82c83</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_START_TIMESTAMP_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a62f6d942b39a975778628d6f5a312f6b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_END_TIMESTAMP_LO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ae7424ee4fb40df6f1a308260407b15b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_END_TIMESTAMP_HI</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>aec9082ad39ed56cd9cfefe83b55d5210</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>STOP_BITS</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>abd1123d1a3a1f52c36d99b27e993e282</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ZERO</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ac328e551bde3d39b6d7b8cc9e048d941</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ONE</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a206b6f5362e56b51ca957635350b70b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SHIFT</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a7a95c3d359fa7be9791278583466a35c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CONV_SEC</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>aa5a6207365d657a308ecacca1d66e2e3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DATA_CNF_PKT_NUM</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a74bb106424b810f839713e04798b9004</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DATA_CNF_CONTINUOUS_MODE</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>aaefe7551a85b7b26574e644830962bce</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DATA_CNF_FIXED_MODE</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a1cec299bc80c083399406f9111d3e763</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CURSOR_UP</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a70efb5dccaa6fceb0309f24ca94c7a11</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CURSOR_DOWN</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ab222bed94f6c29e3c1970935f0d6e5e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_PORT</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a401e1a60d6381236216b6a130a6685bd</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi_10g_cmd.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__10g__cmd_8h.html</filename>
    <includes id="hssi__cmd_8h" name="hssi_cmd.h" local="yes" imported="no">hssi_cmd.h</includes>
    <class kind="class">hssi_10g_cmd</class>
    <member kind="define">
      <type>#define</type>
      <name>CSR_NUM_PACKETS</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>af40e5b66e5d16497e47e8b10fc7e9a44</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RANDOM_LENGTH</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a24b333e7caef952009c18ad2e26badce</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RANDOM_PAYLOAD</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>acf50e8198c1decdb5f3264099db2fea8</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_START</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a2154daea96bd7d69f410acb711d86b03</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STOP</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>afae4a4b81a4217f713ed1aab257fbc31</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SRC_ADDR0</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a59ea99e840facf8745bcaa0b2e00886e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_SRC_ADDR1</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>af7bf5ece5127db0085135197d0d40436</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_DEST_ADDR0</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a2c69118ddfb7e4103b35f7730c1a57b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_DEST_ADDR1</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>aa9a5a168e6cb93107ba02389bd91a117</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_PACKET_TX_COUNT</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a313cf0cae4a33761d5d25dc87b871445</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RND_SEED0</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a1ce90b64e0156aaf617121855559acd7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RND_SEED1</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a204070435ba257c6b00152dee5e73ea4</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RND_SEED2</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a002555ad38e153b807633299fa9d672c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_PACKET_LENGTH</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a9015b2280acc5e2d1e518f764b9e754b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_STA_TSTAMP</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a7188a68123d2f47bc0b80a47c80e9496</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_TX_END_TSTAMP</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a6486622d431be951809ba69f98b916ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_NUM_PKT</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a73e849a9472ea96b16645083c5f71544</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_PKT_GOOD</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a4ea062053ff71409ce9535f3e35546a6</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_PKT_BAD</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a4dbb799ec3d5b286fda4e3251029925f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_AVST_RX_ERR</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>ad95d174ae6c1a6d25aba40f36403bbab</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_STA_TSTAMP</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a0e6c344157d3583b61ca93c43e09ed1b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_RX_END_TSTAMP</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a3f6ae7720cd10eaa8ddbb8adf7d566d9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_MAC_LOOP</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a9d63f6ba744592cdac940e37951018c9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INVALID_PORT</name>
      <anchorfile>hssi__10g__cmd_8h.html</anchorfile>
      <anchor>a6f07d0094e837d61131ab0b3aa8ab1fc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi_200g_400g_cmd.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__200g__400g__cmd_8h.html</filename>
    <includes id="hssi__cmd_8h" name="hssi_cmd.h" local="yes" imported="no">hssi_cmd.h</includes>
    <class kind="class">hssi_200g_400g_cmd</class>
    <member kind="define">
      <type>#define</type>
      <name>CSR_AFU_400G_TG_EN</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>aae3503e5812cff78ef76800d4dc501b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_HW_PC_CTRL</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a39d09ccdde94355900805b753393f14b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_HW_TEST_LOOP_CNT</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a8684ea466882f45fb3f198e58305cf17</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_HW_TEST_ROM_ADDR</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>ae6a313879c94ecd2a1e5d5ea2b9144c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TX_SOP_CNT_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a81fd63728c8a6eeb0cdecece59cf0bc8</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TX_SOP_CNT_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a1740712481e2552b7cf7f5a0e0edce87</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TX_EOP_CNT_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a9b52ee1cbbef09ba0414cda110566e33</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TX_EOP_CNT_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a5f674ba88a4095cdb4c28523a0c45f56</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TX_ERR_CNT_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a1e585ac34b5691afe99f56b530d4978a</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TX_ERR_CNT_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a9208c7efff6ffdddca7a8cfb49c03f21</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_RX_SOP_CNT_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a5dbea227de14aa04c41cb30dd2edaebe</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_RX_SOP_CNT_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>aedaf3c3ae287fcd10069ccf9b4928def</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_RX_EOP_CNT_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>af91a2ed2bf4bb3b2bc4d67898cdd5128</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_RX_EOP_CNT_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a2f6a7e5034087166144e513217aa5f44</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_RX_ERR_CNT_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>ab11c1c1a451fb5235db05c4b80de20e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_RX_ERR_CNT_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a5b59f6c537c3eae4e7db3eb1aa23c6af</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TIMESTAMP_TG_START_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a31dee803cf037e532f88de00e78780e1</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TIMESTAMP_TG_START_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a5fdbe6f0e6c513ae3c2dedf631feace7</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TIMESTAMP_TG_END_LSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>a66684b983a8fe08e21001b7df5ac2c78</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CSR_STAT_TIMESTAMP_TG_END_MSB</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>af300070242fe08668c4ac391ccd23e41</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USER_CLKFREQ_N6001</name>
      <anchorfile>hssi__200g__400g__cmd_8h.html</anchorfile>
      <anchor>abad1d58d09ef43cc051669841058ebdb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi_afu.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__afu_8h.html</filename>
    <class kind="class">hssi_afu</class>
    <member kind="define">
      <type>#define</type>
      <name>ETH_AFU_DFH</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>ac0baaf0917fee2104b9c6b44075298a2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ETH_AFU_ID_L</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>abe08ed7e56c85bcf7089b573333dd320</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ETH_AFU_ID_H</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a8b95eb7d4b6d30992c5274633ab33fef</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>TRAFFIC_CTRL_CMD</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a461eab73cc68f8fa3a50edaae21cfee3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>TRAFFIC_CTRL_DATA</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a3ce04cd0dea4df8da87b9f643cb4d069</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>TRAFFIC_CTRL_PORT_SEL</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a0a379a8f9138cdda028b7d15a52d9e39</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_SCRATCHPAD</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>ab2b3c37ae1a49971b27a9fbdbcc4d1d3</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>READ_CMD</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a21623e2a5501c821da54dd76ffc1d077</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WRITE_CMD</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>af792feb13ae0c1eab8f95f64c8baa96d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ACK_TRANS</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a859eb8f8ff2398eff21aab5346844144</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>AFU_CMD_SHIFT</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a16b1ade4a60a95678d58b2fa6b9fcc10</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>WRITE_DATA_SHIFT</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>abe6eab6a74c11bf2db22b5f73d41fd47</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>NO_TIMEOUT</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>a462fb2ba6f2af99ec3d021ded436bb65</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>hssi__afu_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi_cmd.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__cmd_8h.html</filename>
    <includes id="uio_8h" name="uio.h" local="no" imported="no">opae/uio.h</includes>
    <class kind="class">hssi_cmd</class>
    <member kind="define">
      <type>#define</type>
      <name>PKT_FILT_CSR_DEST_ADDR</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a3c5cddd0ed6864ab681d05e26e9c3554</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INVALID_MAC</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a02028d30e3cea7b4e76419c00a1c6e6d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INVALID_CLOCK_FREQ</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>afac984768dc7184c0b2ed50456a06076</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USER_CLKFREQ_S10</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a7be58e7806ec4b0e9c546470229f4dd2</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USER_CLKFREQ_N6000</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>ae2539670faebaec305a8380ff9fcf798</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BITSVER_MAJOR_S10</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a8c74ad5179038d17b345dfe79279c2c4</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>BITSVER_MAJOR_N6000</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a8c3e18e7cbf560ff92e8047255f8ec3b</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>FPGA_BBS_VER_MAJOR</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a6bd4704606bf8c2114179696c85370d3</anchor>
      <arglist>(i)</arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>hssi__cmd_8h.html</anchorfile>
      <anchor>a14cb42c9e63dc7d0cf1a9475c1385a34</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>hssi_pkt_filt_100g_cmd.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__pkt__filt__100g__cmd_8h.html</filename>
    <includes id="hssi__100g__cmd_8h" name="hssi_100g_cmd.h" local="yes" imported="no">hssi_100g_cmd.h</includes>
    <class kind="class">hssi_pkt_filt_100g_cmd</class>
  </compound>
  <compound kind="file">
    <name>hssi_pkt_filt_10g_cmd.h</name>
    <path>/root/samples/hssi/</path>
    <filename>hssi__pkt__filt__10g__cmd_8h.html</filename>
    <includes id="hssi__10g__cmd_8h" name="hssi_10g_cmd.h" local="yes" imported="no">hssi_10g_cmd.h</includes>
    <class kind="class">hssi_pkt_filt_10g_cmd</class>
  </compound>
  <compound kind="file">
    <name>mem_tg.cpp</name>
    <path>/root/samples/mem_tg/</path>
    <filename>mem__tg_8cpp.html</filename>
    <includes id="tg__test_8h" name="tg_test.h" local="yes" imported="no">tg_test.h</includes>
    <includes id="mem__tg_8h" name="mem_tg.h" local="yes" imported="no">mem_tg.h</includes>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>mem__tg_8cpp.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>mem_tg.h</name>
    <path>/root/samples/mem_tg/</path>
    <filename>mem__tg_8h.html</filename>
    <includes id="token_8h" name="token.h" local="no" imported="no">opae/cxx/core/token.h</includes>
    <class kind="class">mem_tg::mem_tg</class>
    <namespace>mem_tg</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac9891bc8aab5db05ddbcfba187ba231a</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a0a6fc2d03143e8204e8243a38addcee7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ACTIVE</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a6e2000299654529dae8c4c4af161e49c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a39b57be20840498a98f4cd174e292a4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ERROR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a3964c55dcdc68e90d56bf757b65b1d8a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_PASS</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77afbb80104f01fcbf03960c1b685d33aa3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_RESPONSE_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a12b728ef588392d79acd492c13e5df0d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaa8b6510cf448bb867772dc2ec2aa9f8f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_SEQ</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaa0282e80a72d9f469c95c722f5ac1c423</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND_SEQ</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaa569b807ecd9597d27fbd18025e926146</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_ONE_HOT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaaaa9f845f52633dddf601fb2a012a5d06</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_FIXED</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36afa04848192643b5020f9cbed0307c292</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS7</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36a22c5a9122b9ee1237a486ee9793c7380</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS15</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36a5fcae71881d710056306ecbd59d1f353</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS31</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36adb1e0653890ac8340af449158f0ae62e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_ROTATING</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36a582126acece0943286941192f1c5e00e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a0fcea165efcbc477fd0a5f9f05bad92d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a70d3478f33ac42261cce33ae92a58440</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54aa752f89b2cc2fb5ecfb034a3aadc2e3c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NEXT_AFU</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54ac123d30b0268ad1c521e9e98feed74b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH_RSVD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a376cf9f030a0b65a10226edf162499eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SCRATCHPAD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a4c62d78ea8b14f6f8fb749088d81c6b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CTRL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a71c07e22c468e2fac16fc82de00ae213</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_STAT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54ab2e545ddbd0f4ff871aa8c2b227c56ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CLOCKS</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a92a59d89dfb05439439512e623f02212</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_VERSION</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa04aae5e4774f4ba73394e884387749c5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_START</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa36e5f2f8a7ce70509baad0fc29c7217d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_LOOP_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faab7410bb101bc061d4ae261d012f9fcb4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa712daf68ed5d46d6cba0a0033eb45a59</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaf91e5aaa66fc608c1afc2cf0a2a20f49</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_REPEAT_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa4cd1c35dc2ff3d288cd1cfc60777941e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_REPEAT_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaf7d1449e46dfd3fa0fd874fb9f44c2b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURST_LENGTH</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faad9acb4dff9aaa1df270472237301ed7a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_CLEAR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8fb799f1eb48b8410df89647e8122194</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_IDLE_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa0e952a276e6151f1d59e03f7f43f5c37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_LOOP_IDLE_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa22b7465ca8a636c2a167ec4edc46ec4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_WR_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa78901634d9ccae75648d724f9af99a51</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_WR_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa684f05ecb4378c337d13ec3f8fee2fc8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_WR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaa1ae8be038850228674d43f5bf0a756f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RAND_SEQ_ADDRS_WR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa6907903904e0653d73a30bdf29a27ed2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RETURN_TO_START_ADDR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa0f7198530578f4c0eec41f4c3f05a3be</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_ADDR_INCR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8963a3103bffd3412a2340d14d93b21d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_RD_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaf68f3e5878b9fa4c7223312f75cf4a54</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_RD_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8719a1d0099392f7568b960a34f23ca9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_RD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa0d0b4dc47ddd3f5315c986c4297c65cf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RAND_SEQ_ADDRS_RD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa9eed28651432b26b6e893537dad1c8f0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PASS</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaa3ebbf541ffa826713743898a9b87fca</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa77004e9dec4a1b60919402a1099c9b9b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa21b985a60da2e963c039573cf8ce4292</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3f9757573c68c6984a904badea0ddac3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3aa084a514969080b97217bd35465356</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faabeb38326d70d8fa51fa47bf2771e5443</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8a1320c318666ad7219be02ffb547ff2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faafddea5c425e65e7c09f31f3717ff89a7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_COMPLETE</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa83044d71ca0917991b76ef043f7683b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_INVERT_BYTEEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa83be95f292fd349ce0d8de1679bcfc40</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RESTART_DEFAULT_TRAFFIC</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3e5d4c298071e1d27a597c6f0e2e2873</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_USER_WORM_EN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa04a628e98d370b2e44979a1cb0d934b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_BYTEEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa6fc1ee1598e27892c1b190338a727d4a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa26be34da618bd9ff6af7e5085468c683</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_DATA_GEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3abebe209bc7ba6e3993bc9950881722</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_BYTEEN_GEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa066ea4e9f56bdf1df417becadb27a4af</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RDATA_WIDTH</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaee7adf738b7b7831a13aa4a40687bd68</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ERROR_REPORT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faabbb606f7914787cb5c521f428fb72562</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_RATE_WIDTH_RATIO</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa4afe73ab23f12ae3ab710f533707438f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PNF</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faabba43d1a9d25c20bf14088aa3c92b72d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_EXPECTED_DATA</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa99701a288d09c6214525e1cc65a64ba0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_READ_DATA</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa5b509ef9f39fcdbdaa760abce6c05cf1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_SEED</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3ff28b39ca90987ee5328faf55f7cc73</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEED</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa91c7cb5a3402c47f6435ddaa99a35d1b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PPPG_SEL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa4206df51ef621a3c26c4b32d28bdd748</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faab2e2b4a6cb66d31dc888869bb2a42afe</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>AFU_ID</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af02ccce98e14a5901318de5305ff2792</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MEM_TG_TEST_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ab99947d9ae3914d454524af4289c4f75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TEST_SLEEP_INVL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa14a41e302dcb5763a1dfa2af93083cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>tg_pattern</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa321b366aefc06c5924ed83f3fac4109</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const int</type>
      <name>MEM_TG_CFG_OFFSET</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a943a6cf589a5fd0b37a523b5df6513e6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>tg_test.h</name>
    <path>/root/samples/mem_tg/</path>
    <filename>tg__test_8h.html</filename>
    <includes id="mem__tg_8h" name="mem_tg.h" local="yes" imported="no">mem_tg.h</includes>
    <class kind="class">mem_tg::tg_test</class>
    <namespace>mem_tg</namespace>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>tg__test_8h.html</anchorfile>
      <anchor>abc250359b0b249806cbba4173e9644b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::mutex</type>
      <name>tg_print_mutex</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>afb390604495f7ec7dd3aee9cc7b67d8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::mutex</type>
      <name>tg_start_write_mutex</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ad0bd8092778fe7d39d3e37402e72386c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::condition_variable</type>
      <name>tg_cv</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a9829f06cc0a289cb4b12f3ceeb5b94c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::atomic&lt; int &gt;</type>
      <name>tg_waiting_threads_counter</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a1b62d23fb279e2d99d781ac03a43276e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>tg_num_threads</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5f9ac460f06d8d421c98634269f87cd4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>n5010-ctl.c</name>
    <path>/root/samples/n5010-ctl/</path>
    <filename>n5010-ctl_8c.html</filename>
    <includes id="error_8h" name="error.h" local="no" imported="no">error.h</includes>
    <includes id="fpga_8h" name="fpga.h" local="no" imported="no">opae/fpga.h</includes>
    <class kind="struct">n5010_test</class>
    <class kind="struct">n5010</class>
    <member kind="define">
      <type>#define</type>
      <name>DFH_EOL</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a64d2514266ba20fce840e5648ef89199</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFH_NEXT</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>aa14c35cd3b1b02bd5b753c3d19c05308</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFH_TYPE</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>aca731dc493c4ea43958cb0435cdaa571</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFH_TYPE_AFU</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a57df7e6b18c59c759284e4129b591829</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_PORT</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a401e1a60d6381236216b6a130a6685bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>INVALID_PORT</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a6f07d0094e837d61131ab0b3aa8ab1fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CVL0_QSFP01_SWITCH</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>ae5b8e415198ec56ea5cf5fd1b07380df</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>CVL1_QSFP23_SWITCH</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a5c7a15c4dfb0142c7a87e6f70d7cd104</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>signal_callback_handler</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a89a8322bea357674e81ba9cbdefe0378</anchor>
      <arglist>(int signum)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_setread_switch</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>af49d8292da513d31bb1a0a16861e230a</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_read_switch</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a9ceebefbea1cedb27769eb8d6a78b6fb</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_run</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a71f08c9d467b2c0d6627c2a19182406e</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_open</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a8b18501a62f680f0b60aeaf03b0feaaf</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>fpga_close</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>ab3d638b8387fa4ec22627ec8a6b53e62</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_base</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a2e79fe0581ec5864faa636b5fe0e03a4</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>fpga_dump</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>af1edf9b723cd625e1655f1f12eb21efa</anchor>
      <arglist>(struct n5010 *n5010, uint64_t offset, size_t count)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_set_switch</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a6596f7a4b6b9ac8060d18e3f40f42334</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_read_switch_port</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a46c13d0ab2132699eac8a7830a536d7c</anchor>
      <arglist>(struct n5010 *n5010, uint32_t cvl)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_run</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>aa566e69cffeabc52ffa796bd9193ab03</anchor>
      <arglist>(__attribute__((unused)) struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>print_usage</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a9bd4308d463193b0188339cb5495a58f</anchor>
      <arglist>(FILE *f)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>parse_port</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>abf670aede0d10582400b91cb54634821</anchor>
      <arglist>(struct n5010 *n5010, const char *port_str)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>parse_mode</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a443cc7c174314015bd7fe2613c4d8245</anchor>
      <arglist>(struct n5010 *n5010, const char *mode)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static int</type>
      <name>parse_args</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>adde1c5cf0a410d13f51b29238589fd42</anchor>
      <arglist>(int argc, char **argv, struct n5010 *n5010)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a3c04138a5bfe5d72780bb7e82a18e627</anchor>
      <arglist>(int argc, char **argv)</arglist>
    </member>
    <member kind="variable">
      <type>char const  *</type>
      <name>default_guid</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a514ebab49a2fb401d7d7e220a97300e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static volatile bool</type>
      <name>stop</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>acb99505dbfaf7824f8725dff90d47ca8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const struct n5010_test</type>
      <name>n5010_test</name>
      <anchorfile>n5010-ctl_8c.html</anchorfile>
      <anchor>a0583be682de6cecd33c5d44955e5ee66</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>n5010-test.c</name>
    <path>/root/samples/n5010-test/</path>
    <filename>n5010-test_8c.html</filename>
    <includes id="error_8h" name="error.h" local="no" imported="no">error.h</includes>
    <includes id="fpga_8h" name="fpga.h" local="no" imported="no">opae/fpga.h</includes>
    <class kind="struct">n5010_test</class>
    <class kind="struct">n5010</class>
    <member kind="define">
      <type>#define</type>
      <name>DFH_EOL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a64d2514266ba20fce840e5648ef89199</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFH_NEXT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aa14c35cd3b1b02bd5b753c3d19c05308</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFH_TYPE</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aca731dc493c4ea43958cb0435cdaa571</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DFH_TYPE_AFU</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a57df7e6b18c59c759284e4129b591829</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DDR_TEST_MODE0_CTRL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a11fa2de5d944e400aca76489981f2bf5</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DDR_TEST_MODE0_STAT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aafd7639b4452467ade798bbbb6495328</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DDR_TEST_MODE1_CTRL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aa797184b69f2027cb914153bef518e6f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DDR_TEST_MODE1_STAT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a6ad2ae5dc714ef052ac3c0615ff9d52d</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>DDR_TEST_MODE1_BANK_STAT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>af2a7dd16fe742a0cab2a0b44054299f6</anchor>
      <arglist>(x)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HBM_TEST_PASS</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>af6d4323e2c3b5bb514a847b9f3fd535e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HBM_TEST_FAIL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a0cecb64eaade5d14e674207e514755b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HBM_TEST_TIMEOUT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ad1c397afa5c85b9a28f073353ca9032c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>HBM_TEST_CTRL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a2bf1b6f3be700790eed5fda6e84fa81e</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>QDR_TEST_STAT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aee1428f73247ddf1841483bbd9f3928c</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>QDR_FM</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>acd20fcd6bd92565b6e0c8667b24815ea</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>QDR_TEST_TIMEOUT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a99cad47c77096e0eca94240aac7fc258</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>QDR_TEST_FAIL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a59145ff07c3dfaf8ab18785b5e3efc9a</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>QDR_TEST_PASS</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a52603e510127491e9adcc19710c72ff8</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>QDR_TEST_CTRL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a8dbe1ba9e62a18d9a588f473458978ce</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ESRAM_TEST_STAT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a000ba0c3445077186d306cac29ab41e9</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ESRAM_FM</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a2b060f9438390e34799ed496075b28f0</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ESRAM_TEST_TIMEOUT</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aaacce65608c404b4aedc54f5afc8cd6a</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ESRAM_TEST_FAIL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aec1f8e4c78da3130142597eb059a5c5d</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ESRAM_TEST_PASS</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ad2ef5ce635ab16f9c52bfb43c72ba959</anchor>
      <arglist>(h)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ESRAM_TEST_CTRL</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aba15ac25260eb5c478e49ea9bbc93c8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_POLLS</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a31e69e55e98d6aa055fb6a352bb0feef</anchor>
      <arglist></arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_test_ddr_directed</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a66f9a1b5d37745c9587641613bf50cd5</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_test_ddr_prbs</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a4a327a70c05938aa6ecda3e2e9225663</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_test_hbm</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ae92a916f3955b96edf4ef7ab5a667114</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_test_esram</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ae1108fb5be774c3a02c79b22d9e4e8a4</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_test_qdr</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a7d4ba0af37d28b2281e697ff461b8cbb</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_open</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a8b18501a62f680f0b60aeaf03b0feaaf</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>fpga_close</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ab3d638b8387fa4ec22627ec8a6b53e62</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_base</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a2e79fe0581ec5864faa636b5fe0e03a4</anchor>
      <arglist>(struct n5010 *n5010)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>fpga_dump</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>af1edf9b723cd625e1655f1f12eb21efa</anchor>
      <arglist>(struct n5010 *n5010, uint64_t offset, size_t count)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_banks</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a2d826077756d3ecead9f30e5d6eada51</anchor>
      <arglist>(struct n5010 *n5010, uint64_t offset, uint64_t *num_banks)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_start</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aef1214e09ce39997c1133269fd0936e9</anchor>
      <arglist>(struct n5010 *n5010, uint64_t offset, uint64_t num_banks)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_stop</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>aad710973341f9f1301a817df6b1b8090</anchor>
      <arglist>(struct n5010 *n5010, uint64_t offset)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>fpga_wait</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a2973f7fa537bc9d89cbda5fdf88682e6</anchor>
      <arglist>(struct n5010 *n5010, uint64_t offset, uint64_t init, uint64_t result)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>print_usage</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a9bd4308d463193b0188339cb5495a58f</anchor>
      <arglist>(FILE *f)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static bool</type>
      <name>parse_mode</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a443cc7c174314015bd7fe2613c4d8245</anchor>
      <arglist>(struct n5010 *n5010, const char *mode)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static fpga_result</type>
      <name>parse_args</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ab750b2af83dc6cbb0aa2d9a9d1b25079</anchor>
      <arglist>(int argc, char **argv, struct n5010 *n5010)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a3c04138a5bfe5d72780bb7e82a18e627</anchor>
      <arglist>(int argc, char **argv)</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const struct n5010_test</type>
      <name>n5010_test</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a0583be682de6cecd33c5d44955e5ee66</anchor>
      <arglist>[]</arglist>
    </member>
  </compound>
  <compound kind="file">
    <name>object_api.c</name>
    <path>/root/samples/object_api/</path>
    <filename>object__api_8c.html</filename>
    <includes id="fpga_8h" name="fpga.h" local="no" imported="no">opae/fpga.h</includes>
    <class kind="struct">named_object</class>
    <class kind="struct">metric_group</class>
    <class kind="struct">token_group</class>
    <class kind="struct">config</class>
    <member kind="define">
      <type>#define</type>
      <name>ON_ERR_GOTO</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a7635d12a96b25dd1760b4bc44130eed9</anchor>
      <arglist>(res, label, desc)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_CLEANUP</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a14d6fc1e8245723fc29c53beaaa5d7fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>ADD_TO_CLEANUP</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a38f61da696fc8113fbcb512f918c1c81</anchor>
      <arglist>(func, p)</arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_TOKENS</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a09886d6ba96e67553bf3c49ed8ade975</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>MAX_GROUP_OBJECTS</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>ac6a7bbe65decea8da048b4d5b2b58f46</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>GETOPT_STRING</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>acc2c9cc08f0f7b8f21f835d3fa0f6435</anchor>
      <arglist></arglist>
    </member>
    <member kind="define">
      <type>#define</type>
      <name>USEC_TO_SEC</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>aa29fc676ed569b67c95f0ba51150b387</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>destroy_f</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a7ce0fba66c1863e475010b6ef14d66d1</anchor>
      <arglist>)(void *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_err</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>aa11f434898b86d619a5ceb6a5d7bf066</anchor>
      <arglist>(const char *s, fpga_result res)</arglist>
    </member>
    <member kind="function">
      <type>metric_group *</type>
      <name>init_metric_group</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a37d8cef8f07927f8c55c4b5c92bdc477</anchor>
      <arglist>(fpga_token token, const char *name, metric_group *group)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>add_clock</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>acf84beec79fddbce4024fb6b3ff977e5</anchor>
      <arglist>(token_group *t_group)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>add_counter</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>ad17e22d237013df1ad0ad90a5604eee2</anchor>
      <arglist>(metric_group *group, const char *name)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_counters</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>accfee7be7eb9741f2a6ca3281a46dce8</anchor>
      <arglist>(fpga_object clock, metric_group *group)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>help</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a0bed8474bd33a912769360766f6b10d4</anchor>
      <arglist>(void)</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>parse_args</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a3a1b6f72679286eae6f9eec936cc5533</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a0ddf1224851353fc92bfbff6f499fa97</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="variable">
      <type>struct @140</type>
      <name>cleanup</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a742025e52f6ec88c8341d1a90e5ec679</anchor>
      <arglist>[100]</arglist>
    </member>
    <member kind="variable">
      <type>STATIC int</type>
      <name>cleanup_size</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a9f95f0bf91a23d61298714eb5147b475</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct config</type>
      <name>options</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>ab61f552ece35d302b110b6c2e154226f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::afu_test::afu</name>
    <filename>classopae_1_1afu__test_1_1afu.html</filename>
    <member kind="enumeration">
      <type></type>
      <name>exit_codes</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>success</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24acbd7e559c7a281178598e73f9df0934f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>not_run</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a04ee7cb880c09f91baf6c572a753154d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>not_found</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a5b5cf74a6eb73cf60867b2f81a810624</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>no_access</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a0a753cba76e13abc6b16f98745712372</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>exception</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a8b98ec99513ca15fc4c00d45ccd64fa5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>error</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24ab553cc480865cc051b212c1cd0150238</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>int(*</type>
      <name>command_fn</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a4461ef32b1ec7b4bd81614a6491cb23e</anchor>
      <arglist>)(afu *afu, CLI::App *app)</arglist>
    </member>
    <member kind="enumvalue">
      <name>success</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24acbd7e559c7a281178598e73f9df0934f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>not_run</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a04ee7cb880c09f91baf6c572a753154d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>not_found</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a5b5cf74a6eb73cf60867b2f81a810624</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>no_access</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a0a753cba76e13abc6b16f98745712372</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>exception</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24a8b98ec99513ca15fc4c00d45ccd64fa5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>error</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad7127e1c68a3a17b2eb1dce5e840ad24ab553cc480865cc051b212c1cd0150238</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>afu</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a845e58d643b7c868c6dc84277dc88a81</anchor>
      <arglist>(const char *name, const char *afu_id=nullptr, const char *log_level=nullptr)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~afu</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a9f5b3668c254e92f8f2506916f97a232</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>CLI::App &amp;</type>
      <name>cli</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad3e39bbb116a49dce6c1043755ebf2df</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>find_dev_feature</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>adacf6b9b42c1b28c391d8cc4d93023ca</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>unmap_mmio</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>aba7d799aad72ececb2b9eb68c8919353</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>map_mmio</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad08ddd74d13e31d4aacf22c87b81fc6e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>open_handle</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a7af4fff79b88676402bff5f5274b941d</anchor>
      <arglist>(const char *dev)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>main</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>acacf9a109b0c623a7cee83acae4582cf</anchor>
      <arglist>(int argc, char *argv[])</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a2b3d816eac276d4ec1aaa6e9a632b32d</anchor>
      <arglist>(CLI::App *app, command::ptr_t test)</arglist>
    </member>
    <member kind="function">
      <type>CLI::App *</type>
      <name>register_command</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a66b79e6a2f6e58f8379823f0d675a139</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>read64</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a148377c272b94f4e884683bcbcdaf289</anchor>
      <arglist>(uint32_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write64</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a81608240a2e29efbe8a6e60aee74ed60</anchor>
      <arglist>(uint32_t offset, uint64_t value)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>read32</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>aec158ebde0d3442da9a1bf4979e73a0d</anchor>
      <arglist>(uint32_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write32</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a85040c8bd1b3b3aeb0878cff74c4ca92</anchor>
      <arglist>(uint32_t offset, uint32_t value)</arglist>
    </member>
    <member kind="function">
      <type>command::ptr_t</type>
      <name>current_command</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a2db800c668b606885c4f4c405f8da4bc</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>allocate_dsm</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a39e0e1342cd3c8d2e630be07ca787453</anchor>
      <arglist>(size_t len=((4) *1024), uint32_t numa_node=0)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>free_dsm</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a94ab4cd7eb2f5abf7f9c076c9cd3b2e4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>reset_dsm</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a127d8571f7052515afc1d400c0981604</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>allocate_cache_read</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>aeae0c9059bf0455e5d238a168f776a16</anchor>
      <arglist>(size_t len=((2) *1024 *1024), uint32_t numa_node=0)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>free_cache_read</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>adfd7bee969d9fb698179177cb0109fa8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>allocate_cache_write</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ac2756579ad42be859b696a1880a703a2</anchor>
      <arglist>(size_t len=((2) *1024 *1024), uint32_t numa_node=0)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>free_cache_write</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a4f75d9580bec86a37adc9829e038bcf4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>allocate_cache_read_write</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ae9f15c76d68e94b445c2c1c21f727d1a</anchor>
      <arglist>(size_t len=((2) *1024 *1024), uint32_t numa_node=0)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>free_cache_read_write</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>adadb65558a3a54ba88e96b872d43b30d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>uint8_t *</type>
      <name>get_dsm</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>aadd011881d4bf921671c9a426262e61e</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint8_t *</type>
      <name>get_read</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a1ec221419c8668d8b14e1b7e6c056b98</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint8_t *</type>
      <name>get_write</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>abff0d8566f3e4bfe138f44eadab9b16f</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint8_t *</type>
      <name>get_read_write</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ac03cc5d56c562425a9bcbbe1c2edf7c0</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="variable">
      <type>std::shared_ptr&lt; spdlog::logger &gt;</type>
      <name>logger_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>af3999e552b0f5d709ded4693bbeb4d87</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>name_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>aee6c870bd4c573cd06893f815232a1dc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>afu_id_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>af29e9be9b338fcda6a5d02a0431bf522</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>CLI::App</type>
      <name>app_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>abe62e0407c33466b7d6bf5005276fdc3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>pci_addr_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ae9b4f7f4daba866467f04e9c64d660ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>log_level_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a410d46e06fe9a836ec107a08cf8576a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>timeout_msec_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a75f121f0ea96c60091ca1223f5b29004</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>int</type>
      <name>fd_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a99dd66842fa780a0ceb7dda617c4e38b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t *</type>
      <name>mmio_base_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a9b40ebeb9a922bed3bdb079e6285b4e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>mmio_len_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>acb52665e466bb54609b5ac81d4dfd282</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t *</type>
      <name>dsm_buffer_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a4784e4f7a10529f0f107c7f241b89767</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>dsm_buf_len_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a868b16ab618cfdbd8f8bd403b2ba2af7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t *</type>
      <name>rd_buffer_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a5fd0b4fdfb3009117ff4a9f79cf84743</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>rd_buf_len_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ad55f0e9505cb20d242e7c53005d2f462</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t *</type>
      <name>wr_buffer_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>ab6736eb6d461e288c0f7846c741bb511</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>wr_buf_len_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a39bda33296e1b0fb5a486c349d098c77</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t *</type>
      <name>rd_wr_buffer_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>afd43e84a2d933b6141298f89eae7d22e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>rd_wr_buf_len_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a6a13abfbef7c0e17b35bf8dc11c06f00</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>struct dfl_cxl_cache_region_info</type>
      <name>rinfo_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a607ed076daacf92b1482fba24edacd6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>dev_path_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a36d7214cdf09708a4eba033e21447434</anchor>
      <arglist>[2]</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>command::ptr_t</type>
      <name>current_command_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a62081683d3d299f6a71540923eca6291</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::map&lt; CLI::App *, command::ptr_t &gt;</type>
      <name>commands_</name>
      <anchorfile>classopae_1_1afu__test_1_1afu.html</anchorfile>
      <anchor>a3a7e209b3faad4cfd9472cc19d6685f7</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::afu_dfh</name>
    <filename>uniondummy__afu_1_1afu__dfh.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1afu__dfh.html</anchorfile>
      <anchor>ae9b4634d1329e564be6a1cc7156e1cbaa544188b280cf29aaf4a2ead476fbc0f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1afu__dfh.html</anchorfile>
      <anchor>ae9b4634d1329e564be6a1cc7156e1cbaa544188b280cf29aaf4a2ead476fbc0f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>afu_dfh</name>
      <anchorfile>uniondummy__afu_1_1afu__dfh.html</anchorfile>
      <anchor>a301e87cde4d4e5e325aa220ab3ccce3c</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1afu__dfh.html</anchorfile>
      <anchor>ae4801bc07cdbd1212c032fa8a01cce7e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::afu_dfh.__unnamed65__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1afu__dfh_8____unnamed65____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>FeatureID</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a10415075d26efaba20559f95374c3ba5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>FeatureRev</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a20ca4b072df0e27f10cd26da1364b28b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>NextDfhByteOffset</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>abe762b71ccf0b32e81dd414b0dd31a9d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>EOL</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>aecd284e164bdc984d109aea9e86e695c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved41</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>aa66247353afee3549cf74e5ca33030be</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>FeatureType</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ad3abdac08cd922b2c2e5fe50b23ae5b2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::busy</name>
    <filename>classopae_1_1fpga_1_1types_1_1busy.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>busy</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1busy.html</anchorfile>
      <anchor>a64ade3853932d99c06e5d7cc4f667772</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>cache_line</name>
    <filename>hello__fpga_8c.html</filename>
    <anchor>structcache__line</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>uint</name>
      <anchorfile>hello__fpga_8c.html</anchorfile>
      <anchor>a3f28028a2861bc1cb7e3d07fa84bb4a2</anchor>
      <arglist>[16]</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::afu_test::command</name>
    <filename>classopae_1_1afu__test_1_1command.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; command &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a11a258a1fa5a99a5be9bb34a87c48f41</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>command</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a07de34c2af7ef6ae3c0e528888365b0c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~command</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a04e764764cc62327e60d94840590ee73</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a2892a3d443d007a55cbfe9ec93835052</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a064a25924cdd32c14f4a68694f924d5e</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a18fe26fd1da4bec29da4fbfbeb172bbd</anchor>
      <arglist>(afu *afu, CLI::App *app)=0</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>aa91365cda1ee7a6049d8a5403b2e15e6</anchor>
      <arglist>(CLI::App *app)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a51d5c7afb132598121e20be5b0eab5fb</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual uint64_t</type>
      <name>featureid</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>ad9698a8b5ee31447582c286a115724a7</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual uint64_t</type>
      <name>guidl</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>ac239529df959302ece72a162262c6c32</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function" virtualness="pure">
      <type>virtual uint64_t</type>
      <name>guidh</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a7b9e6753ab014e81bb4dd7d1ddbaad85</anchor>
      <arglist>() const =0</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>running</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a36d9eff18f795e0bbf60d3e213b1a81f</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>stop</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>ab5200927c95fe844db13588f9288bf87</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>std::atomic&lt; bool &gt;</type>
      <name>running_</name>
      <anchorfile>classopae_1_1afu__test_1_1command.html</anchorfile>
      <anchor>a38b260d3448bfd7f2368b2ef585414be</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>config</name>
    <filename>object__api_8c.html</filename>
    <anchor>structconfig</anchor>
    <member kind="variable">
      <type>int</type>
      <name>open_flags</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a56a710b532c4f7d6213fbd173920ffca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>run_n3000</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a860ca20f3c0dd13da4979632fb61f505</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>float</type>
      <name>interval_sec</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a026c31834bf67496b4a0af04d40e2adb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>ctrl_config</name>
    <filename>hssi__100g__cmd_8h.html</filename>
    <anchor>structctrl__config</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>pkt_size_data</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>afb14a5a450b96fa0d96b720330b1718a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>ctrl0_data</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a24d19d0023013270e4f30fd6d8b972ac</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>ctrl1_data</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a94d430962eddc3a57f4ad266739811da</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>cxl_mem_tg::cxl_mem_tg</name>
    <filename>classcxl__mem__tg_1_1cxl__mem__tg.html</filename>
    <base>opae::afu_test::afu</base>
    <member kind="function">
      <type></type>
      <name>cxl_mem_tg</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>a296cb2e6e2cb7e04c3795c4d1cb87217</anchor>
      <arglist>(const char *afu_id=AFU_ID)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>ab2e71f7f0b48b5e59b4240c180cb9447</anchor>
      <arglist>(CLI::App *app, test_command::ptr_t test) override</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>get_offset</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>ae1aef01e746ba0f0327ca5be1544b28f</anchor>
      <arglist>(uint32_t base, uint32_t i) const</arglist>
    </member>
    <member kind="function">
      <type>token::ptr_t</type>
      <name>get_token</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>a7b6b2438e7e91612a2fe3be286efad82</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>count_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>acae142eced8d47e700e622befe91ce04</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>mem_ch_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>a4908bbf8b834edcbe31492711d74f7d7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>loop_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>a8ea17c5df9494ad27e1e95f31d489858</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>wcnt_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>aba0cb59c437869963df1d5e222f73805</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>rcnt_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>ae57b9fd56466bebb5321490005803215</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>bcnt_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>ae9800086ad36084c5641526bc5940a0f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>stride_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>aba649e8e87f688e8e66332f3d94f6698</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>pattern_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>ad0796f52001b9197d16889609d44c034</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>mem_speed_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>afd50553bb554f47c1fc061195891f448</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>status_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>ad7709a24252d85accf090db72df0b970</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_offset_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>a7d7ceebe3ce1b800de536ac8c12e4d76</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::map&lt; uint32_t, uint32_t &gt;</type>
      <name>limits_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__mem__tg.html</anchorfile>
      <anchor>a1196abdb029343d2812802aa01e4ada9</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>cxl_mem_tg::cxl_tg_test</name>
    <filename>classcxl__mem__tg_1_1cxl__tg__test.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>cxl_tg_test</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a04b6bcabd36c04303c3463a866c577d9</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~cxl_tg_test</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a43a443aff95343bd0222478b01febec8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a79c9aab7798e80455d97fd8e79942228</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a8c9aba82bd9e827467e43dfd4cc50a7f</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>abcd07262c0248d11ef15bc3c1a90d973</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function">
      <type>double</type>
      <name>bw_calc</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a1499fc2b3cf91c5623f2f5a4a51cac73</anchor>
      <arglist>(uint64_t xfer_bytes, uint64_t num_ticks)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_he_mem_tg</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>afb228c70c14babce503c928fd8f4d9ec</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>tg_clear</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>abfd873f8a748a61c5e99a9eec47d3822</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tg_perf</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>aeaf4cb8c98a142f8e575506e2ad4758b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tg_print_fail_info</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a9e0eff2ffe1dcc8eb9fb4df06098042c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>tg_wait_test_completion</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>ab5a014aa963baa885db8d5e00db90b6b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>config_input_options</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a04ba5e50b22fa602d7dbf270c9f7297e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>run_mem_test</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a04a656410bbaac785e39f80e0f5d131b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a2916882019055c81a3e392d5ba420e82</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>tg_offset_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a8ee5d429a79626832f0b430173fdd495</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>cxl_mem_tg *</type>
      <name>tg_exe_</name>
      <anchorfile>classcxl__mem__tg_1_1cxl__tg__test.html</anchorfile>
      <anchor>a9fef372126f051b6ba4bb1525c0099a0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>dummy_afu::ddr_test</name>
    <filename>classdummy__afu_1_1ddr__test.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>ddr_test</name>
      <anchorfile>classdummy__afu_1_1ddr__test.html</anchorfile>
      <anchor>ae44b501624492b8a12c8b1e7a1cf0921</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~ddr_test</name>
      <anchorfile>classdummy__afu_1_1ddr__test.html</anchorfile>
      <anchor>a80c55e25295baf20a8e613464c38eb4d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classdummy__afu_1_1ddr__test.html</anchorfile>
      <anchor>a5062efb52f0cc3c8d744bd9f39f7298d</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classdummy__afu_1_1ddr__test.html</anchorfile>
      <anchor>a9ee7389012ae0941132809478d6a517b</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classdummy__afu_1_1ddr__test.html</anchorfile>
      <anchor>a5207a71d34529d7bfac6871282566edb</anchor>
      <arglist>(test_afu *afu, CLI::App *app)</arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::ddr_test_bank0_stat</name>
    <filename>uniondummy__afu_1_1ddr__test__bank0__stat.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank0__stat.html</anchorfile>
      <anchor>aa8c7a1aab7459f91472d152330afd777a452d9ce4a0d81f0a03fb108f90910eff</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank0__stat.html</anchorfile>
      <anchor>aa8c7a1aab7459f91472d152330afd777a452d9ce4a0d81f0a03fb108f90910eff</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ddr_test_bank0_stat</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank0__stat.html</anchorfile>
      <anchor>a27970572a33b9e78bb8234e89c9c1f3a</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank0__stat.html</anchorfile>
      <anchor>ae093a3331a30ae7929ab1c444a6efbaa</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::ddr_test_bank0_stat.__unnamed77__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1ddr__test__bank0__stat_8____unnamed77____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestPass</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae5896a6dd1997c0690acb5f4d22f376e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestFail</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a1bbe6c9cf15682789c37b5c2cfc0cb12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestTimeout</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a5ce4dd4e9bac3b3a0e1cfce3b0a3b4e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenFSMState</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a4706630556e088ca1cf93236bc0169d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved4</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a34deb90418b2775e73aee1a6d90defea</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::ddr_test_bank1_stat</name>
    <filename>uniondummy__afu_1_1ddr__test__bank1__stat.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank1__stat.html</anchorfile>
      <anchor>a2ac1436f349a087de1cce1e661730d57a43810c33a13e460071b76c35adbdc7fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank1__stat.html</anchorfile>
      <anchor>a2ac1436f349a087de1cce1e661730d57a43810c33a13e460071b76c35adbdc7fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ddr_test_bank1_stat</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank1__stat.html</anchorfile>
      <anchor>ade87612fe257022a517fdd835c78754b</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank1__stat.html</anchorfile>
      <anchor>a4550efbc4c68c7c2bbe9d0c827fb5135</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::ddr_test_bank1_stat.__unnamed80__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1ddr__test__bank1__stat_8____unnamed80____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestPass</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae5896a6dd1997c0690acb5f4d22f376e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestFail</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a1bbe6c9cf15682789c37b5c2cfc0cb12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestTimeout</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a5ce4dd4e9bac3b3a0e1cfce3b0a3b4e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenFSMState</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a4706630556e088ca1cf93236bc0169d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved4</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a34deb90418b2775e73aee1a6d90defea</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::ddr_test_bank2_stat</name>
    <filename>uniondummy__afu_1_1ddr__test__bank2__stat.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank2__stat.html</anchorfile>
      <anchor>ab45b1138a9723de818f886c0ec98791aa3b5fc2a73673ef8ffba47c5c0dc56baf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank2__stat.html</anchorfile>
      <anchor>ab45b1138a9723de818f886c0ec98791aa3b5fc2a73673ef8ffba47c5c0dc56baf</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ddr_test_bank2_stat</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank2__stat.html</anchorfile>
      <anchor>a5cfadb8d287a49d46efc9fae92dc6394</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank2__stat.html</anchorfile>
      <anchor>a8acbcc0bc572c7ad68a2491bda817ddb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::ddr_test_bank2_stat.__unnamed83__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1ddr__test__bank2__stat_8____unnamed83____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestPass</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae5896a6dd1997c0690acb5f4d22f376e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestFail</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a1bbe6c9cf15682789c37b5c2cfc0cb12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestTimeout</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a5ce4dd4e9bac3b3a0e1cfce3b0a3b4e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenFSMState</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a4706630556e088ca1cf93236bc0169d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved4</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a34deb90418b2775e73aee1a6d90defea</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::ddr_test_bank3_stat</name>
    <filename>uniondummy__afu_1_1ddr__test__bank3__stat.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank3__stat.html</anchorfile>
      <anchor>aec8cdfaa9320b2e7406c38a01d8408c7a5528e77330dfd1667928814465f6282b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank3__stat.html</anchorfile>
      <anchor>aec8cdfaa9320b2e7406c38a01d8408c7a5528e77330dfd1667928814465f6282b</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ddr_test_bank3_stat</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank3__stat.html</anchorfile>
      <anchor>af0e5d533620baa6e2703f4ead46ba799</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__bank3__stat.html</anchorfile>
      <anchor>ae6d442f6a5868de25bcd552a6ded3ba0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::ddr_test_bank3_stat.__unnamed86__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1ddr__test__bank3__stat_8____unnamed86____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestPass</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae5896a6dd1997c0690acb5f4d22f376e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestFail</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a1bbe6c9cf15682789c37b5c2cfc0cb12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenTestTimeout</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a5ce4dd4e9bac3b3a0e1cfce3b0a3b4e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TrafficGenFSMState</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a4706630556e088ca1cf93236bc0169d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved4</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a34deb90418b2775e73aee1a6d90defea</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::ddr_test_ctrl</name>
    <filename>uniondummy__afu_1_1ddr__test__ctrl.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__ctrl.html</anchorfile>
      <anchor>a2e9231688b257b0bf2fe224196c5f627a84fd585fd7d91ae83ffa4d845392c746</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__ctrl.html</anchorfile>
      <anchor>a2e9231688b257b0bf2fe224196c5f627a84fd585fd7d91ae83ffa4d845392c746</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ddr_test_ctrl</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__ctrl.html</anchorfile>
      <anchor>ab39713f855d5b6624d09a5d39b8934d6</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__ctrl.html</anchorfile>
      <anchor>a150f5e0256b7295210490e11080033ba</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::ddr_test_ctrl.__unnamed71__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1ddr__test__ctrl_8____unnamed71____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>DDRBank0StartTest</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a63afce2e700bbbba301fd9a8c63e76dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>DDRBank1StartTest</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a4b9b4dd11109036366c339e295c6671e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>DDRBank2StartTest</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a2839cffa7595242294856a029bf8b14b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>DDRBank3StartTest</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a69b14e807c3c4688b2f0a90b85227f1f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved4</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a34deb90418b2775e73aee1a6d90defea</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::ddr_test_stat</name>
    <filename>uniondummy__afu_1_1ddr__test__stat.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__stat.html</anchorfile>
      <anchor>a37397fb998ffbc4b31c2ce3b35441d4bad730fff971bc9d01c3a16f7b2fa183b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__stat.html</anchorfile>
      <anchor>a37397fb998ffbc4b31c2ce3b35441d4bad730fff971bc9d01c3a16f7b2fa183b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>ddr_test_stat</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__stat.html</anchorfile>
      <anchor>a647fb58c616da1a775f5ebf803d0d874</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1ddr__test__stat.html</anchorfile>
      <anchor>a9bd768d5f16bd893a6a724bc437e845c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::ddr_test_stat.__unnamed74__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1ddr__test__stat_8____unnamed74____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>NumDDRBank</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a518c2490375039e01f8742bb68ea1ffb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved8</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>af1815ef33cbdfc91a6c12fc6ce34caec</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>DFH</name>
    <filename>hssi__100g__cmd_8h.html</filename>
    <anchor>structDFH</anchor>
  </compound>
  <compound kind="union">
    <name>DFH.__unnamed132__</name>
    <filename>hssi__100g__cmd_8h.html</filename>
    <anchor>unionDFH_8____unnamed132____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>csr</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a1f8c50db95e9ead5645e32f8df5baa7b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>DFH.__unnamed132__.__unnamed134__</name>
    <filename>hssi__100g__cmd_8h.html</filename>
    <anchor>structDFH_8____unnamed132_____8____unnamed134____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>id</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ab80bb7740288fda1f201890375a60c8f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>major_rev</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a11521be0d4298edc1d011316a1032c99</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>next</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ad0cab90d8d20d57e2f2b9be52f7dd25d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>eol</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ad985f63c4c4b313bcc1d5db6c9b79f4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved41</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a0e8d7e3a05c9a274d1059add1dda5f98</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>minor_rev</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a5fab36f0641df8efa21f88fd3da3b304</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>version</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a2af72f100c356273d46284f6fd1dfc08</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>type</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a599dcce2998a6b40b1e38e8c6006cb0a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>dummy_afu::dummy_afu</name>
    <filename>classdummy__afu_1_1dummy__afu.html</filename>
    <base>opae::afu_test::afu</base>
    <member kind="function">
      <type></type>
      <name>dummy_afu</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>aae0500ab86280590301a8f2035871ea2</anchor>
      <arglist>(const char *afu_id=&quot;91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73&quot;)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a28786abacbd10d38cb75c7735b5f06d9</anchor>
      <arglist>(CLI::App *app, test_command::ptr_t test) override</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a6cfa065131aeb2c767957a427a45ca55</anchor>
      <arglist>(uint32_t offset) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a8dbe9b9d6a378367d4ebd928a40aa7df</anchor>
      <arglist>(uint32_t offset, T value) const</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>aba415621cca0fff2789c41e2f2a103ce</anchor>
      <arglist>(uint32_t offset, uint32_t i) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a924a1ad3f5a424df863ddaf368daeaab</anchor>
      <arglist>(uint32_t offset, uint32_t i, T value) const</arglist>
    </member>
    <member kind="function">
      <type>shared_buffer::ptr_t</type>
      <name>allocate</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a45941fbe75bbfbe64e1ad425576c51c8</anchor>
      <arglist>(size_t size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>fill</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a367b5eea5a958b43526f6caed83fa3cd</anchor>
      <arglist>(shared_buffer::ptr_t buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>fill</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>ab2520eacac74a76eeb451b2434d7bacf</anchor>
      <arglist>(shared_buffer::ptr_t buffer, uint32_t value)</arglist>
    </member>
    <member kind="function">
      <type>event::ptr_t</type>
      <name>register_interrupt</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>af04b7db0dd1a4cc07a892805e02b4a4f</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>interrupt_wait</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a41f2ecf19989492da3d175c1f1307c45</anchor>
      <arglist>(event::ptr_t event, int timeout=-1)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>compare</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a5b15cc613c3c02bca2247f52b281e316</anchor>
      <arglist>(shared_buffer::ptr_t b1, shared_buffer::ptr_t b2, uint32_t count=0)</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read_register</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a7338b50ed10fd0eb076ed439c6734bed</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>volatile T *</type>
      <name>register_ptr</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>aaaf5867f6ca84c1c1f6e6701b463b24e</anchor>
      <arglist>(uint32_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_register</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a8575458fcf22efea09c3feb33195e043</anchor>
      <arglist>(uint32_t offset, T *reg)</arglist>
    </member>
    <member kind="function" protection="private">
      <type>uint32_t</type>
      <name>get_offset</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>ad76648d08dd4f8ca7fcd93f2a0cf0a57</anchor>
      <arglist>(uint32_t base, uint32_t i) const</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>uint32_t</type>
      <name>count_</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>ae6e804471d96d97b1fefd1be976b1525</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>std::map&lt; uint32_t, uint32_t &gt;</type>
      <name>limits_</name>
      <anchorfile>classdummy__afu_1_1dummy__afu.html</anchorfile>
      <anchor>a559a248f4c1be5a66bf80d164bf5b5c4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::error</name>
    <filename>classopae_1_1fpga_1_1types_1_1error.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; error &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a1ebeb4e4a63d9807d50c13b98a07ac2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>error</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a81fa63e406170f777b19ede3889845be</anchor>
      <arglist>(const error &amp;e)=delete</arglist>
    </member>
    <member kind="function">
      <type>error &amp;</type>
      <name>operator=</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a736a48278182c76699a959192ab23bf7</anchor>
      <arglist>(const error &amp;e)=delete</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>name</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a30ead52bd6a86f4a3cc7045e9dce0c20</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>can_clear</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a7adabe502dd600e493263d684359e7ac</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>read_value</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>acc656f405ba3e3b500604ad64ff0b970</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~error</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a98daa1df4a7d000945797a3259153a6d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_error_info</type>
      <name>c_type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a9817aa22e5e5ba22a32bd203ebdf22a8</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static error::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a75af4be53a11d3e79639a84c79209567</anchor>
      <arglist>(token::ptr_t tok, uint32_t num)</arglist>
    </member>
    <member kind="function" protection="private">
      <type></type>
      <name>error</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a1acdf18b1cb09e0ee1539a6f845c8251</anchor>
      <arglist>(token::ptr_t token, uint32_t num)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>token::ptr_t</type>
      <name>token_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a8fed4f6624e3d0644525642468c52d60</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_error_info</type>
      <name>error_info_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>a686b42984c8fce14c6ebfb102d8a9a64</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>uint32_t</type>
      <name>error_num_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1error.html</anchorfile>
      <anchor>afb19c6ae95eb21e047925477adc9f3b4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::event</name>
    <filename>classopae_1_1fpga_1_1types_1_1event.html</filename>
    <class kind="struct">opae::fpga::types::event::type_t</class>
    <member kind="typedef">
      <type>std::shared_ptr&lt; event &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a018f8b02575a7ed99cc013463f319e03</anchor>
      <arglist></arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~event</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a9b74e1205c51020e8725b6d739564baa</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_event_handle</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>aecd61a05a41f306168b93da471d34d56</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_event_handle</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a25cefc7f2b28be11f45efcbb242e8beb</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>os_object</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a48c524ef2931034e37187ccde27b1ab8</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static event::ptr_t</type>
      <name>register_event</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>aebb5fa8450368e3288b25c729b6dbacf</anchor>
      <arglist>(handle::ptr_t h, event::type_t t, int flags=0)</arglist>
    </member>
    <member kind="function" protection="private">
      <type></type>
      <name>event</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a9c08908d257f2d172e623a16dc9d2b07</anchor>
      <arglist>(handle::ptr_t h, event::type_t t, fpga_event_handle event_h)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>handle::ptr_t</type>
      <name>handle_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>ad977259db55cfabca075e017a9e533e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>event::type_t</type>
      <name>type_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a81cce03e252830b2e49d5d1a9b6dfd18</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_event_handle</type>
      <name>event_handle_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a6fedb2b8e81a38dfdd4e0fb05b6576ce</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>int</type>
      <name>os_object_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1event.html</anchorfile>
      <anchor>a069e9a4b3148da8f05ee5eeb5f9ba4aa</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::except</name>
    <filename>classopae_1_1fpga_1_1types_1_1except.html</filename>
    <member kind="function">
      <type></type>
      <name>except</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>a84db8167d096b004d28c45e0d600a507</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>except</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>a607953d122ae8c0c9abf9bbaaa2bd3b0</anchor>
      <arglist>(fpga_result res, src_location loc) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>except</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>a4f48810eb0499e595115100915503e54</anchor>
      <arglist>(fpga_result res, const char *msg, src_location loc) noexcept</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>what</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>af08375009434a1748d613712108204c9</anchor>
      <arglist>() const noexcept override</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_result</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>ae3ed4c7022f603abeeb98bcb0f583cf7</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const std::size_t</type>
      <name>MAX_EXCEPT</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>af8bfb55f096792ad770e5cd5110e929a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>fpga_result</type>
      <name>res_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>a21c2cbaa1fa87c94c40b32853659e70b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>const char *</type>
      <name>msg_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>ad4ee478e3ab82c0ed668a53601d2e457</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>src_location</type>
      <name>loc_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>a52506f4e5b1af21ddfbecb251f028ec4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>char</type>
      <name>buf_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1except.html</anchorfile>
      <anchor>ab58e8409be0f32b8651a9dd54cadfaef</anchor>
      <arglist>[MAX_EXCEPT]</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::exception</name>
    <filename>classopae_1_1fpga_1_1types_1_1exception.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>exception</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1exception.html</anchorfile>
      <anchor>af39e3e5eef06006d6ab11bcd7d29ed50</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>fpga_error_info</name>
    <filename>types_8h.html</filename>
    <anchor>structfpga__error__info</anchor>
    <member kind="variable">
      <type>char</type>
      <name>name</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a63e0ccbeddbe44792e1c1f7375243751</anchor>
      <arglist>[64]</arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>can_clear</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a8f3a6c3707e10f13a55662a081494e69</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>fpga_metric</name>
    <filename>types_8h.html</filename>
    <anchor>structfpga__metric</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>metric_num</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a05d5221efe860841a350b4c81c26f8d2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>metric_value</type>
      <name>value</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a0e64950f7e2c90ca97009d564cca1d04</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>isvalid</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a43c9f922234a08609b8c586ec948e6b1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>fpga_metric_info</name>
    <filename>types_8h.html</filename>
    <anchor>structfpga__metric__info</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>metric_num</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>adfdb0c64c6e3d00d35f9c8996cf36463</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_guid</type>
      <name>metric_guid</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>af2e751ebec0b9526a73a59a29461a64a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>qualifier_name</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a38d4fbdc2acde2f19711c9733db6e24c</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>group_name</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>aa3930b74a55578cf4f030f9ae284b848</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>metric_name</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a3d336a0f9766b5c8e6a00069c2cc13e0</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>char</type>
      <name>metric_units</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a537b1236822cc923a44ed0440dc8500a</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>enum fpga_metric_datatype</type>
      <name>metric_datatype</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a3aef82b39037b6e4646f811fea4f93e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>enum fpga_metric_type</type>
      <name>metric_type</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a0f1ee188177b88a3ade06b635c45743b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>fpga_token_header</name>
    <filename>types_8h.html</filename>
    <anchor>structfpga__token__header</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>magic</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a2f1ca035b23b1d256c0897c87088e3e9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>vendor_id</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a9d6a110f9565c98f6af128b544ccd9d7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>device_id</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>adbd582f365bccde67633a77ee997b82f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>segment</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a9d99b916146fb609bf7266ee5facea92</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>bus</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a0e5a70d6ef42a26544d85ea50340fb36</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>device</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a44e4a70a27892c61fd9dc0ef8899ce87</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>function</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ae517a83a32a886df54165c583cc5aa0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_interface</type>
      <name>interface</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>aa51f72f309bb8d31b997592b802db331</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_objtype</type>
      <name>objtype</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>aedd4a4c842c5c40092041dc02ea059b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>object_id</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ada99f326ab6582aa3a209ef6b9bd4003</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_guid</type>
      <name>guid</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ae24e0a1d32b1f940e7ffb96a34c31345</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>subsystem_vendor_id</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>aeaca575d589325a187b9abea66a4192a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>subsystem_device_id</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a2710b5d055ec2e27b627e27dfa84007f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>fpga_version</name>
    <filename>types_8h.html</filename>
    <anchor>structfpga__version</anchor>
    <member kind="variable">
      <type>uint8_t</type>
      <name>major</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ad7129ea29f56980bf4031b860102565f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>minor</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a0f3eb4a91ba70d0a60f56174c685b762</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint16_t</type>
      <name>patch</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a3c5b1d07c0628aa9b6f3bd086ed0d962</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae::fpga::types::guid_t</name>
    <filename>structopae_1_1fpga_1_1types_1_1guid__t.html</filename>
    <member kind="function">
      <type></type>
      <name>guid_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>ae0c5217a465cc42331c0c101c739ccc4</anchor>
      <arglist>(fpga_properties *p)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>abac031f524410f0ce6d58c115374a09f</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator uint8_t *</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>af0f419fee764acd2a2cda2cc764a72f5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>const uint8_t *</type>
      <name>c_type</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>a8948f79cb21eb47782a7baf82ffa5941</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>guid_t &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>ab131fdece38cc112bd08244ede8a0f29</anchor>
      <arglist>(fpga_guid g)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>aef93c3b0b9502a01015897ab0ecaf44c</anchor>
      <arglist>(const fpga_guid &amp;g)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>parse</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>a79ba0567868025a34ffebc3eb24ddd5d</anchor>
      <arglist>(const char *str)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>a1ba6d75b0be7aa78ee1b31eb18c7bff3</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>ac8ae3baf06110627da463eb0d3a345fe</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>a051b019dd8b18fb38f8cf7a8db3079bb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>aaf1def223c6be127e63258486d8adc5a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>std::array&lt; uint8_t, 16 &gt;</type>
      <name>data_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>a45f001856e7648ccdb9d58bcc1f6cbde</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1guid__t.html</anchorfile>
      <anchor>afdbcb10d3044f41a17a31718e14b32df</anchor>
      <arglist>(std::ostream &amp;ostr, const guid_t &amp;g)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::handle</name>
    <filename>classopae_1_1fpga_1_1types_1_1handle.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; handle &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a89607a8f09e983ac77b3ba6b37c4f14e</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>handle</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a3e1f8ea9be017afa8aec56b463bd431d</anchor>
      <arglist>(const handle &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>handle &amp;</type>
      <name>operator=</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a80ced96b872b76d5ee29be562e2fe442</anchor>
      <arglist>(const handle &amp;)=delete</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~handle</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>abceb3d49e1d506aafe5ba6139cdf5d76</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_handle</type>
      <name>c_type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>af2581332aaa1b9e0d915e85be86bd8bd</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_handle</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a4c5a8da253bca8153e4049b3b41073dc</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>reconfigure</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>aa9681f30803220ff3731e8346f1ec50c</anchor>
      <arglist>(uint32_t slot, const uint8_t *bitstream, size_t size, int flags)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>read_csr32</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>aec8ba1c8e34ba72d15cf0cb55aad5682</anchor>
      <arglist>(uint64_t offset, uint32_t csr_space=0) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_csr32</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>ae943bcd838bba6589c1148e4921b052e</anchor>
      <arglist>(uint64_t offset, uint32_t value, uint32_t csr_space=0)</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>read_csr64</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a22d74175e9b8989ca6df82d2d03a86f8</anchor>
      <arglist>(uint64_t offset, uint32_t csr_space=0) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_csr64</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a5d47d9c51ebccd91fcc97b137cdfc4a0</anchor>
      <arglist>(uint64_t offset, uint64_t value, uint32_t csr_space=0)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_csr512</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a8f56dc33d92a7dfcbe9afe29afc96656</anchor>
      <arglist>(uint64_t offset, const void *value, uint32_t csr_space=0)</arglist>
    </member>
    <member kind="function">
      <type>uint8_t *</type>
      <name>mmio_ptr</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>ae8fd2777f41adc8ed370153e5cefe2ab</anchor>
      <arglist>(uint64_t offset, uint32_t csr_space=0) const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>reset</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a84150dd292ec5d7d964cc5d46767f6a1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>close</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a479044bb60b23c1626aa9d40d8a9c337</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>token::ptr_t</type>
      <name>get_token</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a0f4fd7f1aa67d46d548e9df2d31fc965</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>bind_sva</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>abbb11abca903c9a8395191c80cbbac46</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static handle::ptr_t</type>
      <name>open</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a90fe7bbbdd9bb0c1e02d2cb4449da42c</anchor>
      <arglist>(fpga_token token, int flags)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static handle::ptr_t</type>
      <name>open</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a8aa65702ffc38aa93d141f9cbd3c1cf0</anchor>
      <arglist>(token::ptr_t token, int flags)</arglist>
    </member>
    <member kind="function" protection="private">
      <type></type>
      <name>handle</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>ac9d2ff15b3db9080aa32a48ae24a9a93</anchor>
      <arglist>(fpga_handle h)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_handle</type>
      <name>handle_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a8ff3920d72109b9be18195550a4ade6c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_token</type>
      <name>token_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>a3f062ef9354f3dedfd2951e1c6722093</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>uint32_t</type>
      <name>pasid_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1handle.html</anchorfile>
      <anchor>aeb907ff1d5395de8b2e7183f52a91a96</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::he_cache_cmd</name>
    <filename>classhost__exerciser_1_1he__cache__cmd.html</filename>
    <base>host_exerciser::he_cmd</base>
    <member kind="function">
      <type></type>
      <name>he_cache_cmd</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a8c785d49f1bcdc97dfe628e2c6b73f0b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~he_cache_cmd</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a8a973b9525ef874425536585cb288c26</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a817e135851552b388df4af1039f51e39</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a41dc4ccfd8a759ce5e4b0c3938c4247a</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>adc969104b344f02d51efe21402fec3b4</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual uint64_t</type>
      <name>featureid</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>af36a2ca6d571c1a7afe9d5228c93743d</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual uint64_t</type>
      <name>guidl</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a3d15cc0e3c84891d1f51967714c669ae</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual uint64_t</type>
      <name>guidh</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a97721da6ffb1f46a1f2b0cee970774b8</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a7ea9ac7a5e8355e976ac96f496108f35</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_fpga_rd_cache_hit_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>af698fea396aaf2c7082070b775507974</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_fpga_wr_cache_hit_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a49cb8e1fbb80fbdf1b17086c5c41bfbc</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_fpga_rd_cache_miss_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a757f7242d634e1868f2bb74d96919895</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_fpga_wr_cache_miss_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>ad52d911bd48ffa4ed15f60b016949e0a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_host_rd_cache_hit_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a250727bf9827f6987ce8dbbb17bdd5f5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_host_wr_cache_hit_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a659777c8ecf70cf2ef2c01394da45ff1</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_host_rd_cache_miss_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>aca5dc55e285d1ee8da150d2cbdd7118f</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>he_run_host_wr_cache_miss_test</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>acd2cfde882772e681f798a074b09652a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_forcetestcmpl</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a060899ac85f7634bc4a83ec3b771234b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_continuousmode</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a762fa5cf0c7c151b44c11184a2f2e87a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>ae74784f608f368c80a40502de25e13a0</anchor>
      <arglist>(test_afu *afu, CLI::App *app)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>bool</type>
      <name>he_continuousmode_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>ad662e355b75488c1e0eb155335badaac</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_contmodetime_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>ac516e18c3f57820b78488ea0cfe24cdd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_linerep_count_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a31dce311f00c89932259680f7429945c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_stride_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a3d2902767bc99ffda4c5e09ac4eba815</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_test_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>aa4864cede6f93292a6ec178b5ee098c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>bool</type>
      <name>he_test_all_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a857ad914201b7132e32ff7c785576ea8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_dev_instance_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a53a60243c460e1f5465c2646d55f45c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>bool</type>
      <name>he_stride_cmd_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>ab6f97df0fc6c78c776a9876556b0770f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_cls_count_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>a8539577c086f6e436b3d80e4e21e14d3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>he_latency_iterations_</name>
      <anchorfile>classhost__exerciser_1_1he__cache__cmd.html</anchorfile>
      <anchor>ae9bd9e13f5b9cc9e2e64be79a2d59a8f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_cache_dsm_status</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__cache__dsm__status</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>test_completed</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a613c2340545dd8507a39bd4701f9b9aa</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>dsm_number</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ac46685a232de0ff1b1ecf41fda598876</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>res1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a361367b2e4575d2d790c02aacba4db28</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>err_vector</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a382ed250702e46303829e108f37b2f12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>num_ticks</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6eb6410d312b7ed6a73e941dca54001</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>num_reads</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7732502f9d03744f1f4c6470b17d4ad2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>num_writes</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aee8983e796569d69015613801d020f69</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>penalty_start</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5de12b551cbd91ed53ae54e3ec4c2b2c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>penalty_end</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a540a4a1063b04c34477558aef1c56f9d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>actual_data</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a97a983911d1619ad65e8a4e719230c75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>expected_data</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a60d0cf65b8c39bdc9bbd174a1e03397a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>res5</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a98230ce8d0fbd9b2482af58b344f9786</anchor>
      <arglist>[2]</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::he_cache_lpbk_cmd</name>
    <filename>classhost__exerciser_1_1he__cache__lpbk__cmd.html</filename>
    <base>host_exerciser::he_cmd</base>
    <member kind="function">
      <type></type>
      <name>he_cache_lpbk_cmd</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>aee33c1d26fa394cd89241adadab1c6d0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~he_cache_lpbk_cmd</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>aede4cf726a7c4070b764aeb57bc55f3e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>a0ea268f42319ad21821cdc68bcbbb044</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>ab7eff8c744aaba805f6a41635951ac15</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>a0626c7d3280125843db5e3a257e89e6c</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual uint64_t</type>
      <name>featureid</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>aef5411cc95e437d3d3e8f3e39eda0af3</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual uint64_t</type>
      <name>guidl</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>a7d031954754042cdfb36e2ac47859552</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual uint64_t</type>
      <name>guidh</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>a22f0bee536e83f805000e7cd81bbb154</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>a113c110ac6b3f414a087ed3019adafeb</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhost__exerciser_1_1he__cache__lpbk__cmd.html</anchorfile>
      <anchor>a00c9c2fbf616c3bd54c527209d5f8765</anchor>
      <arglist>(test_afu *afu, CLI::App *app)</arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_cfg</name>
    <filename>unionhost__exerciser_1_1he__cfg.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__cfg.html</anchorfile>
      <anchor>a56f9aa8ad88c2bf790d0b5ae824c375fa1e7f62f7cb24c3eaec09027463d3559c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__cfg.html</anchorfile>
      <anchor>a56f9aa8ad88c2bf790d0b5ae824c375fa1e7f62f7cb24c3eaec09027463d3559c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__cfg.html</anchorfile>
      <anchor>a4b7d9a2ed28517c9160238fefcb47913</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_cfg.__unnamed109__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__cfg_8____unnamed109____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>DelayEn</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab883db097e1612e1b9565a7641ba7aca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Continuous</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a535863a82f163709557e59e2eb8139a7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TestMode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a41bc18d21cc3fd3011c9cc6eea7093f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>ReqLen</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af2524811eba11dee000f9ed7bd5b0647</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>AtomicFunc</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6307735e860afcd790ab7669b4ba256</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Encoding</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a169a6f6b44766410bffebf76ff3dcf17</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Rsvd_19_14</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a13fbb3d45a1015e30cef2cefd1461cf7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TputInterleave</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae0cf447dd4dd6cd774637cabdb3fe8c0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>TestCfg</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af6635365e08b7c3bd682123ba84fbf4a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>IntrOnErr</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a802cc88e98f36ee11d967b0d082f1c65</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>IntrTestMode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a74169b89c0cee2049ba63145b0d48546</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>ReqLen_High</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a2ca3e42a1c64ae9a73ef7afbb34f4166</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Rsvd_63_32</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ad91da26f082f88fd9aa246286d3f777a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::he_cmd</name>
    <filename>classhost__exerciser_1_1he__cmd.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>he_cmd</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ae16a7859284a742398f3ec3340897faf</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~he_cmd</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a976086a0f5886ad570642a152c26d560</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>double</type>
      <name>he_num_xfers_to_bw</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>aa00565e79a248e635ea25986fdbf41d0</anchor>
      <arglist>(uint64_t num_lines, uint64_t num_ticks)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_perf_counters</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a6f837cc328c98339fb5d550c8d7b1c4a</anchor>
      <arglist>(he_cxl_latency cxl_latency=HE_CXL_LATENCY_NONE)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>print_csr</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ae405e118f67c274dcd176d16467df79c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>host_exerciser_errors</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a7677d9010bc02b8a520b12d1a1ff4625</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>parse_input_options</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>af3fd4e59c9b27facd6b78a7bded1bc04</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_wait_test_completion</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a6e8476a9963e59501717dee0e9cb8030</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_set_bias_mode</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ae2911007f89cd36d54495a534b405721</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_start_test</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a620842e34b55acae0ed65870f2aa3b17</anchor>
      <arglist>(const char *str=&quot;Test started ......&quot;)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>verify_numa_node</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a82424fd9783c7d0316e43fe030b18620</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_get_perf</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a467b5cdf6d89ca2c88b23aae59c0929e</anchor>
      <arglist>(double *perf_data, double *latency, he_cxl_latency cxl_latency=HE_CXL_LATENCY_NONE)</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>get_ticks</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a38150a59d4a41dfb181c3bc3661460fa</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>host_exerciser *</type>
      <name>host_exe_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>aee1926f43ce4f9e05d108bf7a4df862f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_clock_mhz_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a2d45199e7732d491a7064eea6ca587ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>numa_node_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>af6bc7cba5e077c535db0aa67c6a4bf07</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_target_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a5a1a28a1c79bd7d58f0b1741a094d2af</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>he_bias_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ab89826b350afe5ab4fce536a82fc745e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_ctl</type>
      <name>he_ctl_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ab34ca6db81f3f50c8ab368fd27e6a9c9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_info</type>
      <name>he_info_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ad49704bb2bf6ccfcd38a6f97c8f9d68c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_rd_config</type>
      <name>he_rd_cfg_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a4cc72bfa28ed905f24925b9a36f48209</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_wr_config</type>
      <name>he_wr_cfg_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>ac68c9fa27b0e8699ca401c64930ae262</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_rd_addr_table_ctrl</type>
      <name>rd_table_ctl_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a90da23fb1585370e8ae38ce073420c7b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_wr_addr_table_ctrl</type>
      <name>wr_table_ctl_</name>
      <anchorfile>classhost__exerciser_1_1he__cmd.html</anchorfile>
      <anchor>a48cc8df12dc8e7f58cf302865eb31b3c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_ctl</name>
    <filename>unionhost__exerciser_1_1he__ctl.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__ctl.html</anchorfile>
      <anchor>a10f96c1a5c3eb2f0867874e3bb314ae8a2557908f62c0398edfb735c9f1a3d74a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__ctl.html</anchorfile>
      <anchor>a10f96c1a5c3eb2f0867874e3bb314ae8a2557908f62c0398edfb735c9f1a3d74a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__ctl.html</anchorfile>
      <anchor>a10f96c1a5c3eb2f0867874e3bb314ae8a2557908f62c0398edfb735c9f1a3d74a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__ctl.html</anchorfile>
      <anchor>a066fc894df88d1a22eef795038163e7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__ctl.html</anchorfile>
      <anchor>a8c8243134da9edef12a3dbc9b4a1c1a7</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_ctl.__unnamed106__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__ctl_8____unnamed106____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>ResetL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a413c7c71e8225663455de88e7693026f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>Start</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa6122a65eaa676f700ae68d393054a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>ForcedTestCmpl</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aff7373bf6fefae550cf7a7d7431a3cbc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>Reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a942d4e37dd5607ab68e54755540d4a47</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_ctl.__unnamed9__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__ctl_8____unnamed9____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>ResetL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a413c7c71e8225663455de88e7693026f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Start</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa6122a65eaa676f700ae68d393054a37</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>ForcedTestCmpl</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aff7373bf6fefae550cf7a7d7431a3cbc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>bias_support</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a355866a76cb4578ae11a08d2c68a3fb8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a942d4e37dd5607ab68e54755540d4a47</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_dfh</name>
    <filename>unionhost__exerciser_1_1he__dfh.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dfh.html</anchorfile>
      <anchor>afc6564e2dc857abffaf960c201e54b47ae801660fc9ca8f8a7efee1806f06e092</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dfh.html</anchorfile>
      <anchor>afc6564e2dc857abffaf960c201e54b47ae801660fc9ca8f8a7efee1806f06e092</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dfh.html</anchorfile>
      <anchor>afc6564e2dc857abffaf960c201e54b47ae801660fc9ca8f8a7efee1806f06e092</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__dfh.html</anchorfile>
      <anchor>abe99ad2d58ffd76b1b69503d2102b9c0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_dfh.__unnamed3__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__dfh_8____unnamed3____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>CcipVersionNumber</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab18ae725c16f7d13bebcf9a10965a91d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>AfuMajVersion</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a8990d1a04a0288d5610740229a1d710f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>NextDfhOffset</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a717c28589ac515ac0fe0037939fee723</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>EOL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aecd284e164bdc984d109aea9e86e695c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a942d4e37dd5607ab68e54755540d4a47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>FeatureType</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ad3abdac08cd922b2c2e5fe50b23ae5b2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_dfh.__unnamed94__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__dfh_8____unnamed94____</anchor>
    <member kind="variable">
      <type>uint16_t</type>
      <name>CcipVersionNumber</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab18ae725c16f7d13bebcf9a10965a91d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>AfuMajVersion</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a8990d1a04a0288d5610740229a1d710f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>NextDfhOffset</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a717c28589ac515ac0fe0037939fee723</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>EOL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aecd284e164bdc984d109aea9e86e695c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>Reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a942d4e37dd5607ab68e54755540d4a47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>FeatureType</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ad3abdac08cd922b2c2e5fe50b23ae5b2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_dsm_base</name>
    <filename>unionhost__exerciser_1_1he__dsm__base.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__base.html</anchorfile>
      <anchor>acb515f9c25e46c77a8963ef34e389a53a8f44c4983f54559060c6bd7ac12ea155</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__base.html</anchorfile>
      <anchor>acb515f9c25e46c77a8963ef34e389a53a8f44c4983f54559060c6bd7ac12ea155</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__base.html</anchorfile>
      <anchor>a4f834c4cfa491df3ca05ab8e6cfe8a99</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_dsm_base.__unnamed6__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__dsm__base_8____unnamed6____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>DsmBase</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7a2e3d4c39e049659c85bedc400d04d7</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_dsm_baseh</name>
    <filename>unionhost__exerciser_1_1he__dsm__baseh.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__baseh.html</anchorfile>
      <anchor>af0890ec9b388e19d2ed635128f22473ba933f55091f3662dc9680455c0c00c175</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__baseh.html</anchorfile>
      <anchor>af0890ec9b388e19d2ed635128f22473ba933f55091f3662dc9680455c0c00c175</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__baseh.html</anchorfile>
      <anchor>afab6c9858e7da60ef44babd0fef3c01b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_dsm_baseh.__unnamed100__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__dsm__baseh_8____unnamed100____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>DsmBaseH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6eef0b707a38bf0c17d823d597492914</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_dsm_basel</name>
    <filename>unionhost__exerciser_1_1he__dsm__basel.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__basel.html</anchorfile>
      <anchor>a4ba084374684a6db722d333821be40dfaf5f62fe54fc710944d292a93faf315d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__basel.html</anchorfile>
      <anchor>a4ba084374684a6db722d333821be40dfaf5f62fe54fc710944d292a93faf315d1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__dsm__basel.html</anchorfile>
      <anchor>a865c3c35c16d80d8783b1befeaef57c0</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_dsm_basel.__unnamed97__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__dsm__basel_8____unnamed97____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>DsmBaseL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af2f33fb6529c50716b852abf71f13a5f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_dsm_status</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__dsm__status</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>test_completed</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab760bd8d5c508a85bc2e8994e2c7edde</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>dsm_number</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a260eaaadbf1e06745d95dba4eb729ff2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>res1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a82ecba774d0c87a69adaac15275662ab</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>err_vector</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bd23b5241622c4a5be147291842cf9e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>num_ticks</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4bbc5cfeffdb4a66c0a56fd46827eedd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>res2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a2765e988474ae1a9ffdbd140f7e5c49b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>num_reads_h</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ada5d5d02c713fa2f0b5682eecdb7a02c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>num_writes_h</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7ea350aafb859dcf3c0875eea35ac36c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>num_reads_l</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af0301349c3f7e495c64bbb0210bd595a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>num_writes_l</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ac677d9a7dc9bce8b5978864e8c0c7e6c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>penalty_start</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a0815448f7076ec6cdba7242f1c71b3c3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>res3</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6fe1e0417476d1f659dce6e605774949</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>penalty_end</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a93a3a735a7ff1de123303980281ff6ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>res4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ac762af688b03bc74b851276f63f18c52</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>ab_error_info</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>adafdaeeae346ed83510195ba039b9a26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>res5</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5d606261c3fb29410403b6ac1f1b2da0</anchor>
      <arglist>[7]</arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_err_status</name>
    <filename>unionhost__exerciser_1_1he__err__status.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__err__status.html</anchorfile>
      <anchor>a96d831a5eeb0dc4b2cd541ac93fe77b5a028d3076c757b34d401807870b9c6153</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__err__status.html</anchorfile>
      <anchor>a96d831a5eeb0dc4b2cd541ac93fe77b5a028d3076c757b34d401807870b9c6153</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__err__status.html</anchorfile>
      <anchor>a540ba9ec4957d1d78ca7099466d41490</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_err_status.__unnamed42__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__err__status_8____unnamed42____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>data_error</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ace29e8f64e6faa54eea929ee7b698c7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>rsvd1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a2e59fd4a2e93f065e82bde5d425fd332</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>err_index</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a94657f12bf0cd985092032df2e94de31</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>rsvd2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a0fcc3e567374f3c99a54d84d9697d68f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_error</name>
    <filename>unionhost__exerciser_1_1he__error.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__error.html</anchorfile>
      <anchor>a94e3a53258efc2d5f3443a57a10fb1c4acc50ce800065d386ffd094e004a8d4fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__error.html</anchorfile>
      <anchor>a94e3a53258efc2d5f3443a57a10fb1c4acc50ce800065d386ffd094e004a8d4fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__error.html</anchorfile>
      <anchor>a22f48a3ab9d55b81ec5cdb3ffaf8a73d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_error.__unnamed127__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__error_8____unnamed127____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>error</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>acb5e100e5a9a3e7f6d1fd97512215282</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Rsvd</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a38a176f40705de57c4d586ede54e11d7</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_inact_thresh</name>
    <filename>unionhost__exerciser_1_1he__inact__thresh.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__inact__thresh.html</anchorfile>
      <anchor>ad63660d7786206aff3172dede4ba4957ac52f9d87b88da47a40e657896bd0f1f7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__inact__thresh.html</anchorfile>
      <anchor>ad63660d7786206aff3172dede4ba4957ac52f9d87b88da47a40e657896bd0f1f7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__inact__thresh.html</anchorfile>
      <anchor>a15da343d72abc691b156ea26a9a2f242</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_inact_thresh.__unnamed112__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__inact__thresh_8____unnamed112____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>InactivtyThreshold</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a650faa473558269b0c481ed10514e92c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_info</name>
    <filename>unionhost__exerciser_1_1he__info.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__info.html</anchorfile>
      <anchor>a54559c2a617597132e7b00451fc097a1a68619f1544af2099040d6a11d15c956f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__info.html</anchorfile>
      <anchor>a54559c2a617597132e7b00451fc097a1a68619f1544af2099040d6a11d15c956f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__info.html</anchorfile>
      <anchor>aaaf25a13dda64632f7091cc099d097c5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_info.__unnamed12__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__info_8____unnamed12____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>write_addr_table_size</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a409e358f64917dbe8b6ac2ae142f0532</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>read_addr_table_size</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abafdaa1c66c594388107242944f8e182</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a942d4e37dd5607ab68e54755540d4a47</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_interrupt0</name>
    <filename>unionhost__exerciser_1_1he__interrupt0.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__interrupt0.html</anchorfile>
      <anchor>ad1187d50bc640e823a01bf91a17699a8aad6270f3018ef3ecd08ac00dafc61202</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__interrupt0.html</anchorfile>
      <anchor>ad1187d50bc640e823a01bf91a17699a8aad6270f3018ef3ecd08ac00dafc61202</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__interrupt0.html</anchorfile>
      <anchor>a7712031393541844e7ec8da9ed79810a</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_interrupt0.__unnamed115__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__interrupt0_8____unnamed115____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>apci_id</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afdf27665206a22cc41538cfc67c6fdf6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>VectorNum</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a0e44ca4fccde3904f37bdb81c96e2dbc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_num_lines</name>
    <filename>unionhost__exerciser_1_1he__num__lines.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__num__lines.html</anchorfile>
      <anchor>afaa4b94da434fddc2215783343abc0b7a455998b76ac8bc09bdecf81defe10b10</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__num__lines.html</anchorfile>
      <anchor>afaa4b94da434fddc2215783343abc0b7a455998b76ac8bc09bdecf81defe10b10</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__num__lines.html</anchorfile>
      <anchor>ad5e29a4528120ff7a97c415e96847dc4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_num_lines.__unnamed103__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__num__lines_8____unnamed103____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>NumCacheLines</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a06566b7faaee6a619ed7f099a7dba3da</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>Reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a942d4e37dd5607ab68e54755540d4a47</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_rd_addr_table_ctrl</name>
    <filename>unionhost__exerciser_1_1he__rd__addr__table__ctrl.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__addr__table__ctrl.html</anchorfile>
      <anchor>ab364c1ce3514cc4636fdaa774ebd959ea97ccf80be5a01ba6ef41757aaf4c5d6d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__addr__table__ctrl.html</anchorfile>
      <anchor>ab364c1ce3514cc4636fdaa774ebd959ea97ccf80be5a01ba6ef41757aaf4c5d6d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__addr__table__ctrl.html</anchorfile>
      <anchor>a7992154b1fd2f7028221f3aeb840b0a4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_rd_addr_table_ctrl.__unnamed36__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__rd__addr__table__ctrl_8____unnamed36____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>enable_address_table</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afa0605e0c361b4d97eb76beb46e12ed4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>enable_address_stride</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab3a002a398f5a657b2971a9b417a9850</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>stride</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a47101375aa96fa02cfb1d55fd00af989</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7f005c3fa691e77c52d3297cc2699072</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_rd_addr_table_data</name>
    <filename>unionhost__exerciser_1_1he__rd__addr__table__data.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__addr__table__data.html</anchorfile>
      <anchor>a399710215ffee70aba66616798e5d618a7e283de030043d65776332ff798d256a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__addr__table__data.html</anchorfile>
      <anchor>a399710215ffee70aba66616798e5d618a7e283de030043d65776332ff798d256a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__addr__table__data.html</anchorfile>
      <anchor>a2f5534dc68dad85e0bd8fecc4cae5d74</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_rd_addr_table_data.__unnamed39__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__rd__addr__table__data_8____unnamed39____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>address_table_value</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a78d9019fc5b454bf912bed7595d9f434</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_rd_config</name>
    <filename>unionhost__exerciser_1_1he__rd__config.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__config.html</anchorfile>
      <anchor>add7f706b34fd713432e0ee1e200cd087af62648fa0dd7296ecba4fc040431aa61</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__config.html</anchorfile>
      <anchor>add7f706b34fd713432e0ee1e200cd087af62648fa0dd7296ecba4fc040431aa61</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__config.html</anchorfile>
      <anchor>acd0ff90fadb9fa964c3ab553168c3739</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_rd_config.__unnamed33__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__rd__config_8____unnamed33____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>read_traffic_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9aa6fb6e85ad466e5fb25f8c78698326</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>continuous_mode_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abbcd533076ac57dfb4faccad32a40a47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>waitfor_completion</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e3ab1b93dfa6fc41ce1a432f784830b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>prewrite_sync_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aedb8dfd59548687260e8ee5d4f2f98c1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>postwrite_sync_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aebda1b9f4a54b21dd8a600aaa1561bdc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>data_pattern</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a622b5dd94df8d38fdb95dcb7f0684354</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>cl_evict_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3109ca2a59c024a637d163975a027d12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>opcode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a882c467d02aec1bcbdb94b1a18d9b210</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>line_repeat_count</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a99130608aa1134c70e232717498cd4ae</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7f005c3fa691e77c52d3297cc2699072</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_rd_num_lines</name>
    <filename>unionhost__exerciser_1_1he__rd__num__lines.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__num__lines.html</anchorfile>
      <anchor>ad1062785e4abe0012138967a02c89e45a90966ba5d0f0a4471e11870c43643c71</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__num__lines.html</anchorfile>
      <anchor>ad1062785e4abe0012138967a02c89e45a90966ba5d0f0a4471e11870c43643c71</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__rd__num__lines.html</anchorfile>
      <anchor>a26aeecdae96ecde45c38c3d446bf3f81</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_rd_num_lines.__unnamed30__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__rd__num__lines_8____unnamed30____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>read_num_lines</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a40616ff53c294fe14f47e171881250e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7f005c3fa691e77c52d3297cc2699072</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_status0</name>
    <filename>unionhost__exerciser_1_1he__status0.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__status0.html</anchorfile>
      <anchor>ad7e46e6a889b804f11aa0456f9a2aab7a6c63f5e3a29f1c8037609d3967c96bdd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__status0.html</anchorfile>
      <anchor>ad7e46e6a889b804f11aa0456f9a2aab7a6c63f5e3a29f1c8037609d3967c96bdd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__status0.html</anchorfile>
      <anchor>aa82dd2acf55f7c0314ce01198b07e204</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_status0.__unnamed121__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__status0_8____unnamed121____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>numWrites</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>affa900c1a78a86a553b0da57273eccad</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>numReads</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a312b0a681be1050d8c6995cbea955190</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_status1</name>
    <filename>unionhost__exerciser_1_1he__status1.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__status1.html</anchorfile>
      <anchor>a0a224e473d388f10b5139a661a526129a010f07a003f1e17fade4c450a3df4b52</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__status1.html</anchorfile>
      <anchor>a0a224e473d388f10b5139a661a526129a010f07a003f1e17fade4c450a3df4b52</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__status1.html</anchorfile>
      <anchor>a69efd0592ef6d73da063b7e786ead235</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_status1.__unnamed124__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__status1_8____unnamed124____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>numPendWrites</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af07abd81c5902d8c4f69ede7d940050e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>numPendReads</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a14ace46a4d47d176dd2b4aaab1eba30a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>numPendEmifWrites</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a0f084f1c8d6947d1b2f17b4be231269f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>numPendEmifReads</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a674275ab90fe378ec580306c9ea1e01d</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_stride</name>
    <filename>unionhost__exerciser_1_1he__stride.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__stride.html</anchorfile>
      <anchor>a16f62ef57b104697564e582db274a6e9a0590e61f18301509eff48efd04a46d2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__stride.html</anchorfile>
      <anchor>a16f62ef57b104697564e582db274a6e9a0590e61f18301509eff48efd04a46d2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__stride.html</anchorfile>
      <anchor>a5b93da4f8eed75ad0a82b96725e7852b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_stride.__unnamed130__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__stride_8____unnamed130____</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>Stride</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a8c7fd568f39aa9b653ac0088c2ecab39</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_swtest_msg</name>
    <filename>unionhost__exerciser_1_1he__swtest__msg.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__swtest__msg.html</anchorfile>
      <anchor>a4b1ef4b41d00821dda79bfbd12060df3a5b2011678763da5bfb8840740311b7fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__swtest__msg.html</anchorfile>
      <anchor>a4b1ef4b41d00821dda79bfbd12060df3a5b2011678763da5bfb8840740311b7fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__swtest__msg.html</anchorfile>
      <anchor>a73cc68766ca6718b51aeaf59ab0ff392</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_swtest_msg.__unnamed118__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__swtest__msg_8____unnamed118____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>swtest_msg</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a85ae47f248bb592cde3612a8e939ce1e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_wr_addr_table_ctrl</name>
    <filename>unionhost__exerciser_1_1he__wr__addr__table__ctrl.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__addr__table__ctrl.html</anchorfile>
      <anchor>a472c206e1bb60d08238126b6ba711fdfad279c414308721c163fbcc04e625828c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__addr__table__ctrl.html</anchorfile>
      <anchor>a472c206e1bb60d08238126b6ba711fdfad279c414308721c163fbcc04e625828c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__addr__table__ctrl.html</anchorfile>
      <anchor>a007fbeef981749640f30468b1e0d171f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_wr_addr_table_ctrl.__unnamed24__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__wr__addr__table__ctrl_8____unnamed24____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>enable_address_table</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afa0605e0c361b4d97eb76beb46e12ed4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>enable_address_stride</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab3a002a398f5a657b2971a9b417a9850</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>stride</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a47101375aa96fa02cfb1d55fd00af989</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7f005c3fa691e77c52d3297cc2699072</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_wr_addr_table_data</name>
    <filename>unionhost__exerciser_1_1he__wr__addr__table__data.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__addr__table__data.html</anchorfile>
      <anchor>a98b4726876d81bdcc0b07d0a7ebf79f5aa865dbe8b0ce86cc948eb89a9c5a529f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__addr__table__data.html</anchorfile>
      <anchor>a98b4726876d81bdcc0b07d0a7ebf79f5aa865dbe8b0ce86cc948eb89a9c5a529f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__addr__table__data.html</anchorfile>
      <anchor>a25f8d79057b757727f8a52ea17d644a1</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_wr_addr_table_data.__unnamed27__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__wr__addr__table__data_8____unnamed27____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>address_table_value</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a78d9019fc5b454bf912bed7595d9f434</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_wr_byte_enable</name>
    <filename>unionhost__exerciser_1_1he__wr__byte__enable.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__byte__enable.html</anchorfile>
      <anchor>ab8811165ca63e1087b0c4a62e95ef519af3ffc3366f4512ee826fb525e74e49d9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__byte__enable.html</anchorfile>
      <anchor>ab8811165ca63e1087b0c4a62e95ef519af3ffc3366f4512ee826fb525e74e49d9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__byte__enable.html</anchorfile>
      <anchor>afb26631d2d7bb32f48432f5d50c2b528</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_wr_byte_enable.__unnamed18__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__wr__byte__enable_8____unnamed18____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>write_byte_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9cd78695742babe209a3ae2d19a9f487</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_wr_config</name>
    <filename>unionhost__exerciser_1_1he__wr__config.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__config.html</anchorfile>
      <anchor>addc1be2d9ca76e4f4efd4dbd8116939ba11a7c071ac228a4c5fce909c3432e41a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__config.html</anchorfile>
      <anchor>addc1be2d9ca76e4f4efd4dbd8116939ba11a7c071ac228a4c5fce909c3432e41a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__config.html</anchorfile>
      <anchor>accd244b7cfc37a62fc034a2eb3098623</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_wr_config.__unnamed21__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__wr__config_8____unnamed21____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>write_traffic_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a39bbf8f3a91bd2bf34e1b019eb8e3757</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>continuous_mode_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abbcd533076ac57dfb4faccad32a40a47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>waitfor_completion</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e3ab1b93dfa6fc41ce1a432f784830b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>preread_sync_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a82595b3061136abd20a8680b652b9c92</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>postread_sync_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a8ce194270c81e5cd9e4636552c6b435d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>data_pattern</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a622b5dd94df8d38fdb95dcb7f0684354</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>cl_evict_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3109ca2a59c024a637d163975a027d12</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>opcode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a882c467d02aec1bcbdb94b1a18d9b210</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>line_repeat_count</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a99130608aa1134c70e232717498cd4ae</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7f005c3fa691e77c52d3297cc2699072</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>host_exerciser::he_wr_num_lines</name>
    <filename>unionhost__exerciser_1_1he__wr__num__lines.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__num__lines.html</anchorfile>
      <anchor>ab1b81bdc1273bb0e12e32540c6828478af95927787bf74e009771c806dd3e8bc5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__num__lines.html</anchorfile>
      <anchor>ab1b81bdc1273bb0e12e32540c6828478af95927787bf74e009771c806dd3e8bc5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unionhost__exerciser_1_1he__wr__num__lines.html</anchorfile>
      <anchor>a700f44b727f04eb4f301a48bf6b1264f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::he_wr_num_lines.__unnamed15__</name>
    <filename>namespacehost__exerciser.html</filename>
    <anchor>structhost__exerciser_1_1he__wr__num__lines_8____unnamed15____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>write_num_lines</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a04c4bda156e24e30095580f754d2baae</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>reserved</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7f005c3fa691e77c52d3297cc2699072</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::host_exerciser</name>
    <filename>classhost__exerciser_1_1host__exerciser.html</filename>
    <base>opae::afu_test::afu</base>
    <base>opae::afu_test::afu</base>
    <member kind="function">
      <type></type>
      <name>host_exerciser</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>af476538e4a9b2399a7f3b4bcffb5d5bc</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>aadff42bdf84d6b738d68a2a2e5e0c1c1</anchor>
      <arglist>(CLI::App *app, test_command::ptr_t test) override</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>option_passed</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>ac6f0c9cbf8e34553601bad5e8480eb6c</anchor>
      <arglist>(std::string option_str)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>host_exerciser</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>af476538e4a9b2399a7f3b4bcffb5d5bc</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>aadff42bdf84d6b738d68a2a2e5e0c1c1</anchor>
      <arglist>(CLI::App *app, test_command::ptr_t test) override</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a8092f68fd8d743ba34091f5ffb04106f</anchor>
      <arglist>(uint32_t offset) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a78c14cf576bbe30b8c66a880554efad3</anchor>
      <arglist>(uint32_t offset, T value) const</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>abc24eee3d5544c7a20067d6905e6cc55</anchor>
      <arglist>(uint32_t offset, uint32_t i) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>ae2406d616980e35005e17decefe9bf2c</anchor>
      <arglist>(uint32_t offset, uint32_t i, T value) const</arglist>
    </member>
    <member kind="function">
      <type>shared_buffer::ptr_t</type>
      <name>allocate</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a38badb3eee6257a99c81f0a1ba0d85ea</anchor>
      <arglist>(size_t size)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>fill</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a0fca3673f65def196d13a85606b54db0</anchor>
      <arglist>(shared_buffer::ptr_t buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>fill</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a063d8debaf36fbcd13422c921d6db0fe</anchor>
      <arglist>(shared_buffer::ptr_t buffer, uint32_t value)</arglist>
    </member>
    <member kind="function">
      <type>event::ptr_t</type>
      <name>register_interrupt</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a9d136f152136497d7e7ea37761b93d83</anchor>
      <arglist>(uint32_t vector)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>interrupt_wait</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a2967e89d8047270a55228a66b981be13</anchor>
      <arglist>(event::ptr_t event, int timeout=-1)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>compare</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a73fa7a3429af5054fba6797b5da501e5</anchor>
      <arglist>(shared_buffer::ptr_t b1, shared_buffer::ptr_t b2, uint32_t count=0)</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read_register</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a60a53080e51c0dd15c98a5d6ba84ba2a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>volatile T *</type>
      <name>register_ptr</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>aca8425c95098a7fb76e1d408d15395fc</anchor>
      <arglist>(uint32_t offset)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_register</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a2760e914967515e06841ffe1e72c8001</anchor>
      <arglist>(uint32_t offset, T *reg)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>get_offset</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a12e64e7e59fbb08d77031ebc34896af5</anchor>
      <arglist>(uint32_t base, uint32_t i) const</arglist>
    </member>
    <member kind="function">
      <type>token::ptr_t</type>
      <name>get_token</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>ab3b458245b721a21d0a71914143e6e62</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>option_passed</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>ac6f0c9cbf8e34553601bad5e8480eb6c</anchor>
      <arglist>(std::string option_str)</arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>count_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>ad927baa2ca873698997bbead6cb4ab5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_modes_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>aa6b9dea9352f98904af47c79b66ca1c5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_req_cls_len_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a87a3720b3c63612ff022fb5b5c0e3858</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_req_atomic_func_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a899fd45722eaa321e47c5f3dc7fe4177</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_req_encoding_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a0e8ea315da8c41a99c55398eeb92ca74</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>he_delay_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a75792fdfc946251fdd9f1aae67f1bebf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>he_continuousmode_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a20cc5bbf52f222333d621147240cd017</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>he_test_all_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>af576502d3d9ea0b719cd4bb58008fe4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_interleave_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>aa2779adbaf8d70ce736df41c1e1bca3a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_interrupt_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a1ef2e163e5a794cf8cc3b2fba702a01a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_contmodetime_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>adbecdb4af46f92a991d722daa3766876</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>he_clock_mhz_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a9992594aceb0b6efa167a01047873e35</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::map&lt; uint32_t, uint32_t &gt;</type>
      <name>limits_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser.html</anchorfile>
      <anchor>a32d8380022ae3db7e076991e64096144</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::host_exerciser_cmd</name>
    <filename>classhost__exerciser_1_1host__exerciser__cmd.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>host_exerciser_cmd</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a39b41493c2a243ad8e9eed18b6d61d1a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~host_exerciser_cmd</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a53dc085be65fff680e433ee29793ab51</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>host_exerciser_status</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>ac8ae82a49783ce23453d196c0dfe137b</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>host_exerciser_errors</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a93731bfcf8563a4f664e61dd0eccda4c</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>host_exerciser_swtestmsg</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a571f127ba35f9348482090610f26a3fa</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>cacheline_aligned_addr</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>aede1dc5b8f51b0a9119abb2cab54dd30</anchor>
      <arglist>(uint64_t num)</arglist>
    </member>
    <member kind="function">
      <type>double</type>
      <name>he_num_xfers_to_bw</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a69151663bf338cf126429e7cdd555432</anchor>
      <arglist>(uint64_t num_lines, uint64_t num_ticks)</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>dsm_num_ticks</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a9999c693487dffaa84bcdf13401c80e9</anchor>
      <arglist>(const volatile he_dsm_status *dsm_status)</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>dsm_num_reads</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a533fb9839b1d008fff8a3ef65e13ab2e</anchor>
      <arglist>(const volatile he_dsm_status *dsm_status)</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>dsm_num_writes</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a92d3684fd6d98c706ff5708f3d22b18c</anchor>
      <arglist>(const volatile he_dsm_status *dsm_status)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_perf_counters</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a9b75ff69ffc1f2078b5e9fb5c3f74830</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_interrupt</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a324b2ebdd099b3e8ed6e12281fa95628</anchor>
      <arglist>(event::ptr_t ev)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_wait_test_completion</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a23e2128ba0753d1b767983a6cf8f11d8</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_forcetestcmpl</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a672161648b4f66883bc47ef0d6fe0b57</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_init_src_buffer</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a9bf1efb0f48fe335643cf4e7d77dadd7</anchor>
      <arglist>(shared_buffer::ptr_t buffer)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_dump_buffer</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a3f45f83a1b1baf50f08787d25b866d33</anchor>
      <arglist>(shared_buffer::ptr_t buffer, const char *msg)</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>he_compare_buffer</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a1d77b83392d4cb0e0f21677ce73fffdd</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>he_continuousmode</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a81766a7d2da59b6e66a89981f4f7f5c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_cfg_reqlen</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a91adae5c2765cd1a16dec63e5e26e893</anchor>
      <arglist>(hostexe_req_len req_len)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>set_cfg_reqlen</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a7e74e36eaff5db9b33291fbe4748dee1</anchor>
      <arglist>(uint32_t req_len)</arglist>
    </member>
    <member kind="function">
      <type>hostexe_req_len</type>
      <name>get_cfg_reqlen</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a2653d2dbc6852f4a87f2b4584df6f736</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>parse_input_options</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>aa52560a3df999e96661079da1f1e4262</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>run_single_test</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>aa97f70a05e8316fa5b6039effbceb8d2</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>run_all_tests</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>ae6e828551852c834e830f9bfeb13bbd5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a83697b1ec3679aee9edec5b94bde79fa</anchor>
      <arglist>(test_afu *afu, CLI::App *app)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_cfg</type>
      <name>he_lpbk_cfg_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>abe17e7a5912b89f2cccffc7b2c119cb0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_ctl</type>
      <name>he_lpbk_ctl_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a995fa8ea50dfc6b59a80784a20e56866</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>host_exerciser *</type>
      <name>host_exe_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a427ab13cdfdbe49b60b4f3c225899d44</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>shared_buffer::ptr_t</type>
      <name>source_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>adecb328ddf1fd375c17afb03f02d401b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>shared_buffer::ptr_t</type>
      <name>destination_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>ac88ce251d60f9496d039fa8125920b50</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>shared_buffer::ptr_t</type>
      <name>dsm_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a15060baa3ae0403a90156b7d7eefdad1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>he_interrupt0</type>
      <name>he_interrupt_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a5f81b3e1ce55057b7d1e49fe2a9133a9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>token::ptr_t</type>
      <name>token_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>ad32d7e0c503d15815bc66c01bf4c2b22</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>hostexe_req_len</type>
      <name>he_lpbk_max_reqlen_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a414be1769450fa74ee0e622159f7ebfb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t</type>
      <name>he_lpbk_api_ver_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a78dab209d059322d173d77caff13bd16</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>bool</type>
      <name>he_lpbk_atomics_supported_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a709bbfaba5a6a52fd2b05a5caf5a3415</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>bool</type>
      <name>is_ase_sim_</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__cmd.html</anchorfile>
      <anchor>a37e774e7e6d25187d0801e5f77ecec1c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::host_exerciser_lpbk</name>
    <filename>classhost__exerciser_1_1host__exerciser__lpbk.html</filename>
    <base>host_exerciser::host_exerciser_cmd</base>
    <member kind="function">
      <type></type>
      <name>host_exerciser_lpbk</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__lpbk.html</anchorfile>
      <anchor>ae4b34910b78e02ba644c5035014d3611</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~host_exerciser_lpbk</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__lpbk.html</anchorfile>
      <anchor>a0ca8801096015d8abefc887041dcda07</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__lpbk.html</anchorfile>
      <anchor>a427eb571ae0d6a13307cd69687741ba6</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__lpbk.html</anchorfile>
      <anchor>a422ab0c863af2211448578f01d49c39d</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__lpbk.html</anchorfile>
      <anchor>ae8ec166b98127b3f1f577ca8bc1e7040</anchor>
      <arglist>() const override</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>host_exerciser::host_exerciser_mem</name>
    <filename>classhost__exerciser_1_1host__exerciser__mem.html</filename>
    <base>host_exerciser::host_exerciser_cmd</base>
    <member kind="function">
      <type></type>
      <name>host_exerciser_mem</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__mem.html</anchorfile>
      <anchor>a69ef3a8fbc6f9216d8a503abf9cfda50</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~host_exerciser_mem</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__mem.html</anchorfile>
      <anchor>a5f479f04b5086cf6176a6a8d054b74d9</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__mem.html</anchorfile>
      <anchor>a80bfac0224d74c52d42e2b57222e1504</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__mem.html</anchorfile>
      <anchor>a22101d1a6fe6dbc791af67bc5c60de56</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhost__exerciser_1_1host__exerciser__mem.html</anchorfile>
      <anchor>aaa97db87a8f37e7626fe40332f1f80b6</anchor>
      <arglist>() const override</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_100g_cmd</name>
    <filename>classhssi__100g__cmd.html</filename>
    <base>hssi_cmd</base>
    <member kind="function">
      <type></type>
      <name>hssi_100g_cmd</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a272bac05983a284c0bf972d3d487258d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a2ac9bd89910c7d6b4f3d5c0ef3ccf252</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a0e8f86e83c2d30a042da6f16e41cb3ce</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a04f3d33cea2a2112156e2d3c055fd904</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>timestamp_in_seconds</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a17a10d74f5c3836a7cfe847c618073e9</anchor>
      <arglist>(uint64_t x) const</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>total_per_second</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a17f87d943b75bf98510e103bf94bbb9e</anchor>
      <arglist>(uint64_t y, uint64_t z) const</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>data_64</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>aeecaf85339dcf800a0fa53a456df27c2</anchor>
      <arglist>(uint32_t lowerbytes, uint32_t higherbytes) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>read_performance</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>ad86c7d2fe50e9f4a08090a06f4e9e81c</anchor>
      <arglist>(perf_data *perf, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>calc_performance</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a77bb0634f9901672e02df84aa5a43d25</anchor>
      <arglist>(perf_data *old_perf, perf_data *new_perf, perf_data *perf, uint64_t size) const</arglist>
    </member>
    <member kind="function">
      <type>uint8_t</type>
      <name>stall_enable</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>aa60e47554dd2bae485f49d773255f7b0</anchor>
      <arglist>(hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>uint8_t</type>
      <name>stall_disable</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>aaf54dae4a089c878c40bbb830994a3fe</anchor>
      <arglist>(hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_ctrl_config</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a5f9156deb3980fa310215378a77c7b85</anchor>
      <arglist>(hssi_afu *hafu, ctrl_config config_data) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_csr_addr</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a06b70f259d026f9409986b9f0b3790d0</anchor>
      <arglist>(hssi_afu *hafu, uint64_t bin_src_addr, uint64_t bin_dest_addr) const</arglist>
    </member>
    <member kind="function">
      <type>std::ostream &amp;</type>
      <name>print_monitor_headers</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a059acd53ef63ce8eda3f0e9e8b8f09f1</anchor>
      <arglist>(std::ostream &amp;os, uint32_t max_timer) const</arglist>
    </member>
    <member kind="function">
      <type>std::ostream &amp;</type>
      <name>print_monitor_data</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a5d3d2ed202b316571178094d2ccc0c85</anchor>
      <arglist>(std::ostream &amp;os, perf_data *data, int port, uint32_t timer) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>select_port</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a8ac850cb4095cd6a8be0a7af0bbfa9f1</anchor>
      <arglist>(int port, ctrl_config config_data, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>capture_perf</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a5df04375e87839df67bc031db777b440</anchor>
      <arglist>(perf_data *old_perf_data, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>std::ostream &amp;</type>
      <name>run_monitor</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a094298cb31537a46b339965fe8196a01</anchor>
      <arglist>(std::ostream &amp;os, perf_data *old_perf_data, int port, hssi_afu *hafu, uint32_t timer)</arglist>
    </member>
    <member kind="function">
      <type>char</type>
      <name>get_user_input</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>aac9bacf0ad601f646a759426c31feaa4</anchor>
      <arglist>(char key) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>quit_monitor</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a711ab34a9639ea0ca56622e8aa355a69</anchor>
      <arglist>(uint8_t size, std::string move, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>reset_monitor</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a31fd86c8dc63ea8efcaf9baa09d4021f</anchor>
      <arglist>(std::vector&lt; int &gt; port_, std::string move, ctrl_config config_data, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>move_cursor</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>ab23e7abcc352add26ab6c3f726100410</anchor>
      <arglist>(uint8_t size, std::string move) const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>acf6da90316e7dd11f43f60df7a720926</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a74c6040308212cd53dc598359ee319f0</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function">
      <type>std::ostream &amp;</type>
      <name>print_registers</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>adedc4e33cb2ef457b66822f80ec5373c</anchor>
      <arglist>(std::ostream &amp;os, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::vector&lt; int &gt;</type>
      <name>port_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a3ed679b346bbdb71b10257b07f31298d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>eth_loopback_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>af47a5b14c39f63416f61c7e2cff1829b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>num_packets_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a02a72d1c81ed666a40bf28cd3524607d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>gap_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a49339b1918720ef7901cf78aacbd1327</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>pattern_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>ab035b798f21f9a36e1f4dd47023ef754</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>src_addr_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>ae1cb27623d716c598f1c3f9d1efea893</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>dest_addr_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a823ae50e9262fffd9f0a81348164c9f0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>eth_ifc_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>af3db6112939ee4024b08f4c800c18ef6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>start_size_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a903f4acd96b9833b465389c4197c6428</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>end_size_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>afb165abcd4bc91fb813dbb0badeefedf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>end_select_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>aa0a117f3d240d96f78a6355922292b76</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>continuous_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a87688bc407d45f0c648a3a8636eca3db</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>contmonitor_</name>
      <anchorfile>classhssi__100g__cmd.html</anchorfile>
      <anchor>a42069ef54edff7924b1e7127c67f9a22</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_10g_cmd</name>
    <filename>classhssi__10g__cmd.html</filename>
    <base>hssi_cmd</base>
    <member kind="function">
      <type></type>
      <name>hssi_10g_cmd</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a4ee82ae4682d6a531aacfd35ba4f5910</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>af33928e3684381253c94728aa000932a</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a680495300041fec9bde25551362ce195</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a12f9d3d69d8822cdfb3c35b22c4654d8</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a5b43c5b65f93917f74fb158c1261aeee</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a0762cc43180e6bcd5fabc98461450949</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function">
      <type>std::ostream &amp;</type>
      <name>print_registers</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>accc68e7c9f9a76c7a3fad9c62a3fe8be</anchor>
      <arglist>(std::ostream &amp;os, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>port_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>ac229f059a06d51e434c6b9da850abc42</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>dst_port_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>ac19fc7cd8246a8310eea494c5aaf1f39</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>src_port_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>aefe79d4931439f4850137dc040e94f0b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>eth_loopback_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>ae634755c920470721628ab5ab06bdf58</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>he_loopback_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a101007526d07095943bb836fa68496cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>num_packets_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>aadd0f9db0c701bc02a23e89badc64bb0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>random_length_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>abd37648368fdfa6caeb887fe0d8460c0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>random_payload_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a4e0767e0f12273dace4fa75eddf1decd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>packet_length_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>ace75496cfab7ca96e4791575fce22207</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>src_addr_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>abd3ca25b1ca1732b68b4e6caab09ad40</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>dest_addr_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a4078a0208d3d4c0ed25b553056ca6064</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>eth_ifc_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>ab68fd6be1b8441fa2e6eb4a8be65dd2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>rnd_seed0_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a918f3fcf2f29b5ab82d3ed9e6c7e77de</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>rnd_seed1_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a551dce10ac132f2ee200d1c7a37edeec</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>rnd_seed2_</name>
      <anchorfile>classhssi__10g__cmd.html</anchorfile>
      <anchor>a6b6d1608738d2c16d4c1925130c6263e</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_200g_400g_cmd</name>
    <filename>classhssi__200g__400g__cmd.html</filename>
    <base>hssi_cmd</base>
    <member kind="function">
      <type></type>
      <name>hssi_200g_400g_cmd</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>a6576b0cd822b6905274bfc28378cb4ff</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>aa5bca1a5de01d4255636ecea60a873dc</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>a401a104aba6f68ad2eb94ce2e164bb40</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>a191d1450d1fcfe7023bfac66efa7fc74</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>ac363df78df0704d447931248f8e889ed</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>a42c0511ab227d3cc418dc944d4ba24ab</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function">
      <type>std::ostream &amp;</type>
      <name>print_registers</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>a69cf64d96002221fa5916edc02336b3f</anchor>
      <arglist>(std::ostream &amp;os, hssi_afu *hafu) const</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint32_t</type>
      <name>num_packets_</name>
      <anchorfile>classhssi__200g__400g__cmd.html</anchorfile>
      <anchor>a41bf807af704f3bfb68c77eeca9a42f8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_afu</name>
    <filename>classhssi__afu.html</filename>
    <base>opae::afu_test::afu</base>
    <member kind="function">
      <type></type>
      <name>hssi_afu</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>ab13aa4088100a9ac313862ae7d5c7edc</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>glob_ethernet_interface</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>a6eaa4d98775709e3833b8f91b9bf24c9</anchor>
      <arglist>(const std::string &amp;glob_pattern, std::string &amp;eth_ifc)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>ethernet_interface</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>a0a5190683fbccdd1ed97c89fc148981e</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>mbox_write</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>ac82026f100617124ebf962ebf3283d4a</anchor>
      <arglist>(uint64_t port_select, uint16_t offset, uint32_t data)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>mbox_write</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>a86638ffebd322c2f71d60e1ac6ba63ea</anchor>
      <arglist>(uint16_t offset, uint32_t data)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>mbox_read</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>aa1c3fda715b06ed934c8a0bee85786fd</anchor>
      <arglist>(uint64_t port_select, uint16_t offset)</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>mbox_read</name>
      <anchorfile>classhssi__afu.html</anchorfile>
      <anchor>a8555e906fd776954d39ddf432a7996f8</anchor>
      <arglist>(uint16_t offset)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_cmd</name>
    <filename>classhssi__cmd.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>hssi_cmd</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>a9241a77d197ac297be1ea1759c24fe08</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>double</type>
      <name>clock_freq_for</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>a4d645c1913180d63ad93330589ab0892</anchor>
      <arglist>(test_afu *afu) const</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>set_pkt_filt_dest</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>a0cfec35cb1e5910bd54f2ad9122fe6f9</anchor>
      <arglist>(const std::string &amp;dfl_dev, uint64_t bin_dest_addr) const</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>mac_bits_for</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>a153a98d3c481a21b25abed3a6e25192f</anchor>
      <arglist>(std::string addr) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>run_process</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>a308020145dd686d7b5ab1a4bf22ee9c7</anchor>
      <arglist>(const std::string &amp;proc)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>show_eth_stats</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>ab9e34f47f9b8507649f79894950ed59e</anchor>
      <arglist>(const std::string &amp;eth)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>enable_eth_loopback</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>a49f44707a8d19094be36a91ffa570e79</anchor>
      <arglist>(const std::string &amp;eth, bool enable)</arglist>
    </member>
    <member kind="function">
      <type>std::string</type>
      <name>int_to_hex</name>
      <anchorfile>classhssi__cmd.html</anchorfile>
      <anchor>ae4683ed6127d0ac876b8f24a83bbdc1f</anchor>
      <arglist>(X x) const</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_pkt_filt_100g_cmd</name>
    <filename>classhssi__pkt__filt__100g__cmd.html</filename>
    <base>hssi_100g_cmd</base>
    <member kind="function">
      <type></type>
      <name>hssi_pkt_filt_100g_cmd</name>
      <anchorfile>classhssi__pkt__filt__100g__cmd.html</anchorfile>
      <anchor>a3e3db45f2f96f7f65082f964bd42a10a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhssi__pkt__filt__100g__cmd.html</anchorfile>
      <anchor>ab866053ecaeb4ae7b92dc9ac94dfef35</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhssi__pkt__filt__100g__cmd.html</anchorfile>
      <anchor>af6666e8a956190f1032cef988eae2fe6</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhssi__pkt__filt__100g__cmd.html</anchorfile>
      <anchor>a3e74b621df757adbb0b9d8472a43248f</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhssi__pkt__filt__100g__cmd.html</anchorfile>
      <anchor>ae22f15c9dcfeb894b0928e930f3b91f0</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>dfl_dev_</name>
      <anchorfile>classhssi__pkt__filt__100g__cmd.html</anchorfile>
      <anchor>a20f52fb50c5f3319258a92ef3cf1a90f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>hssi_pkt_filt_10g_cmd</name>
    <filename>classhssi__pkt__filt__10g__cmd.html</filename>
    <base>hssi_10g_cmd</base>
    <member kind="function">
      <type></type>
      <name>hssi_pkt_filt_10g_cmd</name>
      <anchorfile>classhssi__pkt__filt__10g__cmd.html</anchorfile>
      <anchor>a8cbdbbc9a74d56cebbd22f72160a0166</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classhssi__pkt__filt__10g__cmd.html</anchorfile>
      <anchor>a55e611ba5a11117f8aa4f7a8d7377b64</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classhssi__pkt__filt__10g__cmd.html</anchorfile>
      <anchor>a417eba4f19d689ece861b13af35d9535</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classhssi__pkt__filt__10g__cmd.html</anchorfile>
      <anchor>aedff941fc7221899f93f2088b0d582e7</anchor>
      <arglist>(CLI::App *app) override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classhssi__pkt__filt__10g__cmd.html</anchorfile>
      <anchor>a805bdf9c5b4bc30e93caa61443c708a1</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>std::string</type>
      <name>dfl_dev_</name>
      <anchorfile>classhssi__pkt__filt__10g__cmd.html</anchorfile>
      <anchor>ac78f3cacdec1400c15f9c2273027afb8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::invalid_param</name>
    <filename>classopae_1_1fpga_1_1types_1_1invalid__param.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>invalid_param</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1invalid__param.html</anchorfile>
      <anchor>a5a6b2f0ac6aca0e54a5215f44073e983</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>dummy_afu::lpbk_test</name>
    <filename>classdummy__afu_1_1lpbk__test.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>lpbk_test</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>acafe03f97d7f9d00cb9ca4739636d8b3</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~lpbk_test</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>a9eb12336596a64101201947174014603</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>a00af051356068d57e098ff6a19a265d6</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>a034296c117fca4fe17f50bd90a47da63</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>a22a64a8f79ef8e002d3ff75534d2f8e0</anchor>
      <arglist>(test_afu *afu, CLI::App *app)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>shared_buffer::ptr_t</type>
      <name>source_</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>a18905dcda929cfd0d01c3e138a521fdf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>shared_buffer::ptr_t</type>
      <name>destination_</name>
      <anchorfile>classdummy__afu_1_1lpbk__test.html</anchorfile>
      <anchor>aff6e9778ec29dc1c2f8547863a3cbbdc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>host_exerciser::MapKeyComparator</name>
    <filename>structhost__exerciser_1_1MapKeyComparator.html</filename>
    <member kind="function">
      <type>bool</type>
      <name>operator()</name>
      <anchorfile>structhost__exerciser_1_1MapKeyComparator.html</anchorfile>
      <anchor>a44a01a2bb3c22ff9bf4b0b411e655ae2</anchor>
      <arglist>(const std::string &amp;a, const std::string &amp;b) const</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>mem_alloc</name>
    <filename>mem__alloc_8h.html</filename>
    <anchor>structmem__alloc</anchor>
    <member kind="variable">
      <type>struct mem_link</type>
      <name>free</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>a5921b4ec4ba3b8fa0087a5211802f995</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct mem_link</type>
      <name>allocated</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>af373018dfb1f354faf143b9fb04a3ca4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>mem_link</name>
    <filename>mem__alloc_8h.html</filename>
    <anchor>structmem__link</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>address</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>a4c968ceba2b3fd177cf209d5157b9cb6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>size</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>ac99991a999b487d1a7afa70c3f9aadcf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct mem_link *</type>
      <name>prev</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>a1225e65e5841948cc0b81c533cfeaab7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct mem_link *</type>
      <name>next</name>
      <anchorfile>mem__alloc_8h.html</anchorfile>
      <anchor>a0b90e6b837df3dd0afab6221a87963cc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>dummy_afu::mem_test_ctrl</name>
    <filename>uniondummy__afu_1_1mem__test__ctrl.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1mem__test__ctrl.html</anchorfile>
      <anchor>ad4f50101bc511fd663c221f8384c10cead77d8b6de99fa585c67d093213b1a39e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>uniondummy__afu_1_1mem__test__ctrl.html</anchorfile>
      <anchor>ad4f50101bc511fd663c221f8384c10cead77d8b6de99fa585c67d093213b1a39e</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>mem_test_ctrl</name>
      <anchorfile>uniondummy__afu_1_1mem__test__ctrl.html</anchorfile>
      <anchor>a0d74ae4d2ec938b7ba7cd49b2ef5828a</anchor>
      <arglist>(uint64_t v)</arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>uniondummy__afu_1_1mem__test__ctrl.html</anchorfile>
      <anchor>a9c14b3653f5071a114a11cda827847d3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>dummy_afu::mem_test_ctrl.__unnamed68__</name>
    <filename>namespacedummy__afu.html</filename>
    <anchor>structdummy__afu_1_1mem__test__ctrl_8____unnamed68____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>StartTest</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a238d901edc5daef041291938b01bf000</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Reserved1</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a93876cb309016fa725517b046474c44b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>mem_tg::mem_tg</name>
    <filename>classmem__tg_1_1mem__tg.html</filename>
    <base>opae::afu_test::afu</base>
    <member kind="function">
      <type></type>
      <name>mem_tg</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>a0368cda5c46775544441ab3e09d8f595</anchor>
      <arglist>(const char *afu_id=AFU_ID)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>ac85aabb722c846ac48ec6be8f06b1f6a</anchor>
      <arglist>(CLI::App *app, test_command::ptr_t test) override</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>get_offset</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>ac1c0cff60b1dc60b9e0d5be523bdf9b3</anchor>
      <arglist>(uint32_t base, uint32_t i) const</arglist>
    </member>
    <member kind="function">
      <type>token::ptr_t</type>
      <name>get_token</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>a4c347292a75105027f821ae3d0cffca0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>duplicate</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>a9960e153571fac14c64eb604b459f54d</anchor>
      <arglist>(mem_tg *duplicate_obj) const</arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>count_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>aa878e5e65a3b83b2d3807993605e08a9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::vector&lt; std::string &gt;</type>
      <name>mem_ch_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>ae8eaf03b82b2f4e79920aba58feb1cee</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>loop_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>af7b8104540b7f3acf91f0e7115733318</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>wcnt_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>ada68c840a6bb6a09ad61e7f2d777257c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>rcnt_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>abbf19481486cb11b3a7e6907ffbeff60</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>bcnt_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>a666f25cb5cdd7bdcdfd945e18e1cbf2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>stride_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>af774e6feb4e8dcacef4b3f23cc7d5ede</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>pattern_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>aa158961ac57acd0d6cbe5b4701e693f0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>mem_speed_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>a921692228f2ce76e661eec4e58eb959c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>status_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>a610e8d4094bf6d993634daf2f0ba7fdf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_offset_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>aad68b3a2869f06e8a141dce0af8dae67</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::map&lt; uint32_t, uint32_t &gt;</type>
      <name>limits_</name>
      <anchorfile>classmem__tg_1_1mem__tg.html</anchorfile>
      <anchor>abfe7736f3e7edc20f0232a3d8df7d6b3</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>cxl_mem_tg::mem_tg0_count</name>
    <filename>unioncxl__mem__tg_1_1mem__tg0__count.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg0__count.html</anchorfile>
      <anchor>a123af22dbcb86da65843eda854efae30ac3b9b421a674466165ecbff1a02e7ffc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg0__count.html</anchorfile>
      <anchor>a123af22dbcb86da65843eda854efae30ac3b9b421a674466165ecbff1a02e7ffc</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg0__count.html</anchorfile>
      <anchor>af8f26542fdec3205f42fd60e743ade1f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>cxl_mem_tg::mem_tg0_count.__unnamed58__</name>
    <filename>namespacecxl__mem__tg.html</filename>
    <anchor>structcxl__mem__tg_1_1mem__tg0__count_8____unnamed58____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>count</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae2942a04780e223b215eb8b663cf5353</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>cxl_mem_tg::mem_tg1_count</name>
    <filename>unioncxl__mem__tg_1_1mem__tg1__count.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg1__count.html</anchorfile>
      <anchor>ad3fcfd06558d7f34995ee57b6c5deb6ba89b8f65025fedebfac46a85efe0b627c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg1__count.html</anchorfile>
      <anchor>ad3fcfd06558d7f34995ee57b6c5deb6ba89b8f65025fedebfac46a85efe0b627c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg1__count.html</anchorfile>
      <anchor>a27b48fef8d4f3bd4e4d1ffe146f6e4f4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>cxl_mem_tg::mem_tg1_count.__unnamed61__</name>
    <filename>namespacecxl__mem__tg.html</filename>
    <anchor>structcxl__mem__tg_1_1mem__tg1__count_8____unnamed61____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>count</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae2942a04780e223b215eb8b663cf5353</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>cxl_mem_tg::mem_tg_ctl</name>
    <filename>unioncxl__mem__tg_1_1mem__tg__ctl.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg__ctl.html</anchorfile>
      <anchor>ae89ad06cbbc38d660c0035221dadfe8ea7247aca431da924bf3a05696a0c9b245</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg__ctl.html</anchorfile>
      <anchor>ae89ad06cbbc38d660c0035221dadfe8ea7247aca431da924bf3a05696a0c9b245</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg__ctl.html</anchorfile>
      <anchor>a064066ce04a14c6a0003a1bd82934285</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>cxl_mem_tg::mem_tg_ctl.__unnamed52__</name>
    <filename>namespacecxl__mem__tg.html</filename>
    <anchor>structcxl__mem__tg_1_1mem__tg__ctl_8____unnamed52____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_capability</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a6d34c45ffdac925a16d3b3c2047ce174</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Rsvd_63_3</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae660654513e1a8d461652d3b08845337</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>counter_clear</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a87e6ccccd6c09e1f75b9761c7f80f224</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>cxl_mem_tg::mem_tg_status</name>
    <filename>unioncxl__mem__tg_1_1mem__tg__status.html</filename>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg__status.html</anchorfile>
      <anchor>ac8b6a489b14ba2987089db65edb906d4a7fe5ab5b3039f80443210df5817b5b19</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>offset</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg__status.html</anchorfile>
      <anchor>ac8b6a489b14ba2987089db65edb906d4a7fe5ab5b3039f80443210df5817b5b19</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>unioncxl__mem__tg_1_1mem__tg__status.html</anchorfile>
      <anchor>a8d3557bfa4cdee21e16f8c81f8e916ad</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>cxl_mem_tg::mem_tg_status.__unnamed55__</name>
    <filename>namespacecxl__mem__tg.html</filename>
    <anchor>structcxl__mem__tg_1_1mem__tg__status_8____unnamed55____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_status0</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a6a78db517f38d353f35ea90e0bda5ea2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_status1</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa49dfa7fbe5bfa23db7a05e8bc1a35e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_status2</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ada25e0aee7b78074d825ae2861d7c43c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>tg_status3</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a04450fbc0102ac045df8cad55a3a3e75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>Rsvd_63_16</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>af5227312123178cb13d634298515daeb</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>metric_group</name>
    <filename>object__api_8c.html</filename>
    <anchor>structmetric__group</anchor>
    <member kind="variable">
      <type>const char *</type>
      <name>name</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a3aa3cf1268d2ae7783b4b295096be14f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_token</type>
      <name>token</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>ac8f8f854299ecb50f1de8960918bf99b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_object</type>
      <name>object</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a3f6d185fb89ae4822f85b88b2a7f8f60</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>bus</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a1cb9ac5fa1b24cd6cb7cc9d32d0b6ab1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>device</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a3645114207ff895c276f6d36cbe485ea</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t</type>
      <name>function</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a6e18dd1bdbfd92fc4d4d4ff658b8718d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>named_object</type>
      <name>objects</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>aaac5ae86f698ed76dd5309bca67ae781</anchor>
      <arglist>[32]</arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>abb44d12920d2d9d1041b00254973ebe2</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>metric_threshold</name>
    <filename>types_8h.html</filename>
    <anchor>structmetric__threshold</anchor>
    <member kind="variable">
      <type>char</type>
      <name>metric_name</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a63c4387d2ad646feb31c6f3dac3fb987</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>upper_nr_threshold</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>abf891f1ebf3009188f833562401c01c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>upper_c_threshold</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ab4343b83ae47a10bfdeb223297d761f9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>upper_nc_threshold</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a2bc47facaf3ef988f041e2401a9ca38b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>lower_nr_threshold</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a731aa786b82861be91dba21a71b8a729</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>lower_c_threshold</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a7e56a0642bbea2cdd5c688e56bd622d7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>lower_nc_threshold</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a023d220c713f20476c51c164b59d0a7b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>threshold</type>
      <name>hysteresis</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>ae212230698181daf72fbbfe71a0fb1e5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>metric_value</name>
    <filename>types_8h.html</filename>
    <anchor>unionmetric__value</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>ivalue</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a3157831ae4c4f8ecda1f61bdaea2ef13</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>double</type>
      <name>dvalue</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a40409d5d9e5b9f16e3270bed34404871</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>float</type>
      <name>fvalue</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a171100c87da379700719f03ccac03092</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>bvalue</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>aee6957b949306871afb1d006b066409f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>dummy_afu::mmio_test</name>
    <filename>classdummy__afu_1_1mmio__test.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>mmio_test</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a5c31bd8c1bc38afb83f814f6406ca937</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~mmio_test</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a70bd0d084015442089e2fa1023351107</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a1bfdb1527b7284659cba26e0d6dc7777</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>ae11ac9f28ce155a19cac986cdb054fa2</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual void</type>
      <name>add_options</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>ad4e1d2ed8ec389198b09a5ce9c940aa5</anchor>
      <arglist>(CLI::App *app)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a11bd14e2eaedd19a9b759f30dcc845a7</anchor>
      <arglist>(test_afu *afu, CLI::App *app)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>run_perf</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>acd17b7274c884cc6f0b700e00c3c4db1</anchor>
      <arglist>(dummy_afu *afu, CLI::App *app)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>uint32_t</type>
      <name>count_</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a04fa6884b8ffc217cace4c2581eb1ac8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>uint32_t</type>
      <name>sp_index_</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a345f52c800f65b0f97f01756fa20e419</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>perf_</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a9df22c82e86ee0d2205676960c01094e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>uint32_t</type>
      <name>width_</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a1475d7585eecd522b39b0767eff6e0a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>std::string</type>
      <name>op_</name>
      <anchorfile>classdummy__afu_1_1mmio__test.html</anchorfile>
      <anchor>a97e5092750cb8d3e671b6fc6d66eebcc</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>n5010</name>
    <filename>n5010-test_8c.html</filename>
    <anchor>structn5010</anchor>
    <member kind="variable">
      <type>fpga_properties</type>
      <name>filter</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>afcfc923993a0c3d7ed0919eae0ab8c7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_token</type>
      <name>token</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a0cd89aec173afe291731dbf496a15e89</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_handle</type>
      <name>handle</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a2a3c98ab8a1308a684bbb24cc837bbce</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_guid</type>
      <name>guid</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ae7ae9573b72d429d0b60811d6f6d0256</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>base</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>abc1821c363984324ab8a00dca071a2a6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const struct n5010_test *</type>
      <name>test</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ab54ff35470c946192ab662a1602e8da8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>port</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>a67f77d17592900684681c485ed981f3d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>bool</type>
      <name>debug</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ae68e58f88c71441c72ecb477b69f8523</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint</type>
      <name>open_mode</name>
      <anchorfile>n5010-test_8c.html</anchorfile>
      <anchor>ad0222c8836fd4aeb7a6da6d56d3176d6</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>n5010_test</name>
    <filename>structn5010__test.html</filename>
    <member kind="variable">
      <type>const char *</type>
      <name>name</name>
      <anchorfile>structn5010__test.html</anchorfile>
      <anchor>a5700b8f7e0f3b2bccfb8d4ad4328a1a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_result(*</type>
      <name>func</name>
      <anchorfile>structn5010__test.html</anchorfile>
      <anchor>a475cef5da0662cd3a96b1e0bcbdc67c8</anchor>
      <arglist>)(struct n5010 *n5010)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>named_object</name>
    <filename>object__api_8c.html</filename>
    <anchor>structnamed__object</anchor>
    <member kind="variable">
      <type>const char *</type>
      <name>name</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a3d81c56fb382b314f10925c9e5be78b6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_object</type>
      <name>object</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>aa4757f48068a9b3df2147f150705570e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>value</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a84bcb0c225f1d4836f67933b4bba3021</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>delta</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>aa60add432596827c889e6e9e22957d88</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::no_access</name>
    <filename>classopae_1_1fpga_1_1types_1_1no__access.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>no_access</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1no__access.html</anchorfile>
      <anchor>aa76892a97af22964ee8540443451bdbb</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::no_daemon</name>
    <filename>classopae_1_1fpga_1_1types_1_1no__daemon.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>no_daemon</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1no__daemon.html</anchorfile>
      <anchor>a0086811cfd62bc1871403d306bc5e4fb</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::no_driver</name>
    <filename>classopae_1_1fpga_1_1types_1_1no__driver.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>no_driver</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1no__driver.html</anchorfile>
      <anchor>a0ca809c3c85c1e25cb1782eb46d9f8ea</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::no_memory</name>
    <filename>classopae_1_1fpga_1_1types_1_1no__memory.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>no_memory</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1no__memory.html</anchorfile>
      <anchor>a60094b671e76cd6f6e9cd875886e74f6</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::not_found</name>
    <filename>classopae_1_1fpga_1_1types_1_1not__found.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>not_found</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1not__found.html</anchorfile>
      <anchor>a2e4911d2a10d677d0ec9361addc52210</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::not_supported</name>
    <filename>classopae_1_1fpga_1_1types_1_1not__supported.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>not_supported</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1not__supported.html</anchorfile>
      <anchor>ab67655acb41e441e126ef6e29003aa4b</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_hash_map</name>
    <filename>structopae__hash__map.html</filename>
    <member kind="variable">
      <type>uint32_t</type>
      <name>num_buckets</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>a63068218b83c3a932af0744583d4d52d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>hash_seed</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>a358c866ba990349f752a6783bea8d1f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>opae_hash_map_item **</type>
      <name>buckets</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>a8465d6028ce340508e8d819130d08ecf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>flags</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>a14ba2f6e8a4681fdd9c1b2a2e1b619e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>cleanup_context</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>afaa41a41911809ba9b50adae302cc606</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t(*</type>
      <name>key_hash</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>afa7da5e87861430406531aa356f689ca</anchor>
      <arglist>)(uint32_t num_buckets, uint32_t hash_seed, void *key)</arglist>
    </member>
    <member kind="variable">
      <type>int(*</type>
      <name>key_compare</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>ad17135479de98e413bd2070306664864</anchor>
      <arglist>)(void *keya, void *keyb)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>key_cleanup</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>af8011981f985e00b4c6a0fff31da3eac</anchor>
      <arglist>)(void *key, void *context)</arglist>
    </member>
    <member kind="variable">
      <type>void(*</type>
      <name>value_cleanup</name>
      <anchorfile>structopae__hash__map.html</anchorfile>
      <anchor>a9fda942cec9921df1bfee590ebf0600d</anchor>
      <arglist>)(void *value, void *context)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_hash_map_item</name>
    <filename>hash__map_8h.html</filename>
    <anchor>structopae__hash__map__item</anchor>
    <member kind="variable">
      <type>void *</type>
      <name>key</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a2bc84efb62b050093f82300b050f9476</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>void *</type>
      <name>value</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a90e4ff6c4d033b0076d462069671f3c5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct _opae_hash_map_item *</type>
      <name>next</name>
      <anchorfile>hash__map_8h.html</anchorfile>
      <anchor>a426277c984ea3e44d733ade585aef233</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_uio</name>
    <filename>uio_8h.html</filename>
    <anchor>structopae__uio</anchor>
    <member kind="variable">
      <type>char</type>
      <name>device_path</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>a956d6cbe927983ba6447e934e4b465e5</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>device_fd</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>a824941d1debb8fd8edf502f828b7d9a6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_uio_device_region *</type>
      <name>regions</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>a7b891a77bc896a02c7a0aaa0ee008468</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_uio_device_region</name>
    <filename>uio_8h.html</filename>
    <anchor>structopae__uio__device__region</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>region_index</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>a20fd192f4d257ba3e4b8622af81a1916</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>region_ptr</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>a5bf9d879f514ef0c88dde0ec5676082d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>region_page_offset</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>aa8c1dc455663807cd6178945a917692e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>region_size</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>aadf0885c5e3af4a91110e1b207b092bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_uio_device_region *</type>
      <name>next</name>
      <anchorfile>uio_8h.html</anchorfile>
      <anchor>ade4d30e7975d0d995bcc4056fc340bc5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio</anchor>
    <member kind="variable">
      <type>pthread_mutex_t</type>
      <name>lock</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ad9ef4790801de907acfa18409dedfaab</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>cont_device</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a7ca65a6a199f57795216b4af7e152b7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>char *</type>
      <name>cont_pciaddr</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a12cbdd053863723e3b51564dab3b1de5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>cont_fd</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a7cdb7430f296c4c469dc54f075014245</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_iova_range *</type>
      <name>cont_ranges</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a65cb87d5caf739177f1627f369704ea4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct mem_alloc</type>
      <name>iova_alloc</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a27de65bf440333362258d9da72fb93d7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_group</type>
      <name>group</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a5aa9019a215689bb45c6de05425489e0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_device</type>
      <name>device</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a0321bdb1c77dad0ad3f57d32d5f00de6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>opae_hash_map</type>
      <name>cont_buffers</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a421b8a46f5c5bca0d663f415f567dc19</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_buffer</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__buffer</anchor>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>buffer_ptr</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a10c301384270143e76d93675740d2a05</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>buffer_size</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a52535a9a9d8c7a129fb313d14a96dc47</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>buffer_iova</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a4bccc7115924108b58c421955a6b9775</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>flags</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a9e334d32ec820b06a9d263bc158da2ed</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_device</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__device</anchor>
    <member kind="variable">
      <type>int</type>
      <name>device_fd</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a5567cac847af54d1b68cd11de6d9071a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>device_config_offset</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a484988f748aa324957a2046bc2590e10</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>device_num_regions</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ad9ca3ae45cd69d3eec04297820c98db4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_device_region *</type>
      <name>regions</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ab45bac066081e611b23ff38c90ab6f26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>device_num_irqs</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ad4fc938dbf2c96ea60222912b4ed72ca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_device_irq *</type>
      <name>irqs</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a491523e147d7ae93d48b49d713bdce7c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_device_irq</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__device__irq</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>flags</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a8a7ea1141298b55129019cc8e1672bb9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>index</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ae80f357cdeaa6c6e20031cedcaad5712</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>count</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ae8531b9a37c51672f1e8693a8473cad2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int32_t *</type>
      <name>event_fds</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a066566f8808335166498fbce4d8a5211</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int32_t *</type>
      <name>masks</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a59f2bf38b7882ff09232e83ba7dea984</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_device_irq *</type>
      <name>next</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a80db0fd825bd3d7b73c4a70b86be60b5</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_device_region</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__device__region</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>region_index</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a276412974a0b0ffb91ac3200149761b0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>region_ptr</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a37bbecd990a16e1d985bece6d8395e86</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>region_size</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>add69a93ef216ab2ed1a053f506605f8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_sparse_info *</type>
      <name>region_sparse</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a52a91a9da49e5a61dbb5e8f511733aba</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_device_region *</type>
      <name>next</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ad58eb4a337da7515de0acca200a2463c</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_group</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__group</anchor>
    <member kind="variable">
      <type>char *</type>
      <name>group_device</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>aa7f7d7923d85bdb630b0b1ec49be5f92</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>group_fd</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a51d7ad18b5d27f7713877bc9e7f31c92</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_iova_range</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__iova__range</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>start</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>ae44d6d78fc59fec1829117f80f09d676</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>end</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a8e09837d655b7187d704d1dec69bd3e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_iova_range *</type>
      <name>next</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>acdadb8bd4497fd424a045de13f1c0570</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae_vfio_sparse_info</name>
    <filename>vfio_8h.html</filename>
    <anchor>structopae__vfio__sparse__info</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>index</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a0f2a85a947a552ace76e299bd66dc91d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>offset</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>afc2d2ce14bb3aeaba49efa5e8a063dfb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>size</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>adefec9a427d225b75b9b272710174df8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint8_t *</type>
      <name>ptr</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a68ad8ff776b8b47536d8f515eadffaa4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>struct opae_vfio_sparse_info *</type>
      <name>next</name>
      <anchorfile>vfio_8h.html</anchorfile>
      <anchor>a6557e0f64b5cb7f9e89218aaa535d583</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="union">
    <name>opae::afu_test::pcie_address</name>
    <filename>unionopae_1_1afu__test_1_1pcie__address.html</filename>
    <member kind="function" static="yes">
      <type>static pcie_address</type>
      <name>parse</name>
      <anchorfile>unionopae_1_1afu__test_1_1pcie__address.html</anchorfile>
      <anchor>afb2617e2ed297046217e2a47de73f5ab</anchor>
      <arglist>(const char *s)</arglist>
    </member>
    <member kind="variable">
      <type>struct opae::afu_test::pcie_address::@44</type>
      <name>fields</name>
      <anchorfile>unionopae_1_1afu__test_1_1pcie__address.html</anchorfile>
      <anchor>a7a062970db60a7bba46e10fd459eeb43</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>value</name>
      <anchorfile>unionopae_1_1afu__test_1_1pcie__address.html</anchorfile>
      <anchor>ab3ea036b3be673377eb47e8f6bdc5e5f</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae::afu_test::pcie_address.fields</name>
    <filename>namespaceopae_1_1afu__test.html</filename>
    <anchor>structopae_1_1afu__test_1_1pcie__address_8fields</anchor>
    <member kind="variable">
      <type>uint32_t</type>
      <name>function</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>ac1c425268e68385d1ab5074c17a94f14</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>device</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>a913f9c49dcb544e2087cee284f4a00b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>bus</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>ad20f83ee6933aa1ea047fe5cbd9c1fd5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>domain</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>aad5f82e879a9c5d6b5b442eb37e50551</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>perf_data</name>
    <filename>hssi__100g__cmd_8h.html</filename>
    <anchor>structperf__data</anchor>
    <member kind="variable">
      <type>volatile uint64_t</type>
      <name>tx_count</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ae1747e09f4ee6b78caedd6cce2360fc5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>volatile uint64_t</type>
      <name>rx_count</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>ac09d03c4186246c23aa04fba521b42e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>volatile uint64_t</type>
      <name>rx_good_packet_count</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a8c213a812b7613bee56cc02835ccb94a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>volatile uint64_t</type>
      <name>rx_pkt_sec</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a91811766745d11dc2a2f2c1b1a7dae22</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>bw</name>
      <anchorfile>hssi__100g__cmd_8h.html</anchorfile>
      <anchor>a4f9cb54703a3140df3e88d4b7600e107</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::properties</name>
    <filename>classopae_1_1fpga_1_1types_1_1properties.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; properties &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a00a34265b465af961adedd706615f348</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>properties</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>aac8f376e4bc640a71ff628f536a10584</anchor>
      <arglist>(const properties &amp;p)=delete</arglist>
    </member>
    <member kind="function">
      <type>properties &amp;</type>
      <name>operator=</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a6f3cb44a161ed02f4a9916d32851192b</anchor>
      <arglist>(const properties &amp;p)=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~properties</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>ab2c64365c493b7b08ff9207bb128ebbe</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_properties</type>
      <name>c_type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a9a976757deb492ada54d65c6ca1aefe9</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_properties</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>ac5a3a372377aea865cbb8345d737127d</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static properties::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>ab2b9142ce432de4aff9c0a20e9bc4c42</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static properties::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a736a6f59a42279056bf3b0877099bd10</anchor>
      <arglist>(fpga_guid guid_in)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static properties::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>abff4b12dfc1a08d861690a5b9a773fb9</anchor>
      <arglist>(fpga_objtype objtype)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static properties::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>aeeb323cca03dd9e26f8b769d9da9ce48</anchor>
      <arglist>(std::shared_ptr&lt; token &gt; t)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static properties::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a365bdca95b6af6592c714fad6a9d096a</anchor>
      <arglist>(fpga_token t)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static properties::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a3cf32f0e11436d76747a18222f18e4fa</anchor>
      <arglist>(std::shared_ptr&lt; handle &gt; h)</arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; fpga_objtype &gt;</type>
      <name>type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a63f080b4941991367ba9900af2e6a0b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint32_t &gt;</type>
      <name>num_errors</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a5913b325cac373affa91ed5d9196ad41</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint16_t &gt;</type>
      <name>segment</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a0a3dfe2e229074b9c2b895d2712d4fc0</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint8_t &gt;</type>
      <name>bus</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a8dc399536eb26ce0b4d434aa7e170316</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint8_t &gt;</type>
      <name>device</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a2da094689d6f9e1ff83a318863e96d95</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint8_t &gt;</type>
      <name>function</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a500427aedb78b3a8fdc68211e0d69bf1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint8_t &gt;</type>
      <name>socket_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a7e727ae54ad257953cd2b0e570c452a1</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint32_t &gt;</type>
      <name>num_slots</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a42d618739d9d8d00873457a6af5b8613</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint64_t &gt;</type>
      <name>bbs_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>adbda11d711131de6441cf8537321ec83</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; fpga_version &gt;</type>
      <name>bbs_version</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a6b0c7cd23a8c0b3e4c55f6c60ae7d620</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint16_t &gt;</type>
      <name>vendor_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>aa263ac16432be869fc6070209476a0fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint16_t &gt;</type>
      <name>device_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a16200ec67ed7b2bbf55475c68b932adb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint16_t &gt;</type>
      <name>subsystem_vendor_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a33f8fa0fd03531574fbd0f13e06b02ba</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint16_t &gt;</type>
      <name>subsystem_device_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a7a2df1b361f7f8ab5c0c2443631f6b30</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; char * &gt;</type>
      <name>model</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a41d099cbad8556f3de7f9ff8cf3f092e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint64_t &gt;</type>
      <name>local_memory_size</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a8ed3e82ae73b34aa2d729aba2cd8140c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint64_t &gt;</type>
      <name>capabilities</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a5a2ebe1ce0870eaa6bb578cbf49fe6ff</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint32_t &gt;</type>
      <name>num_mmio</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a64fabfeaa7795cef364ca444a6535ed8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint32_t &gt;</type>
      <name>num_interrupts</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a07f7a33e0efdc84d39c9ffc5d6374c72</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; fpga_accelerator_state &gt;</type>
      <name>accelerator_state</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>aa046d54251503d161faa02b99a64f0c4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; uint64_t &gt;</type>
      <name>object_id</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>af001f70fa59cd5c49cf28af50e2d2d5f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; fpga_token &gt;</type>
      <name>parent</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a800b2f5a7f12753741e542165a255eca</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>pvalue&lt; fpga_interface &gt;</type>
      <name>interface</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a2bd162e2835a510e9c32ab214c458af4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>guid_t</type>
      <name>guid</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a927bd5aa11cd56706046164da9494b58</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const std::vector&lt; properties::ptr_t &gt;</type>
      <name>none</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>ac9e35b67fef37b1d0ab8c6d17549a35c</anchor>
      <arglist></arglist>
    </member>
    <member kind="function" protection="private">
      <type></type>
      <name>properties</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>a7c81729de274502f8c5f7e1001060ebb</anchor>
      <arglist>(bool alloc_props=true)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties</type>
      <name>props_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1properties.html</anchorfile>
      <anchor>abc025fb8c6b491807571899f5841e0f8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae::fpga::types::pvalue</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <templarg></templarg>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; T, char * &gt;::value, fpga_result(*)(fpga_properties, T), fpga_result(*)(fpga_properties, T *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, T)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; T, char * &gt;::value, typename std::string, T &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; T &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const T &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const T &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(T &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; T &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; char * &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; char *, char * &gt;::value, fpga_result(*)(fpga_properties, char *), fpga_result(*)(fpga_properties, char * *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, char *)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; char *, char * &gt;::value, typename std::string, char * &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; char * &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const char * &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const char * &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(char * &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; char * &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; fpga_accelerator_state &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_accelerator_state, char * &gt;::value, fpga_result(*)(fpga_properties, fpga_accelerator_state), fpga_result(*)(fpga_properties, fpga_accelerator_state *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, fpga_accelerator_state)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_accelerator_state, char * &gt;::value, typename std::string, fpga_accelerator_state &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; fpga_accelerator_state &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const fpga_accelerator_state &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const fpga_accelerator_state &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(fpga_accelerator_state &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; fpga_accelerator_state &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; fpga_interface &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_interface, char * &gt;::value, fpga_result(*)(fpga_properties, fpga_interface), fpga_result(*)(fpga_properties, fpga_interface *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, fpga_interface)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_interface, char * &gt;::value, typename std::string, fpga_interface &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; fpga_interface &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const fpga_interface &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const fpga_interface &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(fpga_interface &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; fpga_interface &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; fpga_objtype &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_objtype, char * &gt;::value, fpga_result(*)(fpga_properties, fpga_objtype), fpga_result(*)(fpga_properties, fpga_objtype *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, fpga_objtype)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_objtype, char * &gt;::value, typename std::string, fpga_objtype &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; fpga_objtype &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const fpga_objtype &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const fpga_objtype &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(fpga_objtype &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; fpga_objtype &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; fpga_token &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_token, char * &gt;::value, fpga_result(*)(fpga_properties, fpga_token), fpga_result(*)(fpga_properties, fpga_token *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, fpga_token)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_token, char * &gt;::value, typename std::string, fpga_token &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; fpga_token &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const fpga_token &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const fpga_token &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(fpga_token &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; fpga_token &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; fpga_version &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_version, char * &gt;::value, fpga_result(*)(fpga_properties, fpga_version), fpga_result(*)(fpga_properties, fpga_version *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, fpga_version)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; fpga_version, char * &gt;::value, typename std::string, fpga_version &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; fpga_version &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const fpga_version &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const fpga_version &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(fpga_version &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; fpga_version &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; uint16_t &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint16_t, char * &gt;::value, fpga_result(*)(fpga_properties, uint16_t), fpga_result(*)(fpga_properties, uint16_t *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, uint16_t)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint16_t, char * &gt;::value, typename std::string, uint16_t &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; uint16_t &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const uint16_t &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const uint16_t &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(uint16_t &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; uint16_t &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; uint32_t &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint32_t, char * &gt;::value, fpga_result(*)(fpga_properties, uint32_t), fpga_result(*)(fpga_properties, uint32_t *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, uint32_t)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint32_t, char * &gt;::value, typename std::string, uint32_t &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; uint32_t &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const uint32_t &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const uint32_t &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(uint32_t &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; uint32_t &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; uint64_t &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint64_t, char * &gt;::value, fpga_result(*)(fpga_properties, uint64_t), fpga_result(*)(fpga_properties, uint64_t *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, uint64_t)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint64_t, char * &gt;::value, typename std::string, uint64_t &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; uint64_t &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const uint64_t &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const uint64_t &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(uint64_t &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; uint64_t &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>pvalue&lt; uint8_t &gt;</name>
    <filename>structopae_1_1fpga_1_1types_1_1pvalue.html</filename>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint8_t, char * &gt;::value, fpga_result(*)(fpga_properties, uint8_t), fpga_result(*)(fpga_properties, uint8_t *)&gt;::type</type>
      <name>getter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a40b6ecebfe823b05ac5afe418ccb373d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>fpga_result(*</type>
      <name>setter_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ab6fde171274b6487500035771f9938c6</anchor>
      <arglist>)(fpga_properties, uint8_t)</arglist>
    </member>
    <member kind="typedef">
      <type>std::conditional&lt; std::is_same&lt; uint8_t, char * &gt;::value, typename std::string, uint8_t &gt;::type</type>
      <name>copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1b43105d816e48f6aa6bfd23f18ab929</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>ad7f2f4a9ba69e28ca33fa69f341e3fa6</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>pvalue</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a1e692a4696493a3368f29efc6651e119</anchor>
      <arglist>(fpga_properties *p, getter_t g, setter_t s)</arglist>
    </member>
    <member kind="function">
      <type>pvalue&lt; uint8_t &gt; &amp;</type>
      <name>operator=</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a97a474b20191622779815c21be1957f7</anchor>
      <arglist>(const uint8_t &amp;v)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>operator==</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a6a7af8459d8f54fa79d72bccf54d636b</anchor>
      <arglist>(const uint8_t &amp;other)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a88159c07b32bbe0c6ed9d7def238bf19</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>update</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a0ac919fa7c6a0ac864cba14cf0f900c7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator copy_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7e0338e36a3156d9052c40b58dcdb15d</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_result</type>
      <name>get_value</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a525cba878a020f7151f2f2a0d34fbede</anchor>
      <arglist>(uint8_t &amp;value) const</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>is_set</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a5594067a17aa24a1e0d642b26b10b3be</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>invalidate</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aebcd10073f31aee3ed0bbd84b2526541</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_properties *</type>
      <name>props_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a7b815ca34873415ed3b4e6b02cc9f470</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>bool</type>
      <name>is_set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a02da787e86c4c5bcfbb4afdfc7804a3f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>getter_t</type>
      <name>get_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a9951039654216c93861a1687817afb6e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>setter_t</type>
      <name>set_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>aa3589206d8260d34501cc08933d0885c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>copy_t</type>
      <name>copy_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a3a7f1a3bf6b95062b649655335a7dab6</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend">
      <type>friend std::ostream &amp;</type>
      <name>operator&lt;&lt;</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1pvalue.html</anchorfile>
      <anchor>a826b900df7d5a5fda09c22e9f42fc5e8</anchor>
      <arglist>(std::ostream &amp;ostr, const pvalue&lt; uint8_t &gt; &amp;p)</arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>ras_inject_error</name>
    <filename>hello__events_8c.html</filename>
    <anchor>structras__inject__error</anchor>
  </compound>
  <compound kind="union">
    <name>ras_inject_error.__unnamed88__</name>
    <filename>hello__events_8c.html</filename>
    <anchor>unionras__inject__error_8____unnamed88____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>csr</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a1f8c50db95e9ead5645e32f8df5baa7b</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>ras_inject_error.__unnamed88__.__unnamed90__</name>
    <filename>hello__events_8c.html</filename>
    <anchor>structras__inject__error_8____unnamed88_____8____unnamed90____</anchor>
    <member kind="variable">
      <type>uint64_t</type>
      <name>catastrophicr_error</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a2a0e5bc9b5c01998865d21cfc6acdd8b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>fatal_error</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a248d7cd65b618004dbd5f44e96b8cd61</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>nonfatal_error</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>a8da0039b80fc69ebb6f5f263320816ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>uint64_t</type>
      <name>rsvd</name>
      <anchorfile>hello__events_8c.html</anchorfile>
      <anchor>aa799822e734135bf270e704b72c08764</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::reconf_error</name>
    <filename>classopae_1_1fpga_1_1types_1_1reconf__error.html</filename>
    <base>opae::fpga::types::except</base>
    <member kind="function">
      <type></type>
      <name>reconf_error</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1reconf__error.html</anchorfile>
      <anchor>a776d748a51668a5c94cc7ff13d8f155a</anchor>
      <arglist>(src_location loc) noexcept</arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::shared_buffer</name>
    <filename>classopae_1_1fpga_1_1types_1_1shared__buffer.html</filename>
    <member kind="typedef">
      <type>std::size_t</type>
      <name>size_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a958fbecd1d6456b8e8950daffb7aa3ae</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>std::shared_ptr&lt; shared_buffer &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>aa90c1193c9d6c97f256f7482529d2afe</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>shared_buffer</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a94af424818d524cb897b1bc6a13ba5f7</anchor>
      <arglist>(const shared_buffer &amp;)=delete</arglist>
    </member>
    <member kind="function">
      <type>shared_buffer &amp;</type>
      <name>operator=</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a0ec4bb470ad2b48722a14fcdce1db2f8</anchor>
      <arglist>(const shared_buffer &amp;)=delete</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~shared_buffer</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a0dda51993e7c2ad5ed8f2759cdb002a5</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>release</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>aeffe4f3dcd9bec506e322ece0e54dff7</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>volatile uint8_t *</type>
      <name>c_type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>ade3a3102e5162ba8bb793342bdbf58f2</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>handle::ptr_t</type>
      <name>owner</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>acf517c2237550557c7b5f6e16c9b74b9</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>size_t</type>
      <name>size</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a615559ac054e924fc9cf853ea625fb47</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>wsid</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>ae927b9d7e242873e6df1c831ed35f291</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>io_address</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>acbdabbc8d4ef32caa9c231a73c5a2761</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>fill</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a0df9843bfb38be926cfbc564e2e21f3d</anchor>
      <arglist>(int c)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>compare</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a2eddbc4e93b5afb0822b08ac905e4980</anchor>
      <arglist>(ptr_t other, size_t len) const</arglist>
    </member>
    <member kind="function">
      <type>T</type>
      <name>read</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>abc03852d5a07e5a5f30bf9086b35950b</anchor>
      <arglist>(size_t offset) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a49c8d0b03a0e47c8b58f84301ac34559</anchor>
      <arglist>(const T &amp;value, size_t offset)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static shared_buffer::ptr_t</type>
      <name>allocate</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a5a3761aa33084d9a4038e457903e189b</anchor>
      <arglist>(handle::ptr_t handle, size_t len, bool read_only=false)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static shared_buffer::ptr_t</type>
      <name>attach</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>aaf0b68c33a84e081652f203443aad788</anchor>
      <arglist>(handle::ptr_t handle, uint8_t *base, size_t len, bool read_only=false)</arglist>
    </member>
    <member kind="function" protection="protected">
      <type></type>
      <name>shared_buffer</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>af3459a34de90e2dc37b608925d82acdf</anchor>
      <arglist>(handle::ptr_t handle, size_t len, uint8_t *virt, uint64_t wsid, uint64_t io_address)</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>handle::ptr_t</type>
      <name>handle_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>a9e5f35cda6592f57fb45494a6ea7b416</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>size_t</type>
      <name>len_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>ace6962636a4abb52615ba899b5a1f0be</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint8_t *</type>
      <name>virt_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>aa8265521240b457d2a80814e1e6b8b20</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>wsid_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>aedeb7bdc69160d09413a93ab2710ef57</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>io_address_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1shared__buffer.html</anchorfile>
      <anchor>ad8d6169b66456e78a22a4575e5f50535</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::src_location</name>
    <filename>classopae_1_1fpga_1_1types_1_1src__location.html</filename>
    <member kind="function">
      <type></type>
      <name>src_location</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>aa873a93570e4652a4ca9e806d94fa7f6</anchor>
      <arglist>(const char *file, const char *fn, int line) noexcept</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>src_location</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a0df0d9a994b3c66491ac4ee85ba5cba5</anchor>
      <arglist>(const src_location &amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type>src_location &amp;</type>
      <name>operator=</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a51cbdd224603d8523c99ccb21f1908fa</anchor>
      <arglist>(const src_location &amp;other) noexcept</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>file</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a82f6ed23a0d1d947b0f1754ed066e45d</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type>const char *</type>
      <name>fn</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a85e8edcb82dd85fbbb53c327541bd667</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>line</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a9dfb5ebf31b205490604b1ee60359cff</anchor>
      <arglist>() const noexcept</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>const char *</type>
      <name>file_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a5f8a4ed4689e7dbafb4b6d78057c064c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>const char *</type>
      <name>fn_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>a1bb3647316406de3745e3f65af4108e6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>int</type>
      <name>line_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1src__location.html</anchorfile>
      <anchor>ae2b7d039ac7da1702409ee07028ab6cf</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::sysobject</name>
    <filename>classopae_1_1fpga_1_1types_1_1sysobject.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; sysobject &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a1a64124cb599b2944cd2cda0081e2653</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>sysobject</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a113c998356671d5e03d526b8c1c8e5aa</anchor>
      <arglist>()=delete</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>sysobject</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>ac687a5693b259d5e0390361225e999fc</anchor>
      <arglist>(const sysobject &amp;o)=delete</arglist>
    </member>
    <member kind="function">
      <type>sysobject &amp;</type>
      <name>operator=</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>adf8b59f15dbd3d4514e0397cf5133a07</anchor>
      <arglist>(const sysobject &amp;o)=delete</arglist>
    </member>
    <member kind="function">
      <type>sysobject::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a8722a8fdb2c7de416e8f47280d1c1888</anchor>
      <arglist>(const std::string &amp;name, int flags=0)</arglist>
    </member>
    <member kind="function">
      <type>sysobject::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>afdbd6396094b0c1c274fa64c18c88119</anchor>
      <arglist>(int index)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~sysobject</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a8f86033c1116397d837a2b6a92ddbbf4</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>uint32_t</type>
      <name>size</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>ae9374e6aa30648d2c364888bed55c71f</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>uint64_t</type>
      <name>read64</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a4120492ac81789a245d8384497a4d374</anchor>
      <arglist>(int flags=0) const</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write64</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a71b53fc73a644e490252f57e28939618</anchor>
      <arglist>(uint64_t value, int flags=0) const</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; uint8_t &gt;</type>
      <name>bytes</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a119935a32c66e6a6d3e7e700a3181863</anchor>
      <arglist>(int flags=0) const</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; uint8_t &gt;</type>
      <name>bytes</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a9e248a37ffa6f6f33fd50d5d81b7b90d</anchor>
      <arglist>(uint32_t offset, uint32_t size, int flags=0) const</arglist>
    </member>
    <member kind="function">
      <type>enum fpga_sysobject_type</type>
      <name>type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>af16b5a7266e18ac298884f72f4a941a3</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>fpga_object</type>
      <name>c_type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a13b45599ddc9e902bf6cfabf20e02768</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_object</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>ac1cb2fa7b43facfdbc70d5a033e394c1</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static sysobject::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>af82679125fec13b5731ab619808f2ccb</anchor>
      <arglist>(token::ptr_t t, const std::string &amp;name, int flags=0)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static sysobject::ptr_t</type>
      <name>get</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a91b4775d5e1d6520ce6fba54db990bb7</anchor>
      <arglist>(handle::ptr_t h, const std::string &amp;name, int flags=0)</arglist>
    </member>
    <member kind="function" protection="private">
      <type></type>
      <name>sysobject</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a152cbb0823356ecb633b295a2c45fd43</anchor>
      <arglist>(fpga_object sysobj, token::ptr_t token, handle::ptr_t hnd)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_object</type>
      <name>sysobject_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a2982d55edea60fcf80245da9592c3abf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>token::ptr_t</type>
      <name>token_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>a9b4075f8e189e37c5992f724dc7380b4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>handle::ptr_t</type>
      <name>handle_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1sysobject.html</anchorfile>
      <anchor>acb5d249e9d682ef0f228a0296b7fe851</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>mem_tg::tg_test</name>
    <filename>classmem__tg_1_1tg__test.html</filename>
    <base>opae::afu_test::command</base>
    <member kind="function">
      <type></type>
      <name>tg_test</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>abc01f94c6c8099d637420bdf98e1195a</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual</type>
      <name>~tg_test</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a3a14f5221622cd3cc598ed1518536645</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>name</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>ae2be4babdc05e96ad0fd49044206ae07</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>description</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a76d61f9943cf4224affb583f06eb1ef4</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual const char *</type>
      <name>afu_id</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a8ae23413206e40e0aec6f51db2a03173</anchor>
      <arglist>() const override</arglist>
    </member>
    <member kind="function">
      <type>double</type>
      <name>bw_calc</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a20aca3e96e317412ade0812dbb9e3d2d</anchor>
      <arglist>(uint64_t xfer_bytes, uint64_t num_ticks)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>tg_perf</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a56b732f00be2546a98773de00c97f87a</anchor>
      <arglist>(mem_tg *tg_exe_)</arglist>
    </member>
    <member kind="function">
      <type>bool</type>
      <name>tg_wait_test_completion</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a60c04995edf8449efbb5d76d76bd5871</anchor>
      <arglist>(mem_tg *tg_exe_)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>config_input_options</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>aa9a2ef74bb44575e90abd161773337ee</anchor>
      <arglist>(mem_tg *tg_exe_)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>run_mem_test</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>aa4a52fa4a7e268e6911172f12611816d</anchor>
      <arglist>(mem_tg *tg_exe_)</arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>run_thread_single_channel</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a97d534705750d044486b471664f019df</anchor>
      <arglist>(mem_tg *tg_exe_)</arglist>
    </member>
    <member kind="function" virtualness="virtual">
      <type>virtual int</type>
      <name>run</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a25caf2aba2c64d4c528724e836dd5ff1</anchor>
      <arglist>(test_afu *afu, CLI::App *app) override</arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>uint64_t</type>
      <name>tg_offset_</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a49c1d34f23394b8d3f417941c3dae13e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>mem_tg *</type>
      <name>tg_exe_</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>af9a56c0724544be2cfeee914e300b264</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="protected">
      <type>token::ptr_t</type>
      <name>token_</name>
      <anchorfile>classmem__tg_1_1tg__test.html</anchorfile>
      <anchor>a19e277d2e24c74242d4be7c3d9b39689</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>threshold</name>
    <filename>types_8h.html</filename>
    <anchor>structthreshold</anchor>
    <member kind="variable">
      <type>char</type>
      <name>threshold_name</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a3b1e319947fc05dfb36ded4fb08940b3</anchor>
      <arglist>[256]</arglist>
    </member>
    <member kind="variable">
      <type>uint32_t</type>
      <name>is_valid</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a65fef448c835c04ba240e1fed004192e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>double</type>
      <name>value</name>
      <anchorfile>types_8h.html</anchorfile>
      <anchor>a1af0ffe5e1a7dd82234952db9ab1bc40</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::token</name>
    <filename>classopae_1_1fpga_1_1types_1_1token.html</filename>
    <member kind="typedef">
      <type>std::shared_ptr&lt; token &gt;</type>
      <name>ptr_t</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>adbc72f7f89d7accf08719ffeb4a11fe3</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>~token</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>aafe7dcd035500e15ff2ed5b77a9900a0</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function">
      <type>fpga_token</type>
      <name>c_type</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>a3fd01e2af2080651b6d632110e2dca11</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_token</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>afd697893ffe9f7b8fd580325dacfa4c1</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function">
      <type>ptr_t</type>
      <name>get_parent</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>a4d25e93b841487e643559bbe801107e5</anchor>
      <arglist>() const</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::vector&lt; token::ptr_t &gt;</type>
      <name>enumerate</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>a6f717fbec87c99a80ce4b94ce262923f</anchor>
      <arglist>(const std::vector&lt; properties::ptr_t &gt; &amp;props)</arglist>
    </member>
    <member kind="function" protection="private">
      <type></type>
      <name>token</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>a2b74dab293c25acd8d8cb4694104e659</anchor>
      <arglist>(fpga_token tok)</arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_token</type>
      <name>token_</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>a006b4a4ace42dd5da5be9efdaaf0496d</anchor>
      <arglist></arglist>
    </member>
    <member kind="friend" protection="private">
      <type>friend class</type>
      <name>handle</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1token.html</anchorfile>
      <anchor>ad5131fca8411651b7eaed9ca2b4a6fef</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>token_group</name>
    <filename>object__api_8c.html</filename>
    <anchor>structtoken__group</anchor>
    <member kind="variable">
      <type>fpga_token</type>
      <name>token</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>aff8b9ebac32292d61a68cdf373abcd07</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>metric_group *</type>
      <name>groups</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>acf0c62f0bafada1b09873fae376f9c83</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>size_t</type>
      <name>count</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>a4d21c3b21093ffeb2d94e795ab5e6cde</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>fpga_object</type>
      <name>clock</name>
      <anchorfile>object__api_8c.html</anchorfile>
      <anchor>ad1a9c073ec875ad3619da1436c9beccd</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="struct">
    <name>opae::fpga::types::event::type_t</name>
    <filename>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</filename>
    <member kind="function">
      <type></type>
      <name>type_t</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</anchorfile>
      <anchor>aac601cf4f78c04b0ade8ee231d4f2639</anchor>
      <arglist>(fpga_event_type c_type)</arglist>
    </member>
    <member kind="function">
      <type></type>
      <name>operator fpga_event_type</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</anchorfile>
      <anchor>ad4b33eb9683bc28c4ce91c3935721776</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static constexpr fpga_event_type</type>
      <name>interrupt</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</anchorfile>
      <anchor>a03e7db8472bf2f66c1fb44243089fe91</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static constexpr fpga_event_type</type>
      <name>error</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</anchorfile>
      <anchor>aee47a71c7fca3f1610fc033e92688d80</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static constexpr fpga_event_type</type>
      <name>power_thermal</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</anchorfile>
      <anchor>aa0be75f7d747013ef24922bb5055c17a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" protection="private">
      <type>fpga_event_type</type>
      <name>type_</name>
      <anchorfile>structopae_1_1fpga_1_1types_1_1event_1_1type__t.html</anchorfile>
      <anchor>ac9493c510e51fef60ad0b35500299ee8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="class">
    <name>opae::fpga::types::version</name>
    <filename>classopae_1_1fpga_1_1types_1_1version.html</filename>
    <member kind="function" static="yes">
      <type>static fpga_version</type>
      <name>as_struct</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1version.html</anchorfile>
      <anchor>a7e528dea61cf0bd424848ba091aae300</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::string</type>
      <name>as_string</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1version.html</anchorfile>
      <anchor>aa5683b008ba67e352c4ccc08a310b8bd</anchor>
      <arglist>()</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static std::string</type>
      <name>build</name>
      <anchorfile>classopae_1_1fpga_1_1types_1_1version.html</anchorfile>
      <anchor>acf36384ad94dfdd0c13c7595b37aeccf</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>cxl_mem_tg</name>
    <filename>namespacecxl__mem__tg.html</filename>
    <class kind="union">cxl_mem_tg::mem_tg_ctl</class>
    <class kind="union">cxl_mem_tg::mem_tg_status</class>
    <class kind="union">cxl_mem_tg::mem_tg0_count</class>
    <class kind="union">cxl_mem_tg::mem_tg1_count</class>
    <class kind="class">cxl_mem_tg::cxl_mem_tg</class>
    <class kind="class">cxl_mem_tg::cxl_tg_test</class>
    <class kind="struct">cxl_mem_tg::mem_tg_ctl.__unnamed52__</class>
    <class kind="struct">cxl_mem_tg::mem_tg_status.__unnamed55__</class>
    <class kind="struct">cxl_mem_tg::mem_tg0_count.__unnamed58__</class>
    <class kind="struct">cxl_mem_tg::mem_tg1_count.__unnamed61__</class>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae21fb3ab55ae8cb59106b87200d95a7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ab54b53736c1e8bd7876a69f2baef17b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ACTIVE</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3a7cbd86dff0180f632d573ef7e5a55057</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_TIMEOUT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3a8e63adddabc288e5498f2f4df66f0b27</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ERROR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3ad6d2ca96385ead5241bb420e1e7ae64e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_PASS</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a9346c08ca8d6da1adde540395a909db3aa0b11c71d367595e2a12c20197b44ce9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2ae50331c964539f5dd5c26007735d4ed3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_SEQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2ab7e300ecb7930052b948a653ddcaa184</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND_SEQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2af669ed7de6b50d6000674e9427d25182</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_ONE_HOT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa4af1bf553a1eaba8cd8f49e614476e2a92b5675ef5ee0396ead56070a486e58e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_FIXED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa7407f4233717e1d98f8a776390eb82af</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS7</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa657e027c0dafddd6b1507f6cce1194ad</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS15</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa17eadd59787408f22df400c34c6a1b0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS31</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaae5e9b7fe639f5d85614b3a1759b7c659</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_ROTATING</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>aa1d50ccef06b3cce729d5a785c2cfdeaa71564d8355c494d5a258cb5c26b4fc98</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea01597032eab3f760346c82fbc788ec5c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eae1b3a54124834078d9db7680c3caa7bd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea5debc24069234094ad6ba398106c65fb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NEXT_AFU</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eaaa35301298cf8446efb61505e0798fa0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH_RSVD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea0664bd36175dd3d76875ebaa52980af6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SCRATCHPAD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea762eb285e7fc88be1e742b50533c5f9a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CTRL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea4756bce556cb643e0aa2c409f21f0c37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_STAT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eadd994b8040392db4076101038b417fe4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CLK_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea88d8b198bfb44c9dcad46aa0ab207970</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_WR_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09eaf6d9bbd396589527f372a7b48f1a4d92</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CLK_FREQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a8ba6380a3d80e0f4b6ec8ef2529ae09ea0fd9d1b39b6d59014ef01787ec3a4eef</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_VERSION</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a21e0b7931a80741eb0c73747a3fe4377</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_START</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9afbeb224f5b1d32e9b55b1ce8fcc9e32c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_LOOP_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad02dea59ee54f68764340c1c233829b1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aab71456eea9b0ccecabf1e0891954a0e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a411d376733b772443344588a2cac4458</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_REPEAT_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aa30e3bd5cec3f3cd992e6121bdbab09e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_REPEAT_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a90a29710d3b2099c1795a61d2622076e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURST_LENGTH</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab821353518367c2fcd2ab57a9c6e7840</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_CLEAR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a88ec11b1fb1a062f02451c50b0016949</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_IDLE_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab64e524dd316d881cc22757087d33439</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_LOOP_IDLE_COUNT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aed6f3dfb644b6e070fefcaf16e2e1d11</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_WR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ae286a4c714c99b08b1002ee3ed1a587f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_WR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a7709164257a8f5718596b5ab2c472ad4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RETURN_TO_START_ADDR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad39fc44c8edf2e15dad6d831aa6fe819</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_ADDR_INCR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9acf979f868802b88029d479d18111f465</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_RD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a201705870a4fa204363246058cb2076e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_RD</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a9ce33fc8beaf69c76c4ce3776805d1c4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PASS</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9af157488b545676cc548397859997b02a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a0b00917713940ec859e81f0a86dc562f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a6c4409c3d1f07c62bcd75fd3b05467fe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a1382e3f82472ef4eb8dcbbb17e377285</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9afaf1344c0006254f6f4947ed9728bee6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aa10c574348ce256ce6987b453ab10f70</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a810379cbef2d3d1791fb853c8a7d3792</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab3188e1e50ec3ed8a6eed8dafd7e3bfe</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_COMPLETE</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a397a7f61b4c32a69262d9db33ec35fda</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_INVERT_BYTEEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad69950385bd5a8260e9262d3473d65e9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RESTART_DEFAULT_TRAFFIC</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab7a46ab46b32d143ea74bd68d6e1708c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_USER_WORM_EN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a138084b5a721464cfb312d4d675410bf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_BYTEEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a50c1a9359d982b1f90a076434a82062b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TIMEOUT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9acd7be04dcb9016fd18b247be44487e93</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_DATA_GEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aad2624687ce89631f8307ceb034b86e8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_BYTEEN_GEN</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a6d207aa57db45407a6d02d79c5a0f7e3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RDATA_WIDTH</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a8f351711d6e1137882a8248bb4536fb8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ERROR_REPORT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab50773a7b547928a51e3ebb30a00b0fa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PNF</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a75cfc6f2b701f77e83465a606133b241</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_EXPECTED_DATA</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a0452589b12a20397977344288abae82f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_READ_DATA</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a26ef0000b2f002b1fe653aeb6d2677ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_SEED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ad19fda5e987a2fae893421a426f6766d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9ab95729a6c6553c58502a0f1306f1b004</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PPPG_SEL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9adf412af7a4ff3287fe327afee84e348d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a3c528143f8a6d19ca435a0f458cc29b8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_FIELD_RELATIVE_FREQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aece950f87a9fd52ec05f2326d00499fd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_FIELD_MSB_INDEX</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9aeb51b5206be439a4f73216c8737b15f2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURSTLENGTH_OVERFLOW_OCCURRED</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a4373da92590dd537b2f82603b269fd6f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURSTLENGTH_FAIL_ADDR_L</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a8382e8133ee151e0c00c1993011deeaa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURSTLENGTH_FAIL_ADDR_H</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a0e3799ef4889e3dae68d80346fee6d61</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WORM_MODE_TARGETTED_DATA</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a001f2f66547e050fa059f9843621d0f9a533db7920ef8344cb657bc340e903227</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>AFU_ID</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a7187f3c940e7b0bede33c18321170c26</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MEM_TG_TEST_TIMEOUT</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a7488ec5ac28f6679e818452cb7b347ec</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TEST_SLEEP_INVL</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ae2fec4e55f5f8fbfb78d0c38ec1e3289</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TG_CTRL_CLEAR</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a7e452dc886e26cc1525c4533efbfd1c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TG_SLEEP</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>adf88f83a1ea74efb4228b2e7de8d8ae8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TG_FREQ</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>ad72ec47242a649d327e0bde858f7f2b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>tg_pattern</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>a4adbfbc2d17a8a0c2a818f243385336a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const int</type>
      <name>MEM_TG_CFG_OFFSET</name>
      <anchorfile>namespacecxl__mem__tg.html</anchorfile>
      <anchor>afb32111a410e98fdae3118bc9a2c9277</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>dummy_afu</name>
    <filename>namespacedummy__afu.html</filename>
    <class kind="class">dummy_afu::ddr_test</class>
    <class kind="union">dummy_afu::afu_dfh</class>
    <class kind="union">dummy_afu::mem_test_ctrl</class>
    <class kind="union">dummy_afu::ddr_test_ctrl</class>
    <class kind="union">dummy_afu::ddr_test_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank0_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank1_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank2_stat</class>
    <class kind="union">dummy_afu::ddr_test_bank3_stat</class>
    <class kind="class">dummy_afu::dummy_afu</class>
    <class kind="class">dummy_afu::lpbk_test</class>
    <class kind="class">dummy_afu::mmio_test</class>
    <class kind="struct">dummy_afu::afu_dfh.__unnamed65__</class>
    <class kind="struct">dummy_afu::mem_test_ctrl.__unnamed68__</class>
    <class kind="struct">dummy_afu::ddr_test_ctrl.__unnamed71__</class>
    <class kind="struct">dummy_afu::ddr_test_stat.__unnamed74__</class>
    <class kind="struct">dummy_afu::ddr_test_bank0_stat.__unnamed77__</class>
    <class kind="struct">dummy_afu::ddr_test_bank1_stat.__unnamed80__</class>
    <class kind="struct">dummy_afu::ddr_test_bank2_stat.__unnamed83__</class>
    <class kind="struct">dummy_afu::ddr_test_bank3_stat.__unnamed86__</class>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ac06b3900bd7a2d4ba2c3f19ea20f622d</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>aa01df4dcd37386cbf0800129c6f3183c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba2e87f72819846d7a53092b360755c7cd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_L</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba7bc0113085a1537b87b9533c1d1b93e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_H</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046baa48ec3d788fa335ebc4bb4469bafc48c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NEXT_AFU</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba222f7920be0e34b7907643cef6889fd4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH_RSVD</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba1a9e79d25022f4af20b0fb5347f7a76e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SCRATCHPAD</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bac7cab4d551fa9a04a732ed2d48f121a0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MMIO_TEST_SCRATCHPAD</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba2dd1000f0770dcda833f518caaef2139</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_CTRL</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba33b12307658c4d48d2e3e830f31c8d4a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bae4462a80036a5ac3aa3cada2418bc2a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_SRC_ADDR</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba50907b523a57b88d331c2224e057f8dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TEST_DST_ADDR</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba23997995c982f0e6a26fea04efa07c37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_CTRL</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046baf36a9e642b0ddac03c1f9938fb6789c3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046baab48d50e6990ff1a2a76fe12bd189a51</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK0_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bada2d999413f409ffcf8f73006412f96f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK1_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba47ea5964b0401582a714a12a42998c83</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK2_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046ba0edc832f3a1cb6b2be9e63b044a1570a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>DDR_TEST_BANK3_STAT</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ae3ac4ffc32c20cde26b9d2090105046bac7b6c66db9078f216f384bcdc09ae8d5</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>int</type>
      <name>check_stat</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>aef855a00ce0a89e5c9d10373084438ad</anchor>
      <arglist>(std::shared_ptr&lt; spdlog::logger &gt; logger, const char *name, T stat)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>timeit_wr</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a2331454a35bd7d061543b78f7708db09</anchor>
      <arglist>(std::shared_ptr&lt; spdlog::logger &gt; log, dummy_afu *afu, uint32_t count)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>timeit_rd</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ab7da4ff6c8571efbc707ba26cfba6723</anchor>
      <arglist>(std::shared_ptr&lt; spdlog::logger &gt; log, dummy_afu *afu, uint32_t count)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_verify</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>ab2db23ee00af6f322f726f9017f60fdf</anchor>
      <arglist>(dummy_afu *afu, uint32_t addr, T value)</arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>write_verify</name>
      <anchorfile>namespacedummy__afu.html</anchorfile>
      <anchor>a8aa59ff4d5d11ffe262b3bcc1524046c</anchor>
      <arglist>(dummy_afu *afu, uint32_t addr, uint32_t i, uint64_t value)</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>host_exerciser</name>
    <filename>namespacehost__exerciser.html</filename>
    <class kind="class">host_exerciser::he_cache_cmd</class>
    <class kind="class">host_exerciser::he_cache_lpbk_cmd</class>
    <class kind="class">host_exerciser::he_cmd</class>
    <class kind="union">host_exerciser::he_dfh</class>
    <class kind="union">host_exerciser::he_dsm_base</class>
    <class kind="union">host_exerciser::he_ctl</class>
    <class kind="union">host_exerciser::he_info</class>
    <class kind="union">host_exerciser::he_wr_num_lines</class>
    <class kind="union">host_exerciser::he_wr_byte_enable</class>
    <class kind="union">host_exerciser::he_wr_config</class>
    <class kind="union">host_exerciser::he_wr_addr_table_ctrl</class>
    <class kind="union">host_exerciser::he_wr_addr_table_data</class>
    <class kind="union">host_exerciser::he_rd_num_lines</class>
    <class kind="union">host_exerciser::he_rd_config</class>
    <class kind="union">host_exerciser::he_rd_addr_table_ctrl</class>
    <class kind="union">host_exerciser::he_rd_addr_table_data</class>
    <class kind="union">host_exerciser::he_err_status</class>
    <class kind="struct">host_exerciser::he_cache_dsm_status</class>
    <class kind="class">host_exerciser::host_exerciser</class>
    <class kind="union">host_exerciser::he_dsm_basel</class>
    <class kind="union">host_exerciser::he_dsm_baseh</class>
    <class kind="union">host_exerciser::he_num_lines</class>
    <class kind="union">host_exerciser::he_cfg</class>
    <class kind="union">host_exerciser::he_inact_thresh</class>
    <class kind="union">host_exerciser::he_interrupt0</class>
    <class kind="union">host_exerciser::he_swtest_msg</class>
    <class kind="union">host_exerciser::he_status0</class>
    <class kind="union">host_exerciser::he_status1</class>
    <class kind="union">host_exerciser::he_error</class>
    <class kind="union">host_exerciser::he_stride</class>
    <class kind="struct">host_exerciser::he_dsm_status</class>
    <class kind="struct">host_exerciser::MapKeyComparator</class>
    <class kind="class">host_exerciser::host_exerciser_cmd</class>
    <class kind="class">host_exerciser::host_exerciser_lpbk</class>
    <class kind="class">host_exerciser::host_exerciser_mem</class>
    <class kind="struct">host_exerciser::he_dfh.__unnamed3__</class>
    <class kind="struct">host_exerciser::he_dfh.__unnamed94__</class>
    <class kind="struct">host_exerciser::he_dsm_base.__unnamed6__</class>
    <class kind="struct">host_exerciser::he_ctl.__unnamed9__</class>
    <class kind="struct">host_exerciser::he_ctl.__unnamed106__</class>
    <class kind="struct">host_exerciser::he_info.__unnamed12__</class>
    <class kind="struct">host_exerciser::he_wr_num_lines.__unnamed15__</class>
    <class kind="struct">host_exerciser::he_wr_byte_enable.__unnamed18__</class>
    <class kind="struct">host_exerciser::he_wr_config.__unnamed21__</class>
    <class kind="struct">host_exerciser::he_wr_addr_table_ctrl.__unnamed24__</class>
    <class kind="struct">host_exerciser::he_wr_addr_table_data.__unnamed27__</class>
    <class kind="struct">host_exerciser::he_rd_num_lines.__unnamed30__</class>
    <class kind="struct">host_exerciser::he_rd_config.__unnamed33__</class>
    <class kind="struct">host_exerciser::he_rd_addr_table_ctrl.__unnamed36__</class>
    <class kind="struct">host_exerciser::he_rd_addr_table_data.__unnamed39__</class>
    <class kind="struct">host_exerciser::he_err_status.__unnamed42__</class>
    <class kind="struct">host_exerciser::he_dsm_basel.__unnamed97__</class>
    <class kind="struct">host_exerciser::he_dsm_baseh.__unnamed100__</class>
    <class kind="struct">host_exerciser::he_num_lines.__unnamed103__</class>
    <class kind="struct">host_exerciser::he_cfg.__unnamed109__</class>
    <class kind="struct">host_exerciser::he_inact_thresh.__unnamed112__</class>
    <class kind="struct">host_exerciser::he_interrupt0.__unnamed115__</class>
    <class kind="struct">host_exerciser::he_swtest_msg.__unnamed118__</class>
    <class kind="struct">host_exerciser::he_status0.__unnamed121__</class>
    <class kind="struct">host_exerciser::he_status1.__unnamed124__</class>
    <class kind="struct">host_exerciser::he_error.__unnamed127__</class>
    <class kind="struct">host_exerciser::he_stride.__unnamed130__</class>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3396b0a2f5f048347375ed7968bf936f</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9047898033282bdb49d6657d7532fe2f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa96f5f9ad616abbff98f88fd8d3749eec</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_L</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa994517cdf13de8a08a6fbcebc5d4b7eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_H</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa89326af32d3ac7eb75a59785c9ff1ee3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0547c29b0609234864bd196f04e9e101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaf9a43fa5b1790303e01e463acba370a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa712f6bdeef038f12b2943223e36a3702</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DSM_BASE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a9940ff171541be267a8cf74277844b32</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CTL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aabd5379c2a756d7f71b6707abdfc88089</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INFO</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a63ab354d6a895d75d3ea2681c4a73c85</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_NUM_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a3b1b6c8c2fcbf57ad624de1077356fd4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_BYTE_ENABLE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248ab8930a0aeafce2a45089f931c333f205</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_CONFIG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248ab29c859275463170cc2bbb6e2fb0297c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_ADDR_TABLE_CTRL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a1adff58b128d7f57593d15ed67a54fc6</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_WR_ADDR_TABLE_DATA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a6d77f005b2d7e53be9d91ef0593eadda</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_NUM_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a344481e3ac1122c5e74bb59a7a934409</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_CONFIG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a8871c897146f0f2eaccd5077f6181472</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_ADDR_TABLE_CTRL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a321b37ff8fc5472a16b46da5631a6357</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_RD_ADDR_TABLE_DATA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a300dc5ac788c901d20f421a24af98d7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_STATUS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248aca954c7b87a79cfff2d4d6f6c8e23eb1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_EXP_DATA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a2845aa2671b2f9a6235667aa6bc76ce0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a76ad387e8d2fb4639563c8e0272aa28c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a8cbbda04e37bbff5290a1ac024909baa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a2daca8866beee6f107128bb3bb23d2cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA3</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248ad8040e2bd695023243cd93bc540b1ae9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a872972b8d10fc0f8c38eb8bffc6a9537</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA5</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248aac19634eecfe2a67e232d483d5b07f7d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA6</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a237210f2c6acd07b79645c8196c00d7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR_ACT_DATA7</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa650b87101d59c62f5aeee6f8e767248a8253c945a94fbb2dd2a45ae100ae6683</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_rd_opcode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>RD_LINE_I</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983a8615bca5d3aa6d3f844bd4f9c5f07e2e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>RD_LINE_S</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983a383b82e47fbec58b8a1b328d3be3ab35</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>RD_LINE_EM</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5bd22f57a22dd60a9586ce15bbf7f983a19aaf26804a3b10760a8347133f35937</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_wr_opcode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34df</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_LINE_I</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa9f9b0eb93b101bd67e617acc0e38a671</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_LINE_M</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfae5bc76f014dcbb30bf9377d595d738c7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_PUSH_I</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa56f244dab07fac79a257306c87d88468</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_BARRIER_FRNCE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa8ed760c5fa905aaf07983d90158f4287</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_FLUSH_CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa5b357c8754747c7c1e90b2accb3c656a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_FLUSH_CL_HCOH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfa28048131a444e8a31d7eaf6fbe710730</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>WR_FLUSH_CL_DCOH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af1c4e1cf2fb828e63fa13fab9b8d34dfad0dd56b280f7707ad776a5bcb48ff068</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_test_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_RD_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ad211a335379d690f23f9561bdd1e4aea</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_WR_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ae25066dd8d70eddfe8b4fb3f9771c92a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_RD_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979afc8a66080da962c0f662ff19ec429a0b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_FPGA_WR_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ac7679022e0d491948f9439be39524836</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_RD_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979a237b3d2bbc5e51b408936c8879e0ef8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_WR_CACHE_HIT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979a67f17167a9df28c18e6125573d488bfa</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_RD_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ac1f7a621fcee208489af401304507857</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_HOST_WR_CACHE_MISS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa80f63fac1f69c78c803dc78b3e31979ab048697a1ec0a6ef71078d01dab304ee</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_target</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7e3f43915a1b25c35bb95863a87b1429</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_TARGET_HOST</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7e3f43915a1b25c35bb95863a87b1429a7f8712a5623abfd073edf0859128ab8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_TARGET_FPGA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7e3f43915a1b25c35bb95863a87b1429a1253eee565e44c426c6a34eadfaa0821</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_cxl_latency</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_LATENCY_NONE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4a92dbb5ac604c6744a28687c41d70d2c2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_RD_LATENCY</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4ad7b91477904acff1abcaba47619bf1a5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_WR_LATENCY</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4a12ca9c21fbca0f9853b3e22b8578faaf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_RD_WR_LATENCY</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37ffa3c2ac30b63e2b03cc8f6e52c4a4a2553554934734ab40d12a7b9d72f025c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_bisa_support</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTMEM_BIAS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15aa4d06aaac0f2a27387b5fc9461a75ca3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_BIAS_NA</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15a23ba95206a39722ed39c434ba4058846</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGAMEM_HOST_BIAS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15ad7cf526a75d59ecc712f3208c999a816</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>FPGAMEM_DEVICE_BIAS</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a6bb2d96272ec06618f0a61a7d0699c15a53f78272c191221a7b8d94466d9ee76a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_cxl_dev</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9fb8d62824f4c57e9f79549c21b80592</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_DEVICE0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9fb8d62824f4c57e9f79549c21b80592adab5ca75a104003678a8720b1c8ad694</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CXL_DEVICE1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9fb8d62824f4c57e9f79549c21b80592a8c25ad936358399012804259bdf95788</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_addrtable_size</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE4096</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255ae12fcf27fe2cbc28ad429f8b1d469410</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE2048</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a8714dd9f55a88e0d1c8c5ab0822bb1fc</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE1024</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a6d094ba063a90590e5e1e753ef740f8c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE512</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a1ceb659f01e34a669743bd97fd793507</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE256</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a96f0b218e77aeaf8c7e78f18a050793d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE128</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a1fa2286f9d42bde78410951d6e70cb4f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE64</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a06c7e799fd21e744e4a83d6741559a0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE32</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a7956f4187e400ed80b82ccac3de88acb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE16</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255aeb0ac8e8b22329ca7c4b25278dd6af95</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255a7f88fa903cae0f7c61e174de96b25073</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255ab943ac256680c233c1b70966c3d2077d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ADDRTABLE_SIZE2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aef8f7bf36f75de53c23f271a69d18255ae665c4520f174df79c45d4d5fb9c5d2b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>he_traffic_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a619d5ce8b001ff40b27522d1be17bc7e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ENABLE_TRAFFIC_STAGE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a619d5ce8b001ff40b27522d1be17bc7ead216c542fdaa45ed1cc9e1c71c8df72b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SIP_SEQ_STAGE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a619d5ce8b001ff40b27522d1be17bc7eacbe77aa1bda4e8827b71c6bf5beedf4d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa96f5f9ad616abbff98f88fd8d3749eec</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_L</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa994517cdf13de8a08a6fbcebc5d4b7eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ID_H</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa89326af32d3ac7eb75a59785c9ff1ee3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0547c29b0609234864bd196f04e9e101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DFH_RSVD1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaf9a43fa5b1790303e01e463acba370a8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa712f6bdeef038f12b2943223e36a3702</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aadacb6d1d03994d938c33005ab3eef726</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SCRATCHPAD2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa33453ac7a61a0e1b3ede18bff3cef947</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DSM_BASEL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0f0436ad7cb26fd22fbdf4fd3264f309</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DSM_BASEH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa0351bf59f34601c8d81894f0897e6cde</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SRC_ADDR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa7e7c47794d23f20fef4489d3b71c0122</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_DST_ADDR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa14be24d91af1512e70b6ca78c7694101</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_NUM_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aab5e8a172abefb6c61695f517de783395</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CTL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aabd5379c2a756d7f71b6707abdfc88089</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_CFG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaf48cdfa6534904b034ff9641dab6b34f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INACT_THRESH</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa12797e3cf3c370e7151ec1b257ad82e5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INTERRUPT0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aaaf7d586e7208f7ea7b598601d02e3d0e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_SWTEST_MSG</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa04279c71f1e9c5a535cfe8515bb52f12</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_STATUS0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa196cabd4f8f2a096cf6312f2c719d409</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_STATUS1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa5bd3e4e22deae78be590cf3c51949174</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_ERROR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aad24b45a44d85621225d9f8215457623f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_STRIDE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa878952b31118fec2cb65f1f0d039d8f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HE_INFO0</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc876826cec1f18e5dd446b0556f409aa2f434e10b654a82eaad847a090e164ef</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>host_exe_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbca</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_LPBK1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaa6631ee58c265dc79c90c5c2b526b9192</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_READ</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaa1ea63bb1d444f6bed6a46303b90ab7be</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_WRITE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaab65e84c23fdb71a2b385ce3593b78b04</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOST_EXEMODE_TRPUT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af465fb34b1359b3f4f82b8393a21fbcaa7a915772549100ded4d3a36df386fbe3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_req_len</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_1</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0aab7592a31c4b72cf471ea23e0d8367d9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_2</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0ace806856e806e5cc8b9c78b91a328d4c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0a08cc11d8a56708a49ca49a13932cd698</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0aff641659fe8c6a51abd942fae0ccd2e2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_CLS_16</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab6410838913a9fb5e26f2d0291cc3fa0affdb50fd72b4b6b60d754f421f7f35f7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_atomic_func</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_OFF</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84aca03793e71244ba100cbbb44e4523979</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_FADD_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84a9e355a7bceadb9f9ffb0786d26670926</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_FADD_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84af631fb94c0d4adf7d6f3567d3293c3e4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_SWAP_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84a4e635662561727a4d1f8903658862503</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_SWAP_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84a9af1bd83b6aca61b4422036cbef2da34</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_CAS_4</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84abb08f60e74724813b14692184afec582</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ATOMIC_CAS_8</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4e377e3c6ebfeb2a97c78584d8e75c84aafb548060eed16af2fd9c9ad87b09e01</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_encoding</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_DEFAULT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456a043d79b53e107d6653db3a845fb66556</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_DM</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456a1d5ec8224b5922cee68164f7e7fbb401</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_PU</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456afff5d203a4ffba51d8d78231ba8c4884</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_ENCODING_RANDOM</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae17003a3398959b9d3f26abdc9c84456a05f5799651cb2b0d71bca3615e22802d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumeration">
      <type></type>
      <name>hostexe_test_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abe4c0db8055b8c2abb6cd96f72bc1890</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_TEST_ROLLOVER</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abe4c0db8055b8c2abb6cd96f72bc1890a3e8ad9fc752d1b50e0f0725b87c410b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>HOSTEXE_TEST_TERMINATION</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>abe4c0db8055b8c2abb6cd96f72bc1890a90f94357ec7daa6c0d2a473deded4a0a</anchor>
      <arglist></arglist>
    </member>
    <member kind="function">
      <type>void</type>
      <name>he_cache_thread</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a63cc59095cdb804e9d969dbd8445b35d</anchor>
      <arglist>(uint8_t *buf_ptr, uint64_t len)</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_TIMEOUT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4d5f99e45d6527c0a2e376d00329612a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_SLEEP_INVL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37f4cd6ba67fd8899f4218bba01b2b96</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3c15318d7b073d9b79b270aab66d37a3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>KB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4f74c9687586e00027a417a41a34d05b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae3548651828dac1b7e2adec934493f98</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>BUFFER_SIZE_2MB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a61b58598a908fe8bbf0db3dcfc8b706f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>BUFFER_SIZE_32KB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a0a736cbba6907deb25688e3082084e56</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>FPGA_32KB_CACHE_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>af766e4d826b22e69e7b7ea58d14989dd</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>FPGA_2MB_CACHE_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a32fc2a41fc6262bcfac663c1e72547d7</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>FPGA_512CACHE_LINES</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5c57d1dc4cb0cc9df9983da2498ceb0d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const double</type>
      <name>LATENCY_FACTOR</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7181c0e391f834ad9cdda3e578176b3a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_test_modes</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9236a519bfc92f2e8a8cf399433bf29b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_targets</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aa029adb74bb308968e3ab7ce3036610d</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_bias</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a06c6876e392a9a89941d649a7162f8c9</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_cxl_device</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7d0aa3fa8a77d912617648e824ab251f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>traffic_enable</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ace4c7dc1522d8416d8c6162641bc8083</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::map&lt; uint32_t, uint32_t &gt;</type>
      <name>addrtable_size</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a55022611ecb6aba94c51e3620f980b7f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_TIMEOUT</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4d5f99e45d6527c0a2e376d00329612a</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>HELPBK_TEST_SLEEP_INVL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a37f4cd6ba67fd8899f4218bba01b2b96</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3c15318d7b073d9b79b270aab66d37a3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>KB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4f74c9687586e00027a417a41a34d05b</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MB</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ae3548651828dac1b7e2adec934493f98</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>LOG2_CL</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>aaf2fdab399a1e5df1decd13aaff46648</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const size_t</type>
      <name>LPBK1_DSM_SIZE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>afc614c0a041ef93b39806f91153b822f</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const size_t</type>
      <name>LPBK1_BUFFER_SIZE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>ab4b46ce5277ad6a137e9d753cc54f7c8</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const size_t</type>
      <name>LPBK1_BUFFER_ALLOCATION_SIZE</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4a48249a14d845c7ad6c2913209cbfe5</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_modes</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a9dc7a132365a9a2e883166de316d3579</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t, MapKeyComparator &gt;</type>
      <name>he_req_cls_len</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a2ea0bead4a89cf726adea06214201dc3</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_req_atomic_func</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a3384e6ab4680f2301d6a9db29c196edf</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_req_encoding</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a5e3a5f37a59e69f383e1d72cae05e9f4</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>he_test_mode</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a7891643f1c1433756f12022500d93dff</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>interleave_help</name>
      <anchorfile>namespacehost__exerciser.html</anchorfile>
      <anchor>a4ca77ad5a2b9a9762842871600fbf1d8</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>mem_tg</name>
    <filename>namespacemem__tg.html</filename>
    <class kind="class">mem_tg::mem_tg</class>
    <class kind="class">mem_tg::tg_test</class>
    <member kind="typedef">
      <type>opae::afu_test::afu</type>
      <name>test_afu</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac9891bc8aab5db05ddbcfba187ba231a</anchor>
      <arglist></arglist>
    </member>
    <member kind="typedef">
      <type>opae::afu_test::command</type>
      <name>test_command</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a0a6fc2d03143e8204e8243a38addcee7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ACTIVE</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a6e2000299654529dae8c4c4af161e49c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a39b57be20840498a98f4cd174e292a4b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_ERROR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a3964c55dcdc68e90d56bf757b65b1d8a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_PASS</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77afbb80104f01fcbf03960c1b685d33aa3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_STATUS_RESPONSE_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5d9d9825a394d5cd74f8fe5396624a77a12b728ef588392d79acd492c13e5df0d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaa8b6510cf448bb867772dc2ec2aa9f8f1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_SEQ</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaa0282e80a72d9f469c95c722f5ac1c423</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_RAND_SEQ</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaa569b807ecd9597d27fbd18025e926146</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_ONE_HOT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af666da5e7c0a6dcca811edb68ea7cfbaaaa9f845f52633dddf601fb2a012a5d06</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_FIXED</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36afa04848192643b5020f9cbed0307c292</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS7</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36a22c5a9122b9ee1237a486ee9793c7380</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS15</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36a5fcae71881d710056306ecbd59d1f353</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_PRBS31</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36adb1e0653890ac8340af449158f0ae62e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_ROTATING</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa375902ebcc3c4d140be55be6817fd36a582126acece0943286941192f1c5e00e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a0fcea165efcbc477fd0a5f9f05bad92d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a70d3478f33ac42261cce33ae92a58440</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_ID_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54aa752f89b2cc2fb5ecfb034a3aadc2e3c</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>NEXT_AFU</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54ac123d30b0268ad1c521e9e98feed74b2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>AFU_DFH_RSVD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a376cf9f030a0b65a10226edf162499eb</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>SCRATCHPAD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a4c62d78ea8b14f6f8fb749088d81c6b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CTRL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a71c07e22c468e2fac16fc82de00ae213</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_STAT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54ab2e545ddbd0f4ff871aa8c2b227c56ed</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>MEM_TG_CLOCKS</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ac0cdbbdcef2cc707d348513b5579eb54a92a59d89dfb05439439512e623f02212</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_VERSION</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa04aae5e4774f4ba73394e884387749c5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_START</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa36e5f2f8a7ce70509baad0fc29c7217d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_LOOP_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faab7410bb101bc061d4ae261d012f9fcb4</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa712daf68ed5d46d6cba0a0033eb45a59</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaf91e5aaa66fc608c1afc2cf0a2a20f49</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_WRITE_REPEAT_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa4cd1c35dc2ff3d288cd1cfc60777941e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_READ_REPEAT_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaf7d1449e46dfd3fa0fd874fb9f44c2b9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BURST_LENGTH</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faad9acb4dff9aaa1df270472237301ed7a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_CLEAR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8fb799f1eb48b8410df89647e8122194</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_IDLE_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa0e952a276e6151f1d59e03f7f43f5c37</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RW_GEN_LOOP_IDLE_COUNT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa22b7465ca8a636c2a167ec4edc46ec4e</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_WR_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa78901634d9ccae75648d724f9af99a51</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_WR_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa684f05ecb4378c337d13ec3f8fee2fc8</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_WR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaa1ae8be038850228674d43f5bf0a756f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RAND_SEQ_ADDRS_WR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa6907903904e0653d73a30bdf29a27ed2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RETURN_TO_START_ADDR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa0f7198530578f4c0eec41f4c3f05a3be</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_ADDR_INCR</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8963a3103bffd3412a2340d14d93b21d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_RD_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaf68f3e5878b9fa4c7223312f75cf4a54</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_SEQ_START_ADDR_RD_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8719a1d0099392f7568b960a34f23ca9</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ADDR_MODE_RD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa0d0b4dc47ddd3f5315c986c4297c65cf</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RAND_SEQ_ADDRS_RD</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa9eed28651432b26b6e893537dad1c8f0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PASS</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaa3ebbf541ffa826713743898a9b87fca</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa77004e9dec4a1b60919402a1099c9b9b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa21b985a60da2e963c039573cf8ce4292</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_COUNT_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3f9757573c68c6984a904badea0ddac3</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3aa084a514969080b97217bd35465356</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FIRST_FAIL_ADDR_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faabeb38326d70d8fa51fa47bf2771e5443</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_L</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa8a1320c318666ad7219be02ffb547ff2</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TOTAL_READ_COUNT_H</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faafddea5c425e65e7c09f31f3717ff89a7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_COMPLETE</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa83044d71ca0917991b76ef043f7683b5</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_INVERT_BYTEEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa83be95f292fd349ce0d8de1679bcfc40</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RESTART_DEFAULT_TRAFFIC</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3e5d4c298071e1d27a597c6f0e2e2873</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_USER_WORM_EN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa04a628e98d370b2e44979a1cb0d934b7</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TEST_BYTEEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa6fc1ee1598e27892c1b190338a727d4a</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa26be34da618bd9ff6af7e5085468c683</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_DATA_GEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3abebe209bc7ba6e3993bc9950881722</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_NUM_BYTEEN_GEN</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa066ea4e9f56bdf1df417becadb27a4af</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_RDATA_WIDTH</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faaee7adf738b7b7831a13aa4a40687bd68</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_ERROR_REPORT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faabbb606f7914787cb5c521f428fb72562</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_RATE_WIDTH_RATIO</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa4afe73ab23f12ae3ab710f533707438f</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PNF</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faabba43d1a9d25c20bf14088aa3c92b72d</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_EXPECTED_DATA</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa99701a288d09c6214525e1cc65a64ba0</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_FAIL_READ_DATA</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa5b509ef9f39fcdbdaa760abce6c05cf1</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_DATA_SEED</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa3ff28b39ca90987ee5328faf55f7cc73</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEED</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa91c7cb5a3402c47f6435ddaa99a35d1b</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_PPPG_SEL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faa4206df51ef621a3c26c4b32d28bdd748</anchor>
      <arglist></arglist>
    </member>
    <member kind="enumvalue">
      <name>TG_BYTEEN_SEL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a2bc8acdef632ab5e18ed7bc7fb93e0faab2e2b4a6cb66d31dc888869bb2a42afe</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const char *</type>
      <name>AFU_ID</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>af02ccce98e14a5901318de5305ff2792</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>MEM_TG_TEST_TIMEOUT</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ab99947d9ae3914d454524af4289c4f75</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static const uint64_t</type>
      <name>TEST_SLEEP_INVL</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa14a41e302dcb5763a1dfa2af93083cb</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const std::map&lt; std::string, uint32_t &gt;</type>
      <name>tg_pattern</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>aa321b366aefc06c5924ed83f3fac4109</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>const int</type>
      <name>MEM_TG_CFG_OFFSET</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a943a6cf589a5fd0b37a523b5df6513e6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::mutex</type>
      <name>tg_print_mutex</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>afb390604495f7ec7dd3aee9cc7b67d8e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::mutex</type>
      <name>tg_start_write_mutex</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>ad0bd8092778fe7d39d3e37402e72386c</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::condition_variable</type>
      <name>tg_cv</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a9829f06cc0a289cb4b12f3ceeb5b94c6</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>std::atomic&lt; int &gt;</type>
      <name>tg_waiting_threads_counter</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a1b62d23fb279e2d99d781ac03a43276e</anchor>
      <arglist></arglist>
    </member>
    <member kind="variable">
      <type>int</type>
      <name>tg_num_threads</name>
      <anchorfile>namespacemem__tg.html</anchorfile>
      <anchor>a5f9ac460f06d8d421c98634269f87cd4</anchor>
      <arglist></arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>opae</name>
    <filename>namespaceopae.html</filename>
    <namespace>opae::afu_test</namespace>
    <namespace>opae::fpga</namespace>
  </compound>
  <compound kind="namespace">
    <name>opae::afu_test</name>
    <filename>namespaceopae_1_1afu__test.html</filename>
    <class kind="union">opae::afu_test::pcie_address</class>
    <class kind="class">opae::afu_test::command</class>
    <class kind="class">opae::afu_test::afu</class>
    <class kind="struct">opae::afu_test::pcie_address.fields</class>
    <member kind="function">
      <type>bool</type>
      <name>parse_match_int</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>a51c524d228e9ad342ed2e1a45efe741b</anchor>
      <arglist>(const char *s, regmatch_t m, T &amp;v, int radix=10)</arglist>
    </member>
    <member kind="function">
      <type>std::vector&lt; std::string &gt;</type>
      <name>spdlog_levels</name>
      <anchorfile>namespaceopae_1_1afu__test.html</anchorfile>
      <anchor>a86ee03f14e97bbb40476aacd85cc9868</anchor>
      <arglist>()</arglist>
    </member>
  </compound>
  <compound kind="namespace">
    <name>opae::fpga</name>
    <filename>namespaceopae_1_1fpga.html</filename>
    <namespace>opae::fpga::types</namespace>
  </compound>
  <compound kind="namespace">
    <name>opae::fpga::types</name>
    <filename>namespaceopae_1_1fpga_1_1types.html</filename>
    <namespace>opae::fpga::types::detail</namespace>
    <class kind="class">opae::fpga::types::error</class>
    <class kind="class">opae::fpga::types::event</class>
    <class kind="class">opae::fpga::types::src_location</class>
    <class kind="class">opae::fpga::types::except</class>
    <class kind="class">opae::fpga::types::invalid_param</class>
    <class kind="class">opae::fpga::types::busy</class>
    <class kind="class">opae::fpga::types::exception</class>
    <class kind="class">opae::fpga::types::not_found</class>
    <class kind="class">opae::fpga::types::no_memory</class>
    <class kind="class">opae::fpga::types::not_supported</class>
    <class kind="class">opae::fpga::types::no_driver</class>
    <class kind="class">opae::fpga::types::no_daemon</class>
    <class kind="class">opae::fpga::types::no_access</class>
    <class kind="class">opae::fpga::types::reconf_error</class>
    <class kind="class">opae::fpga::types::handle</class>
    <class kind="class">opae::fpga::types::properties</class>
    <class kind="struct">opae::fpga::types::guid_t</class>
    <class kind="struct">opae::fpga::types::pvalue</class>
    <class kind="class">opae::fpga::types::shared_buffer</class>
    <class kind="class">opae::fpga::types::sysobject</class>
    <class kind="class">opae::fpga::types::token</class>
    <class kind="class">opae::fpga::types::version</class>
  </compound>
  <compound kind="namespace">
    <name>opae::fpga::types::detail</name>
    <filename>namespaceopae_1_1fpga_1_1types_1_1detail.html</filename>
    <member kind="typedef">
      <type>bool(*</type>
      <name>exception_fn</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>a27ea035002c36a1c80c601a99b9fa3db</anchor>
      <arglist>)(fpga_result, const opae::fpga::types::src_location &amp;loc)</arglist>
    </member>
    <member kind="function">
      <type>constexpr bool</type>
      <name>is_ok</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>ad6802666a800e1d9b4ffe647a816083f</anchor>
      <arglist>(fpga_result result, const opae::fpga::types::src_location &amp;loc)</arglist>
    </member>
    <member kind="function" static="yes">
      <type>static void</type>
      <name>assert_fpga_ok</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>afb71b5ca2216303a26cd91e01da63bc2</anchor>
      <arglist>(fpga_result result, const opae::fpga::types::src_location &amp;loc)</arglist>
    </member>
    <member kind="variable" static="yes">
      <type>static exception_fn</type>
      <name>opae_exceptions</name>
      <anchorfile>namespaceopae_1_1fpga_1_1types_1_1detail.html</anchorfile>
      <anchor>aaf5c7cfe3e7998add9471462d43ee1ff</anchor>
      <arglist>[12]</arglist>
    </member>
  </compound>
</tagfile>
