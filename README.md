# Vix P2P HTTP

**HTTP control surface for Vix P2P runtimes**

Simple. Observable. Scriptable.

---

## Overview

`vix/p2p_http` exposes an HTTP layer on top of the P2P runtime.

It lets you inspect and control a running node through regular HTTP endpoints such as:

- `GET /ping`
- `GET /status`
- `POST /connect`
- `GET /peers`
- `GET /logs`
- `POST /admin/hook`

Routes are registered through `registerRoutes(app, runtime, opt)`, and behavior is controlled with `P2PHttpOptions` such as `prefix`, `enable_*`, `auth_ctx`, `auth_legacy`, `log_sink`, and `stats_every_ms`. fileciteturn2file0

---

## Why this module exists

`vix/p2p` gives you the runtime.

`vix/p2p_http` gives you the control plane.

That means you can:

- probe a node from curl or a browser
- connect peers remotely with JSON
- inspect current peer state
- expose logs for dashboards
- add lightweight auth around sensitive routes

This module is designed for operational visibility and small control APIs, not for replacing the P2P engine itself. fileciteturn2file0

---

## Main API

### Register routes

```cpp
void registerRoutes(
    vix::App &app,
    vix::p2p::P2PRuntime &runtime,
    const P2PHttpOptions &opt);
```

This is the entry point of the module. It binds HTTP routes onto an application and wires them to a live `P2PRuntime`. fileciteturn2file0

### Shutdown live logs

```cpp
void shutdown_live_logs();
```

Stops the live log thread and clears the global P2P log sink. fileciteturn2file0

### External live log sink

```cpp
void set_live_log_sink(std::function<void(std::string)> sink);
```

Lets you redirect live log lines somewhere else, such as SSE, WebSocket, or a custom dashboard sink. fileciteturn2file0

---

## Configuration

`P2PHttpOptions` controls the exposed endpoints and route behavior.

```cpp
struct P2PHttpOptions
{
  std::string prefix = "/p2p";
  bool enable_ping = true;
  bool enable_status = true;
  bool enable_logs = true;
  bool enable_live_logs = true;
  int stats_every_ms = 1000;
  bool enable_peers{true};
  AuthHookCtx auth_ctx = nullptr;
  AuthHookLegacy auth_legacy = nullptr;
  LogSink log_sink = nullptr;
};
```

Notable fields:

- `prefix` changes the base URL, for example `/p2p` or `/control`
- `enable_ping` exposes `GET <prefix>/ping`
- `enable_status` exposes `GET <prefix>/status`
- `enable_peers` exposes both `GET <prefix>/peers` and `POST <prefix>/connect`
- `enable_logs` exposes `GET <prefix>/logs`
- `enable_live_logs` starts the background stats-to-log thread when logs are enabled
- `auth_ctx` is the preferred middleware-based auth hook
- `auth_legacy` is the raw request/response fallback
- `log_sink` lets you consume module log lines yourself fileciteturn2file0

---

## Exposed routes

### `GET /ping`

Returns a simple JSON health response:

```json
{
  "ok": true,
  "pong": true,
  "module": "p2p_http"
}
```

Useful for smoke tests and wiring validation. fileciteturn2file0

### `GET /status`

Returns runtime-level P2P stats including:

- `peers_total`
- `peers_connected`
- `handshakes_started`
- `handshakes_completed`
- `connect_attempts`
- `connect_deduped`
- `connect_failures`
- `backoff_skips`
- `tracked_endpoints` fileciteturn2file0

### `POST /connect`

Accepts JSON like:

```json
{
  "host": "127.0.0.1",
  "port": 9002,
  "scheme": "tcp"
}
```

Then asks the runtime node to connect to that peer endpoint. This route exists only when `enable_peers` is enabled. fileciteturn2file0

### `GET /peers`

Returns a stable, sorted view of known peers, including:

- `peer_id`
- connection `state`
- resolved endpoint
- security flags
- handshake stage
- last seen age
- short public/session key fingerprints

The handler sorts peers by `peer_id` before serializing the response. fileciteturn2file0

### `GET /logs`

Returns the in-memory log buffer as plain text. The module keeps a bounded internal buffer and appends runtime-related lines to it. fileciteturn2file0

### `POST /admin/hook`

This is the admin-oriented route used to demonstrate:

- auth protection
- heavy route tagging
- future extension points

Right now it returns `501 not_implemented`. fileciteturn2file0

---

## Middleware behavior

The module supports two small middleware helpers:

