# OPAE C++ API
## Overview
The OPAE C++ API enables C++ developers with the means to use FPGA resources by integrating the OPAE software stack into C++ applications. It is built on top of the OPAE C API and is split up into two components:
1. The basic or fundemental types that wrap the data structures defined in the OPAE C API.
2. Classes that abstract FPGA resources and functions

## Goals
The following is a list of goals and design decisions made while designing this API.

### Simplicity
Keep the API as small and lightweight as possible. Although features such as system validation or orchestration are beyond the scope of this API, using this API for their development should be relatively easy.

### Extensibility and Interoperability
While keeping to the goal of simplicity, the OPAE C++ API is designed to allow for better reuse by either extending the API or by integrating with other languages. 

### Modern C++ Coding Practices
The OPAE C++ API will use the C++ 11 standard library and make use of its features whenever practical. Furthermore, the OPAE C++ API is designed to require as few as possible any other third party libraries/dependencies.

### Error Handling
The OPAE C++ API is designed to throw exceptions when appropriate. The structure of OPAE C++ exceptions will be similar to the error codes in the OPAE C API. This gives users of the API more freedom on error handling while providing better debug information in cases of failure.

### Coding Style
_TODO_

## Fundamental Types
Basic types for the OPAE C++ API are found in the `opae.fpga.types` namespace.
They serve as an adapter layer between the OPAE-C API and the OPAE-C++ layer.
Aside from providing a C++ binding to the C fundemental types, these types also
* Manage the lifetime and scope of the corresponding C struct
  * For example, a C++ destructor will take care of calling the appropriate C function to deallocate or release the data structure being wrapped
* Provide an easy to use syntax for using them

Most classes in this namespace have a `get()` method that returns the C data structure being wrapped. This is useful for when one wishes to use one of OPAE C API functions.

Below is a diagram of the classes defined in the opae::fpga::types namespace.

```plantuml
@startuml
namespace opae.fpga.types {

    class pvalue<< property >>{
        -fpga_properties *props_
        -get_
        -set_
        +pvalue()
        +operator=()
        +get_value()
    }

    class properties {
        +pvalue<fpga_objtype> object_type
        +pvalue<uint8_t> bus
        +pvalue<uint8_t> device
        +pvalue<uint8_t> function
        +pvalue<uint8_t> socket_id
        +pvalue<uint32_t> num_slots
        +pvalue<uint64_t> bbs_id
        +pvalue<fpga_version> bbs_version
        +pvalue<uint16_t> vendor_id
        +pvalue<char*> model
        +pvalue<uint64_t> local_memory_size
        +pvalue<uint64_t> capabilities
        +pvalue<fpga_guid> guid
        +pvalue<uint32_t> num_mmio
        +pvalue<uint32_t> num_interrupts
        +pvalue<fpga_accelerator_state> accelerator_state
        +pvalue<uint64_t> object_id
        +pvalue<fpga_token> parent
        +fpga_properties get()
        +{static} properties::ptr_t read(fpga_token t)
        -fpga_properties props_
    }

    class token {
        +{static} token::ptr_t enumerate(const std::vector<properties> & props)
        +fpga_token get()
        -token(fpga_token t)
        -fpga_token token_
    }
    
    class handle {
        +fpga_handle get()
        +{static} handle::ptr_t open(fpga_token t, int flags)
        +{static} handle::ptr_t open(token::ptr_t t, int flags)
        +fpga_result close()
        -handle(fpga_handle h)
        -fpga_handle handle_
    }


}

@enduml
```

```plantuml
@startuml

namespace opae.fpga.error {

    class port_error {
        +uint64_t value()
    }
    class fme_error{
        +uint64_t value()
    }

    class fpga_error

    fpga_error <|-- std::exception
    port_error <|-- fpga_error
    fme_error <|-- fpga_error


}
@enduml
```


## Resource Abstractions
The second layer in the OPAE C++ API encapsulates fpga resources into types that tie the fundemental types to the methods or operations applicaple to them. The classes in this layer are designed in a hierarchy that enables more code reuse through inheritance or by composition.

```plantuml
@startuml
namespace opae.fpga.resource {
    enum status {
        closed
        open
        error
    }

    abstract class device {
        + status open(bool s)
        + status close()
        + is_open()
        {static} tokens enumerate_tokens(filter f)
    }

    class fpga {
        + reconfigure(string gbsfile)
        {static} fpga* enumerate(filter f)
    }

    class accelerator {
        {static} accelerator* enumerate(filter f)
        + buffer allocate_buffer(size s)
        + buffer use_buffer(char* buff, size s)
        + char* mmio_pointer(size_t offset = 0)
        + void read_mmio32(size_t offset, uint32_t & value)
        + void read_mmio64(size_t offset, uint64_t & value)
        + void write_mmio32(size_t offset, uint32_t value)
        + void write_mmio64(size_t offset, uint64_t value)

    }

    class buffer {
        + buffer(uint8_t* address, uint64_t iova, size_t size, buffer::ptr_t parent)
        + void release()
        + uint8_t* address()
        + uint64_t iova()
        + size_t size()
        + buffer::ptr_t split(size_t s)
        - uint8_t* address_
        - uint64_t iova_
        - size_t size_
        - buffer::ptr_t parent_
    }
   
    device <|-- fpga 
    device <|-- accelerator
    
}
@enduml
```

## Examples
```c++

    int main(int argc, char* argv[])
    {

    }

```