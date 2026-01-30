# p2p_http — HTTP control plane for Vix P2P

`p2p_http` is a lightweight HTTP module for **Vix.cpp** that exposes a control API
to **observe, manage, and interact with the P2P runtime**.

It is designed as a **control plane**, not a data plane:
- HTTP is used for **introspection and orchestration**
- P2P transports (TCP, QUIC, …) are used for **actual peer-to-peer traffic**

---

## Goals

- Provide a simple HTTP API to control the P2P node
- Enable web UIs, CLIs, and dashboards to interact with P2P
- Stay **stateless, safe, and non-blocking**
- Integrate cleanly with Vix middleware and runtime

---

## What this module does

- Exposes HTTP endpoints on top of a running `vix::p2p::Node`
- Allows external clients (web UI, curl, CLI) to:
  - Inspect node status
  - List peers
  - Connect to new peers
- Acts as a **bridge between HTTP and the P2P runtime**

---

## What this module does NOT do

- ❌ It does not implement P2P protocols
- ❌ It does not handle encryption or transport logic
- ❌ It does not store peer data persistently

All networking, crypto, handshakes, and state live in `vix::p2p`.

---

## API Overview

### GET /p2p/status

Returns global node status and statistics.

```json
{
  "ok": true,
  "running": true,
  "peers_total": 3,
  "peers_connected": 2,
  "handshakes_started": 4,
  "handshakes_completed": 2
}
```

---

### GET /p2p/peers

Returns a snapshot of known peers.

```json
{
  "ok": true,
  "peers": [
    {
      "id": "a3f1…",
      "state": "Connected",
      "endpoint": "tcp://127.0.0.1:9002",
      "secure": true,
      "last_seen_ms": 1234
    }
  ]
}
```

---

### POST /p2p/connect

Connects to a peer endpoint.

Request body:
```json
{
  "host": "127.0.0.1",
  "port": 9002,
  "scheme": "tcp"
}
```

Response:
```json
{
  "ok": true,
  "started": true,
  "endpoint": "tcp://127.0.0.1:9002"
}
```

Validation rules:
- `host` must be non-empty
- `port` must be in range 1–65535
- `scheme` defaults to `tcp`

---

## Usage Example

```cpp
#include <vix.hpp>
#include <vix/console.hpp>
#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>
#include <vix/p2p_http/P2PHttp.hpp>

using namespace vix;

int main()
{
  App app;

  vix::p2p::NodeConfig cfg;
  cfg.node_id = "A";
  cfg.listen_port = 9001;

  auto node = vix::p2p::make_tcp_node(cfg);
  vix::p2p::P2PRuntime runtime(node);
  runtime.start();

  vix::p2p_http::P2PHttpOptions opt;
  opt.prefix = "/api/p2p";
  opt.enable_ping = true;
  opt.enable_status = true;
  opt.enable_peers = true;
  opt.enable_logs = true;
  opt.enable_live_logs = true;
  opt.stats_every_ms = 250;

  vix::p2p_http::registerRoutes(app, runtime, opt);

  app.static_dir("./public");
  app.get("/", [](Request &, Response &res)
          { res.file("./public/index.html"); });

  app.get("/connect", [](Request &, Response &res)
          { res.file("./public/connect.html"); });

  app.listen(5178, [](const vix::utils::ServerReadyInfo &info)
             { console.info("UI API listening on", info.port); });

  app.wait();
  runtime.stop();
}
```

---

## Middleware Integration

When compiled with middleware support, routes can be marked as heavy or protected.
This allows safe exposure in production environments.

---

## Threading & Safety

- All P2P operations are dispatched into the P2P runtime
- HTTP handlers never block on network operations
- JSON parsing is validated and defensive
- No raw pointers are stored across requests

---

## Design Philosophy

- Control plane only
- No hidden background threads
- Explicit and observable behavior
- Crash-safe input handling

---

## License

MIT — same as Vix.cpp
https://github.com/vixcpp/vix

