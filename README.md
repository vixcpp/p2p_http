# Vix.cpp P2P HTTP Module

HTTP control surface for Vix P2P runtimes.

The P2P HTTP module exposes a small HTTP control layer on top of the P2P runtime. It lets you inspect, operate, and control a running P2P node through regular HTTP endpoints.

## Documentation

Full documentation will be available here:

https://docs.vixcpp.com/modules/p2p-http/

API reference:

https://docs.vixcpp.com/modules/p2p-http/api-reference

## What P2P HTTP provides

- HTTP control surface for P2P nodes
- Health check endpoint
- Runtime status endpoint
- Peer connection endpoint
- Peer inspection endpoint
- Logs endpoint
- Admin hook endpoint
- Custom route prefix support
- Optional auth hooks
- Optional middleware integration
- Live runtime log support
- Dashboard-friendly operational API

## Public header

```cpp
#include <vix/p2p_http.hpp>
```

Or include specific headers when needed:

```cpp
#include <vix/p2p_http/routes.hpp>
#include <vix/p2p_http/options.hpp>
```

## Basic idea

```text
vix::p2p
  -> P2P runtime

vix::p2p_http
  -> HTTP control plane
  -> status
  -> peers
  -> connect
  -> logs
  -> admin hooks
```

`vix/p2p` gives you the runtime.
`vix/p2p_http` gives you the operational control surface.

## Register routes

```cpp
#include <vix.hpp>
#include <vix/p2p.hpp>
#include <vix/p2p_http.hpp>

int main()
{
  vix::App app;

  vix::p2p::P2PRuntime runtime;

  vix::p2p_http::P2PHttpOptions options;
  options.prefix = "/p2p";

  vix::p2p_http::registerRoutes(app, runtime, options);

  app.run(8080);

  return 0;
}
```

## Exposed routes

With the default prefix:

```text
GET  /p2p/ping
GET  /p2p/status
GET  /p2p/peers
GET  /p2p/logs
POST /p2p/connect
POST /p2p/admin/hook
```

## Ping route

```bash
curl http://127.0.0.1:8080/p2p/ping
```

Example response:

```json
{
  "ok": true,
  "pong": true,
  "module": "p2p_http"
}
```

## Status route

```bash
curl http://127.0.0.1:8080/p2p/status
```

The status endpoint exposes runtime-level P2P stats such as peer count, connected peers, handshake counters, connection attempts, failures, backoff skips, and tracked endpoints.

## Connect route

```bash
curl -X POST http://127.0.0.1:8080/p2p/connect \
  -H "content-type: application/json" \
  -d '{"host":"127.0.0.1","port":9002,"scheme":"tcp"}'
```

This asks the runtime to connect to a peer endpoint.

## Peers route

```bash
curl http://127.0.0.1:8080/p2p/peers
```

The peers endpoint returns known peers, their state, endpoint information, handshake state, security flags, and key fingerprints.

## Logs route

```bash
curl http://127.0.0.1:8080/p2p/logs
```

The logs endpoint returns the in-memory P2P HTTP log buffer as plain text.

## Custom prefix

```cpp
vix::p2p_http::P2PHttpOptions options;
options.prefix = "/control";

vix::p2p_http::registerRoutes(app, runtime, options);
```

Routes become:

```text
GET  /control/ping
GET  /control/status
GET  /control/peers
GET  /control/logs
POST /control/connect
POST /control/admin/hook
```

## Options

```cpp
vix::p2p_http::P2PHttpOptions options;

options.prefix = "/p2p";
options.enable_ping = true;
options.enable_status = true;
options.enable_peers = true;
options.enable_logs = true;
options.enable_live_logs = true;
options.stats_every_ms = 1000;
```

## Runtime examples

### Ping route

```bash
vix run examples/p2p_http/01_ping_route_basic.cpp
```

Then:

```bash
curl http://127.0.0.1:8080/p2p/ping
```

