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
Formatting of the OPAE C++ API will apply most of the recommendations of the Google C++ style style. For example, the OPAE C++ API will:
* Use opening braces on the same line as their scope definition
* Use spaces instead of tabs for indentation
* Use indentation of two spaces

Refer to the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) for more information.

## Fundamental Types
Basic types for the OPAE C++ API are found in the `opae.fpga.types` namespace.
They serve as an adapter layer between the OPAE-C API and the OPAE-C++ layer.
Aside from providing a C++ binding to the C fundemental types, these types also
* Manage the lifetime and scope of the corresponding C struct
  * For example, a C++ destructor will take care of calling the appropriate C function to deallocate or release the data structure being wrapped
* Provide an easy to use syntax for using them

Most classes in this namespace have a `get()` method that returns the C data structure being wrapped. This is useful for when one wishes to use one of OPAE C API functions. Additionally, most classes in this namespace have implicit conversion operators that allow instances of these types to be interoperable with OPAE C API functions. These operators are designed so that they are invoked when instances are converted to their corresponding C structure (either explicitly or implicitly).

The two diagrams below illustrate the design of the class in the `opae::fpga::types` namespace. 

### Properties classes
The first diagram shows classes related to `fpga_properties`. The class, `properties`, wraps `fpga_properties` and uses public members of type `pvalue` and `guid_t` to get/set properties stored in an instance of an `fpga_properties`.
These two classes are designed to call an accessor method in the OPAE C API to either read property values or set them. Because most accessor methods in the OPAE C API share a similar signature, `pvalue` generalizes them into common operations that translate into calling the corresponding C API function. The class, `guid_t`, follows similar patterns when reading or assigning values.



```plantuml
@startuml
namespace opae.fpga.types {

    class pvalue<< T >>{
        -fpga_properties *props_
        -get_
        -set_
        +pvalue(fpga_properties *props)
        +operator=(T value)
    }
    
    class guid_t {
        -fpga_properties *props_
        +guid_t(fpga_properties *props)
        +operator=(fpga_guid guid)
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
        +pvalue<uint32_t> num_mmio
        +pvalue<uint32_t> num_interrupts
        +pvalue<fpga_accelerator_state> accelerator_state
        +pvalue<uint64_t> object_id
        +pvalue<fpga_token> parent
        +guid_t guid
        +properties(fpga_guid guid)
        +properties(fpga_objtype objtype)
        +fpga_properties get()
        +operator fpga_properties()
        +{static} properties::ptr_t read(fpga_token t)
        -fpga_properties props_
    }
}
```

### Resource Classes
The basic types in `opae::fpga::types` used for enumerating and accessing fpga resources are shown in the diagram below.

```plantuml
@startuml
namespace opae.fpga.types {

    class token {
        +{static} token::ptr_t enumerate(const std::vector<properties> & props)
        +fpga_token get()
        +operator fpga_token()
        -token(fpga_token t)
        -fpga_token token_
    }
    
    class handle {
        +fpga_handle get()
        +operator fpga_handle()
        +bool write(uint64_t offset, uint32_t value)
        +bool write(uint64_t offset, uint64_t value)
        +bool read(uint64_t offset, uint32_t value)
        +bool read(uint64_t offset, uint64_t value)
        +uint8_t* mmio_ptr()
        +{static} handle::ptr_t open(fpga_token t, int flags)
        +{static} handle::ptr_t open(token::ptr_t t, int flags)
        #fpga_result close()
        -handle(fpga_handle h, uint32_t mmio_region, uint8_t* mmio_base)
        -fpga_handle handle_
        -uint32_t mmio_region_
        -uint8_t *mmio_base_
    }

    class dma_buffer {
        +{static}dma_buffer::ptr_t allocate(handle::ptr_t handle, size_t len)
        +{static}dma_buffer::ptr_t attach(handle::ptr_t handle, uint8_t *base, size_t len)
        +uint8_t *get()
        +handle::ptr_t owner()
        +size_t size()
        +uint64_t iova()
        +vector<dma_buffer::ptr_t> split(initializer_list sizes)
        +fill(int c)
        int compare(dma_buffer::ptr_t other, size_t len)
        +T read(size_t offset)
        +write(T value, size_t offset)
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
        const char* NLB0 = "D8424DC4-A4A3-C413-F89E-433683F9040B";
 
        properties props_filter;
 
        props_filter.socket_id = 1;
        props_filter.type = FPGA_ACCELERATOR;
        uuid_t uuid;
        if (uuid_parse(NLB0, uuid) == 0){
            props_filter.guid = uuid;
        }
        props_filter.bbs_id = 0; // This is invalid - libopae-c prints out a warning
        auto tokens = token::enumerate({props_filter});
        if (tokens.size() > 0){
            auto tok = tokens[0];
            auto props = properties::read(tok);
            std::cout << "guid prop read: " << props->guid << "\n";
 
            std::cout << "bus: 0x" << std::hex << props->bus << "\n";
            handle::ptr_t h = handle::open(tok, FPGA_OPEN_SHARED);
            uint64_t value1 = 0xdeadbeef, value2 = 0;
            h->write(0x100, value1);
            h->read(0x100, value2);
            std::cout << "mmio @0x100: 0x" << std::hex << value2 << "\n";
            std::cout << "mmio @0x100: 0x" << std::hex << *reinterpret_cast<uint64_t*>(h->mmio_ptr(0x100)) << "\n";
        }
 
        return  0;
    }

```
