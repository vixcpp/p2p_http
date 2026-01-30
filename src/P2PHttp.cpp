/**
 *
 *  @file P2PHttp.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */

#include <vix/p2p_http/P2PHttp.hpp>
#include <vix/p2p_http/P2PHttpOptions.hpp>
#include <vix/p2p_http/RouteOptions.hpp>

#include <vix/app/App.hpp>
#include <vix/http/RequestHandler.hpp>
#include <vix/http/Response.hpp>
#include <vix/json/json.hpp>

#include <vix/p2p/Node.hpp>
#include <vix/p2p/P2P.hpp>

#include <string>
#include <utility>
#include <deque>
#include <mutex>
#include <sstream>
#include <atomic>
#include <chrono>
#include <thread>

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/middleware/middleware.hpp>
#endif

namespace J = vix::json;

namespace vix::p2p_http
{
  class LogBuffer
  {
  public:
    explicit LogBuffer(std::size_t cap = 500) : cap_(cap) {}

    void push(std::string line)
    {
      std::lock_guard<std::mutex> lock(mu_);
      if (lines_.size() >= cap_)
        lines_.pop_front();
      lines_.push_back(std::move(line));
    }

    std::string dump() const
    {
      std::lock_guard<std::mutex> lock(mu_);
      std::ostringstream oss;
      for (const auto &l : lines_)
        oss << l << "\n";
      return oss.str();
    }

  private:
    std::size_t cap_;
    mutable std::mutex mu_;
    std::deque<std::string> lines_;
  };

  static LogBuffer g_logs{800};

  static std::atomic<bool> g_tick_started{false};
  static std::atomic<bool> g_tick_stop{false};

  static std::string stats_line_plain(const vix::p2p::RuntimeStats &st)
  {
    std::ostringstream oss;
    oss
        << "peers_total=" << st.peers_total
        << " peers_connected=" << st.peers_connected
        << " handshakes_started=" << st.handshakes_started
        << " handshakes_completed=" << st.handshakes_completed
        << " connect_attempts=" << st.connect.connect_attempts
        << " connect_deduped=" << st.connect.connect_deduped
        << " connect_failures=" << st.connect.connect_failures
        << " backoff_skips=" << st.connect.backoff_skips
        << " tracked_endpoints=" << st.connect.tracked_endpoints;
    return oss.str();
  }

  static std::string join_prefix(std::string base, std::string path)
  {
    if (!base.empty() && base.front() != '/')
      base.insert(base.begin(), '/');
    while (base.size() > 1 && base.back() == '/')
      base.pop_back();

    if (!path.empty() && path.front() != '/')
      path.insert(path.begin(), '/');
    while (path.size() > 1 && path.back() == '/')
      path.pop_back();

    if (base.empty())
      return path.empty() ? std::string{"/"} : path;

    if (path.empty() || path == "/")
      return base;

    return base + path;
  }

  // Fallback auth (no middleware): module-local.
  static bool legacy_auth_or_401(
      const P2PHttpOptions &opt,
      vix::vhttp::Request &req,
      vix::vhttp::ResponseWrapper &res)
  {
    if (!opt.auth_legacy)
    {
      res.status(401).json(J::obj({
          "ok",
          false,
          "error",
          "unauthorized",
          "hint",
          "auth required",
      }));
      return false;
    }

    return opt.auth_legacy(req, res);
  }

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
  // Route-level middleware install.
  static void install_route_middlewares(
      vix::App &app,
      const std::string &path,
      vix::p2p_http::RouteOptions ro,
      const P2PHttpOptions &opt)
  {
    using namespace vix::middleware::app;

    if (!ro.heavy && !ro.require_auth)
      return;

    // Auth hook (Context)
    auto auth_ctx = [opt](vix::mw::Context &ctx, vix::mw::Next next) mutable
    {
      if (!opt.auth_ctx)
      {
        ctx.res().status(401).json(J::obj({
            "ok",
            false,
            "error",
            "unauthorized",
            "hint",
            "auth required",
        }));
        return;
      }

      const bool ok = opt.auth_ctx(ctx);
      if (!ok)
        return;

      next();
    };

    // Heavy tag (Context)
    auto heavy_ctx = [](vix::mw::Context &ctx, vix::mw::Next next)
    {
      ctx.res().header("x-vix-route-heavy", "1");
      next();
    };

    if (ro.require_auth && ro.heavy)
    {
      install_exact(app, path, chain(adapt_ctx(auth_ctx), adapt_ctx(heavy_ctx)));
      return;
    }

    if (ro.require_auth)
    {
      install_exact(app, path, adapt_ctx(auth_ctx));
      return;
    }

    // heavy only
    install_exact(app, path, adapt_ctx(heavy_ctx));
  }
#endif