### Status route

```bash
vix run examples/p2p_http/02_status_route_basic.cpp
```

Then:

```bash
curl http://127.0.0.1:8081/p2p/status
```

### Custom prefix

```bash
vix run examples/p2p_http/03_custom_prefix.cpp
```

Then:

```bash
curl http://127.0.0.1:8082/control/ping
curl http://127.0.0.1:8082/control/status
```

### Connect route

Terminal 1:

```bash
vix run examples/p2p_http/04_connect_route_basic.cpp --run target
```

Terminal 2:

```bash
vix run examples/p2p_http/04_connect_route_basic.cpp --run api
```

Then:

```bash
curl -X POST http://127.0.0.1:8083/p2p/connect \
  -H "content-type: application/json" \
  -d '{"host":"127.0.0.1","port":9201,"scheme":"tcp"}'
```

### Peers route

```bash
vix run examples/p2p_http/05_peers_route_basic.cpp --run api
```

Then:

```bash
curl http://127.0.0.1:8084/p2p/peers
```

### Full dashboard server

```bash
vix run examples/p2p_http/10_full_dashboard_server.cpp
```

Useful endpoints:

```text
GET  /p2p/ping
GET  /p2p/status
GET  /p2p/peers
GET  /p2p/logs
POST /p2p/connect
POST /p2p/admin/hook
```

## Runtime arguments

When using `vix run`, keep this rule:

```text
--     = compiler or linker flags
--run  = runtime arguments passed to the program
```

Correct:

```bash
vix run examples/p2p_http/04_connect_route_basic.cpp --run api
```

Wrong:

```bash
vix run examples/p2p_http/04_connect_route_basic.cpp -- api
```

## Architecture

```text
vix::App
  -> P2P HTTP routes
  -> P2P runtime
  -> peers
  -> logs
  -> status
  -> control actions
```

With middleware enabled:

```text
Request
  -> auth hook
  -> heavy route tag
  -> P2P HTTP handler
  -> P2P runtime
```

## Operational notes

- The log buffer is bounded and stored in memory.
- Live stats logging starts only when logs and live logs are enabled.
- `POST /connect` and `GET /peers` depend on `enable_peers`.
- Admin routes should be protected with an auth hook.
- Live logs should be shut down cleanly when the app stops.

## Build

Contributors should use the Vix CLI to build this module.

Vix wraps the C++ build workflow with project detection, presets, Ninja builds, clean logs, caching, and focused diagnostics. This keeps the contributor workflow consistent and helps avoid hidden C++ build issues.

### Build the project

```bash
git clone https://github.com/vixcpp/vix.git
cd vix
vix build
```

### Build all targets

Use this before running the full test suite, install workflows, or release checks:

```bash
vix build --build-target all
```

### Clean rebuild

Use this when the local CMake cache or build directory may be stale:

```bash
vix build --clean
```

### Release build

```bash
vix build --preset release
```

## Tests

Build all targets first, then run tests:

```bash
vix build --build-target all
vix tests
```

Before opening a pull request, use:

```bash
vix fmt --check
vix build --build-target all
vix tests
```

## Useful links

- P2P HTTP documentation: https://docs.vixcpp.com/modules/p2p-http/
- P2P HTTP API reference: https://docs.vixcpp.com/modules/p2p-http/api-reference
- P2P documentation: https://docs.vixcpp.com/modules/p2p/
- P2P CLI command: https://docs.vixcpp.com/cli/p2p
- Build command: https://docs.vixcpp.com/cli/build
- Tests command: https://docs.vixcpp.com/cli/tests
- Documentation: https://docs.vixcpp.com/
- Engineering notes: https://blog.vixcpp.com/
- Registry: https://registry.vixcpp.com/
- GitHub: https://github.com/vixcpp/vix

## License

MIT License.

See [`LICENSE`](../../LICENSE) for details.
