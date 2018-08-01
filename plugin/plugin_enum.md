sequenceDiagram
    participant ClientApp
    ClientApp->>opae: fpgaEnumerate(filter)
    loop ForEach(A in AdapterTables(filter))
        opae->>A: fpgaEnumerate(filter)
        A-->>opae: ptokens
        loop ForEach(ptoken in tokens)
            opae-->>opae: wrap(ptoken)
        end
    opae-->>ClientApp:tokens
    end