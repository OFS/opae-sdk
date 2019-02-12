```mermaid
sequenceDiagram
    participant ClientApp
    participant opae
    participant A as plugin A
    ClientApp->>opae: fpgaOpen(token)
    opae->>opae: unwrap(token)->(adapter_table, A_token)
    opae->>A: fpgaOpen(A_token)
    A-->>opae: A_handle
    opae->>opae: wrap(A_handle)->handle
    opae-->>ClientApp: handle
```