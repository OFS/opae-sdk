```mermaid
sequenceDiagram
    participant opae
    participant ProxyPlugin
    opae->>ProxyPlugin: fpgaEnumerate(filter)
    ProxyPlugin->>ProxyPlugin: make_message(type=enum, payload=filter)->msg
    ProxyPlugin->>RemoteEndpoint: send_msg(msg)
    RemoteEndpoint->>remote_opae: fpgaEnumerate(filter)
    remote_opae-->>RemoteEndpoint: tokens
    RemoteEndpoint->>RemoteEndpoint:make_response(tokens)->resp
    RemoteEndpoint-->>ProxyPlugin:resp
    ProxyPlugin->>ProxyPlugin: unpack(resp)->proxy_tokens
    ProxyPlugin-->>opae: proxy_tokens
    Note over opae: proxy_tokens wrapped into tokens
 ```