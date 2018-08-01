sequenceDiagram
    participant ClientApp
    participant O as opae.PluginManager
    Note over O: Initalize can be implicit
    ClientApp->>O: Initialize(cfile)
    loop ForEach(N in NativePlugins)
        N->>N: CreateAdapterTable()
        Note over N: Map Plugin FN Ptrs
        N->>O: Register(AdapterTable)
        opt
            O->>N: Configure(cdata)
            O->>N: Initialize()
        end
    end
    O->>O: ParseConfig(cfile)
    loop ForEach(P in OtherPlugins)
        O->>O: LoadPlugin(P)
        participant P
        activate P
        P->>P: CreateAdapterTable()
        Note over P: Map Plugin FN Ptrs
        P->>O: Register(AdapterTable)
        opt
            O->>P: Configure(cdata)
            O->>P: Initialize()
        end
        deactivate P
    end