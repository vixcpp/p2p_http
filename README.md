# vixcpp/p2p_http

HTTP adapter module for Vix.cpp P2P.

This module bridges **vix::core** (Router / HTTP types) with **vix::p2p** (P2PRuntime) without introducing circular dependencies.

## API

```cpp
#include <vix/p2p/http/P2PHttp.hpp>

vix::router::Router router;
vix::p2p::P2PRuntime p2p(node);

vix::p2p::http::registerP2PRoutes(router, p2p);
```
## Default routes

GET /p2p/ping
GET /p2p/status
POST /p2p/hook (501 placeholder)

---

## 6) (Optionnel) Exemple d’usage côté app

Dans ton app :

```cpp
#include <vix.hpp>
#include <vix/p2p/http/P2PHttp.hpp>

using namespace vix;

int main()
{
  App app;

  // ... init node + runtime p2p

  auto r = app.router();
  vix::p2p::http::registerP2PRoutes(*r, p2p);

  app.listen(8080);
  app.wait();
}
```
