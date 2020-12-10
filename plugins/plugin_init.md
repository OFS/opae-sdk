```mermaid
sequenceDiagram
    participant ClientApp
    participant O as opae.PluginManager
    Note over O: Initalize can be implicit
    ClientApp->>O: Initialize(cfile)
    O->>O: ParseConfig(cfile)
    loop ForEach(N in NativePlugins)
        O->>O: InitializeAdapterTable() -> a_table
        O->>N: PluginConfigure(a_table, cdata)
        N->>N: FillAdapterTable(a_table)
        opt
            O->>N: Initialize()
        end
    end
    loop ForEach(P in OtherPlugins)
        O->>O: LoadPlugin(P)
        participant P
        activate P
        O->>O: InitializeAdapterTable()->a_table
        O->>P: PluginConfigure(a_table, cdata)
        opt
            O->>P: Initialize()
        end
        deactivate P
    end
```