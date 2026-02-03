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
  /** @brief Authentication hook using middleware context (preferred). */
  using AuthHookCtx = std::function<bool(vix::mw::Context &)>;

  /** @brief Legacy authentication hook using raw HTTP request/response. */
  using AuthHookLegacy = std::function<bool(vix::vhttp::Request &, vix::vhttp::ResponseWrapper &)>;

  /** @brief Log sink callback for P2P HTTP events. */
  using LogSink = std::function<void(std::string_view)>;

  /**
   * @brief Configuration options for the P2P HTTP control endpoints.
   *
   * Controls which endpoints are exposed under a given prefix and how
   * authentication, logging, and statistics are handled.
   */
  struct P2PHttpOptions
  {
    /** @brief Base URL prefix for P2P HTTP endpoints. */
    std::string prefix = "/p2p";

    /** @brief Enable /ping endpoint. */
    bool enable_ping = true;

    /** @brief Enable /status endpoint. */
    bool enable_status = true;

    /** @brief Enable static logs endpoint. */
    bool enable_logs = true;

    /** @brief Enable live log streaming endpoint. */
    bool enable_live_logs = true;

    /** @brief Statistics emission interval in milliseconds. */
    int stats_every_ms = 1000;

    /** @brief Enable peers listing endpoint. */
    bool enable_peers{true};

    /** @brief Authentication hook using middleware context. */
    AuthHookCtx auth_ctx = nullptr;

    /** @brief Legacy authentication hook (raw HTTP). */
    AuthHookLegacy auth_legacy = nullptr;

    /** @brief Optional sink for forwarding log lines. */
    LogSink log_sink = nullptr;
  };

} // namespace vix::p2p_http

#endif // VIX_P2P_HTTP_OPTIONS_HPP
