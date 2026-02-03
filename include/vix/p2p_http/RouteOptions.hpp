/**
 *
 *  @file RouteOptions.hpp
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
#ifndef VIX_P2P_HTTP_ROUTE_OPTIONS_HPP
#define VIX_P2P_HTTP_ROUTE_OPTIONS_HPP

namespace vix::p2p_http
{
  /**
   * @brief Per-route configuration flags for P2P HTTP endpoints.
   *
   * Used to annotate routes with execution and security hints,
   * typically consumed by the HTTP routing or middleware layer.
   */
  struct RouteOptions
  {
    /** @brief Marks the route as heavy or resource-intensive. */
    bool heavy = false;

    /** @brief Require authentication before executing the route. */
    bool require_auth = false;
  };
}

#endif // VIX_P2P_HTTP_ROUTE_OPTIONS_HPP
