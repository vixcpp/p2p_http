/**
 *
 *  @file AuthHook.hpp
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
#ifndef VIX_P2P_HTTP_MW_AUTH_HOOK_HPP
#define VIX_P2P_HTTP_MW_AUTH_HOOK_HPP

#include <vix/p2p_http/P2PHttpOptions.hpp>

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
#include <vix/middleware/app/adapter.hpp>
#include <vix/middleware/Context.hpp>
#endif

namespace vix::p2p_http::mw
{
#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)

  /**
   * @brief Build a middleware authentication hook for P2P HTTP routes.
   *
   * The middleware delegates authentication to the AuthHookCtx provided
   * in P2PHttpOptions. If no hook is configured, the request is rejected
   * with an HTTP 401 response.
   *
   * @param opt P2P HTTP options containing the authentication hook.
   * @return Middleware function enforcing authentication.
   */
  inline vix::middleware::MiddlewareFn auth_hook(P2PHttpOptions opt)
  {
    return [opt = std::move(opt)](vix::middleware::Context &ctx, vix::middleware::Next next) mutable
    {
      if (!opt.auth_ctx)
      {
        ctx.res().status(401).json(vix::json::obj({
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
  }

#endif
} // namespace vix::p2p_http::mw

#endif // VIX_P2P_HTTP_MW_AUTH_HOOK_HPP
