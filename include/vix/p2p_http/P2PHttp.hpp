/**
 *
 *  @file P2PHttp.hpp
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
#ifndef VIX_P2P_HTTP_HPP
#define VIX_P2P_HTTP_HPP

#include <vix/p2p_http/P2PHttpOptions.hpp>

namespace vix
{
  class App;
}

namespace vix::p2p
{
  class P2PRuntime;
}

namespace vix::p2p_http
{
  /**
   * @brief Register P2P HTTP control routes on the application.
   *
   * Exposes endpoints such as ping, status, peers, and logs under the
   * configured prefix. Routes interact with the P2P runtime and may
   * be protected by authentication hooks.
   *
   * @param app Application instance used to register routes.
   * @param runtime Active P2P runtime.
   * @param opt Configuration options controlling exposed endpoints.
   */
  void registerRoutes(
      vix::App &app,
      vix::p2p::P2PRuntime &runtime,
      const P2PHttpOptions &opt);

  /**
   * @brief Stop live log streaming and release related resources.
   */
  void shutdown_live_logs();

  /**
   * @brief Set a sink used to forward live log lines.
   *
   * This is typically used to stream logs over HTTP or SSE.
   *
   * @param sink Callback receiving log lines.
   */
  void set_live_log_sink(std::function<void(std::string)> sink);
}

#endif // VIX_P2P_HTTP_HPP