  // Public
  void registerRoutes(vix::App &app,
                      vix::p2p::P2PRuntime &runtime,
                      const P2PHttpOptions &opt)
  {
    const std::string base = (opt.prefix.empty() ? "/p2p" : opt.prefix);

    g_logs.push("[p2p_http] routes registered");

    if (opt.enable_live_logs && opt.enable_logs)
    {
      bool expected = false;
      if (g_tick_started.compare_exchange_strong(expected, true))
      {
        g_tick_stop.store(false);

        const int every = (opt.stats_every_ms <= 0 ? 1000 : opt.stats_every_ms);

        auto *rt = &runtime;

        std::thread(
            [rt, every]()
            {
          vix::p2p::RuntimeStats last{};
          while (!g_tick_stop.load())
          {
            const auto st = rt->runtime_stats();

            const bool changed =
              (st.peers_total != last.peers_total) ||
              (st.peers_connected != last.peers_connected) ||
              (st.handshakes_started != last.handshakes_started) ||
              (st.handshakes_completed != last.handshakes_completed) ||

              (st.connect.connect_attempts != last.connect.connect_attempts) ||
              (st.connect.connect_deduped != last.connect.connect_deduped) ||
              (st.connect.connect_failures != last.connect.connect_failures) ||
              (st.connect.backoff_skips != last.connect.backoff_skips) ||
              (st.connect.tracked_endpoints != last.connect.tracked_endpoints);

            if (changed)
            {
              g_logs.push(std::string("[p2p] ") + stats_line_plain(st));
              last = st;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(every));
          } })
            .detach();
      }
    }

    // GET /p2p/ping
    if (opt.enable_ping)
    {
      const std::string path = join_prefix(base, "/ping");

      app.get(path, [](vix::vhttp::Request &, vix::vhttp::ResponseWrapper &res)
              { res.json(J::obj({"ok", true,
                                 "pong", true,
                                 "module", "p2p_http"})); });

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
      {
        vix::p2p_http::RouteOptions ro;
        ro.heavy = false;
        ro.require_auth = false;
        install_route_middlewares(app, path, ro, opt);
      }
#endif
    }

    // GET /p2p/status
    if (opt.enable_status)
    {
      const std::string path = join_prefix(base, "/status");

      app.get(path, [&runtime](vix::vhttp::Request &, vix::vhttp::ResponseWrapper &res)
              {
        const auto st = runtime.runtime_stats();

        res.json(J::obj({
          "ok", true,
          "module", "p2p_http",

          "peers_total", (long long)st.peers_total,
          "peers_connected", (long long)st.peers_connected,
          "handshakes_started", (long long)st.handshakes_started,
          "handshakes_completed", (long long)st.handshakes_completed,

          "connect_attempts", (long long)st.connect.connect_attempts,
          "connect_deduped", (long long)st.connect.connect_deduped,
          "connect_failures", (long long)st.connect.connect_failures,
          "backoff_skips", (long long)st.connect.backoff_skips,
          "tracked_endpoints", (long long)st.connect.tracked_endpoints
        })); });

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
      {
        vix::p2p_http::RouteOptions ro;
        ro.heavy = false;
        ro.require_auth = false;
        install_route_middlewares(app, path, ro, opt);
      }
#endif
    }

    // GET /p2p/peers  (multi-peer view for dashboard)
    if (opt.enable_peers)
    {
      const std::string path = join_prefix(base, "/peers");

      app.get(path, [&runtime](vix::vhttp::Request &, vix::vhttp::ResponseWrapper &res)
              {
            auto node = runtime.node();
            if (!node)
            {
              res.status(503).json(J::obj({
                "ok", false,
                "error", "p2p_node_unavailable"
              }));
              return;
            }

            const auto snap = node->peers_snapshot();

            // Make output stable: sort by peer_id
            std::vector<std::pair<vix::p2p::PeerId, vix::p2p::Peer>> items;
            items.reserve(snap.size());
            for (const auto &kv : snap)
              items.push_back(kv);

            std::sort(items.begin(), items.end(),
                      [](const auto &a, const auto &b)
                      {
                        return a.first < b.first;
                      });

            auto state_to_string = [](vix::p2p::PeerState s) -> const char *
            {
              switch (s)
              {
              case vix::p2p::PeerState::Disconnected: return "disconnected";
              case vix::p2p::PeerState::Connecting:   return "connecting";
              case vix::p2p::PeerState::Handshaking:  return "handshaking";
              case vix::p2p::PeerState::Connected:    return "connected";
              case vix::p2p::PeerState::Stale:        return "stale";
              case vix::p2p::PeerState::Closed:       return "closed";
              default:                                return "unknown";
              }
            };

            auto hs_stage_to_string = [](vix::p2p::HandshakeState::Stage s) -> const char *
            {
              switch (s)
              {
              case vix::p2p::HandshakeState::Stage::None:          return "none";
              case vix::p2p::HandshakeState::Stage::HelloSent:     return "hello_sent";
              case vix::p2p::HandshakeState::Stage::HelloReceived: return "hello_received";
              case vix::p2p::HandshakeState::Stage::AckSent:       return "ack_sent";
              case vix::p2p::HandshakeState::Stage::AckReceived:   return "ack_received";
              case vix::p2p::HandshakeState::Stage::Finished:      return "finished";
              default:                                             return "unknown";
              }
            };

            auto endpoint_to_string = [](const std::optional<vix::p2p::PeerEndpoint> &ep) -> std::string
            {
              if (!ep)
                return "";

              const std::string scheme = (ep->scheme.empty() ? "tcp" : ep->scheme);
              return scheme + "://" + ep->host + ":" + std::to_string(ep->port);
            };

            const auto now = std::chrono::steady_clock::now();

            std::vector<J::token> peers_arr;
            peers_arr.reserve(items.size());

            for (const auto &[peer_id, p] : items)
            {
              const std::string ep_str = endpoint_to_string(p.endpoint);

              long long last_seen_ms_ago = -1;
              if (p.meta.last_seen.time_since_epoch().count() != 0)
              {
                const auto diff = now - p.meta.last_seen;
                last_seen_ms_ago =
                    (long long)std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
              }

              const bool secure = p.meta.secure;
              const long long public_key_len = (long long)p.meta.public_key.size();
              const long long session_key_len = (long long)p.meta.session_key_32.size();
              const long long capabilities_count = (long long)p.meta.capabilities.size();

              // Handshake block (optional)
              const bool has_hs = p.handshake.has_value();
              const char *hs_stage = "none";
              long long hs_age_ms = -1;
              long long hs_nonce_a = 0;
              long long hs_nonce_b = 0;
              long long hs_ts_ms   = 0;

              if (has_hs)
              {
                hs_stage = hs_stage_to_string(p.handshake->stage);

                if (p.handshake->started_at.time_since_epoch().count() != 0)
                {
                  const auto diff = now - p.handshake->started_at;
                  hs_age_ms =
                      (long long)std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
                }

                hs_nonce_a = (long long)p.handshake->nonce_a;
                hs_nonce_b = (long long)p.handshake->nonce_b;
                hs_ts_ms   = (long long)p.handshake->ts_ms;
              }

              // Endpoint split (optional)
              const bool has_ep = p.endpoint.has_value();
              const std::string ep_scheme = (has_ep ? (p.endpoint->scheme.empty() ? "tcp" : p.endpoint->scheme) : "");
              const std::string ep_host   = (has_ep ? p.endpoint->host : "");
              const long long ep_port      = (has_ep ? (long long)p.endpoint->port : 0);

              // Final peer object
              peers_arr.push_back(J::obj({
                "peer_id", peer_id,
                "state", state_to_string(p.state),

                "endpoint", ep_str,
                "has_endpoint", has_ep,
                "scheme", ep_scheme,
                "host", ep_host,
                "port", ep_port,

                "secure", secure,
                "capabilities_count", capabilities_count,
                "public_key_len", public_key_len,
                "session_key_len", session_key_len,

                "last_seen_ms_ago", (long long)last_seen_ms_ago,

                "has_handshake", has_hs,
                "handshake_stage", hs_stage,
                "handshake_age_ms", (long long)hs_age_ms,

                // debug-friendly (safe, no secrets)
                "nonce_a", (long long)hs_nonce_a,
                "nonce_b", (long long)hs_nonce_b,
                "ts_ms", (long long)hs_ts_ms
              }));
            }

            res.json(J::obj({
              "ok", true,
              "module", "p2p_http",
              "total", (long long)peers_arr.size(),
              "peers", J::array(std::move(peers_arr))
            })); });

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
      {
        vix::p2p_http::RouteOptions ro;
        ro.heavy = false;
        ro.require_auth = false;
        install_route_middlewares(app, path, ro, opt);
      }
#endif
    }

    // GET /p2p/logs
    if (opt.enable_logs)
    {
      const std::string path = join_prefix(base, "/logs");

      app.get(path, [](vix::vhttp::Request &, vix::vhttp::ResponseWrapper &res)
              {
               res.type("text/plain; charset=utf-8");
                res.text(g_logs.dump()); });

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
      {
        vix::p2p_http::RouteOptions ro;
        ro.heavy = false;
        ro.require_auth = false;
        install_route_middlewares(app, path, ro, opt);
      }
#endif
    }

    // POST /p2p/admin/hook (heavy + auth)
    {
      const std::string path = join_prefix(base, "/admin/hook");

      vix::p2p_http::RouteOptions ro;
      ro.heavy = true;
      ro.require_auth = true;

      // Copy opt into lambda safely (options object is cheap enough; holds std::function)
      const P2PHttpOptions opt_copy = opt;

      app.post(path, [opt_copy, ro](vix::vhttp::Request &req, vix::vhttp::ResponseWrapper &res) mutable
               {
#if !defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
        if (ro.require_auth)
        {
          if (!legacy_auth_or_401(opt_copy, req, res))
            return;
        }
        if (ro.heavy)
          res.header("x-vix-route-heavy", "1");
#else
        (void)req;
#endif

        res.status(501).json(J::obj({
          "ok", false,
          "status", 501,
          "error", "not_implemented",
          "message", "p2p_http: admin endpoint planned",
        })); });

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
      install_route_middlewares(app, path, ro, opt);
#endif
    }
  }

} // namespace vix::p2p_http
