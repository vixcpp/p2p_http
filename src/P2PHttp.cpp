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

#include <string>
#include <utility>

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/app/presets.hpp>
#include <vix/middleware/middleware.hpp>
#endif

namespace J = vix::json;

namespace vix::p2p_http
{
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
    (void)runtime;

    const std::string base = (opt.prefix.empty() ? "/p2p" : opt.prefix);

    // GET /p2p/ping
    if (opt.enable_ping)
    {
      const std::string path = join_prefix(base, "/ping");

      app.get(path, [](vix::vhttp::Request &, vix::vhttp::ResponseWrapper &res)
              { res.json(J::obj({
                    "ok",
                    true,
                    "pong",
                    true,
                    "module",
                    "p2p_http",
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

    // GET /p2p/status
    if (opt.enable_status)
    {
      const std::string path = join_prefix(base, "/status");

      app.get(path, [](vix::vhttp::Request &, vix::vhttp::ResponseWrapper &res)
              { res.json(J::obj({
                    "ok",
                    true,
                    "status",
                    "ok",
                    "module",
                    "p2p_http",
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
