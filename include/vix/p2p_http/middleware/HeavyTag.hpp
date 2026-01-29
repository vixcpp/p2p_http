/**
 *
 *  @file HeavyTag.hpp
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
#ifndef VIX_P2P_HTTP_MW_HEAVY_TAG_HPP
#define VIX_P2P_HTTP_MW_HEAVY_TAG_HPP

#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)
#include <vix/middleware/Context.hpp>
#endif

namespace vix::p2p_http::mw
{
#if defined(VIX_P2P_HTTP_WITH_MIDDLEWARE)

  inline vix::middleware::MiddlewareFn heavy_tag()
  {
    return [](vix::middleware::Context &ctx, vix::middleware::Next next)
    {
      ctx.res().header("x-vix-route-heavy", "1");
      next();
    };
  }

#endif
}

#endif
