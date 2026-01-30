/**
 *
 *  @file P2PHttpOptions.hpp
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
#ifndef VIX_P2P_HTTP_OPTIONS_HPP
#define VIX_P2P_HTTP_OPTIONS_HPP

#include <functional>
#include <string>

#include <vix/http/RequestHandler.hpp>
#include <vix/mw/context.hpp>

namespace vix::p2p_http
{
  using AuthHookCtx = std::function<bool(vix::mw::Context &)>;

  using AuthHookLegacy = std::function<bool(vix::vhttp::Request &, vix::vhttp::ResponseWrapper &)>;

  struct P2PHttpOptions
  {
    std::string prefix = "/p2p";

    bool enable_ping = true;
    bool enable_status = true;
    bool enable_logs = true;

    AuthHookCtx auth_ctx = nullptr;
    AuthHookLegacy auth_legacy = nullptr;
  };

}

#endif