- `auth_hook(P2PHttpOptions)` rejects unauthenticated access with HTTP 401 when no valid auth hook is present
- `heavy_tag()` adds `x-vix-route-heavy: 1` to responses for heavy routes fileciteturn2file0

When `VIX_P2P_HTTP_WITH_MIDDLEWARE` is enabled, `registerRoutes()` installs route-level middleware for auth and heavy tagging. Without middleware, `auth_legacy` is used as the fallback for the admin route. fileciteturn2file0

---

## Examples

The examples below follow the real route surface implemented by the module.

### 1. Ping route

```bash
vix run examples/p2p_http/01_ping_route_basic.cpp
```

Then:

```bash
curl http://127.0.0.1:8080/p2p/ping
```

### 2. Status route

```bash
vix run examples/p2p_http/02_status_route_basic.cpp
```

Then:

```bash
curl http://127.0.0.1:8081/p2p/status
```

### 3. Custom prefix

```bash
vix run examples/p2p_http/03_custom_prefix.cpp
```

Then:

```bash
curl http://127.0.0.1:8082/control/ping
curl http://127.0.0.1:8082/control/status
```

### 4. Connect route

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

### 5. Peers route

Terminal 1:

```bash
vix run examples/p2p_http/05_peers_route_basic.cpp --run target
```

Terminal 2:

```bash
vix run examples/p2p_http/05_peers_route_basic.cpp --run api
```

Then connect and inspect:

```bash
curl -X POST http://127.0.0.1:8084/p2p/connect \
  -H "content-type: application/json" \
  -d '{"host":"127.0.0.1","port":9211,"scheme":"tcp"}'

curl http://127.0.0.1:8084/p2p/peers
```

### 6. Logs route

```bash
vix run examples/p2p_http/06_logs_route_basic.cpp
```

Then:

```bash
curl http://127.0.0.1:8085/p2p/logs
curl http://127.0.0.1:8085/p2p/status
```

### 7. Middleware auth hook

```bash
vix run examples/p2p_http/07_auth_hook_ctx_basic.cpp
```

Then:

```bash
curl -X POST http://127.0.0.1:8086/p2p/admin/hook
curl -X POST http://127.0.0.1:8086/p2p/admin/hook -H "x-auth-token: secret"
```

This example is only effective when middleware support is enabled. fileciteturn2file0

### 8. Legacy auth hook

```bash
vix run examples/p2p_http/08_auth_hook_legacy_basic.cpp
```

Then:

```bash
curl -X POST http://127.0.0.1:8087/p2p/admin/hook
curl -X POST http://127.0.0.1:8087/p2p/admin/hook -H "x-auth-token: legacy-secret"
```

### 9. Heavy admin route

```bash
vix run examples/p2p_http/09_heavy_admin_hook.cpp
```

Then:

```bash
curl -i -X POST http://127.0.0.1:8088/p2p/admin/hook -H "x-auth-token: admin"
```

Look for:

```text
x-vix-route-heavy: 1
```

### 10. Full dashboard server

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

Admin header:

```text
x-auth-token: dashboard-admin
```

---

## Important `vix run` rule

For script mode examples:

```text
--      = compiler or linker flags
--run   = runtime arguments passed to argv
```

So these are correct:

```bash
vix run examples/p2p_http/04_connect_route_basic.cpp --run api
vix run examples/p2p_http/05_peers_route_basic.cpp --run target
```

And this style is wrong for runtime args:

```bash
vix run file.cpp -- api
```

Because `api` would be treated as a compiler or linker argument, not as a program argument.

---

## Operational notes

- The log buffer is bounded internally and stored in memory. fileciteturn2file0
- Live stats logging starts only when both `enable_logs` and `enable_live_logs` are enabled. fileciteturn2file0
- The module installs a global P2P log sink and should be shut down cleanly with `shutdown_live_logs()`. fileciteturn2file0
- `POST /connect` and `GET /peers` are tied to `enable_peers`. fileciteturn2file0
- The admin route is intentionally present even though its body is currently a placeholder returning `501`. fileciteturn2file0

---

## Summary

`vix/p2p_http` is the small HTTP control layer for the P2P runtime.

It is useful when you want to:

- inspect nodes from scripts or dashboards
- drive peer connections through JSON
- expose runtime stats over HTTP
- stream or export logs
- secure control endpoints with lightweight auth

It is the operational face of the P2P engine.

---

## License

MIT License
Copyright (c) 2025 Gaspard Kirira

