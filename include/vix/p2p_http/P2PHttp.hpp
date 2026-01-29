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
  void registerRoutes(
      vix::App &app,
      vix::p2p::P2PRuntime &runtime,
      const P2PHttpOptions &opt);
}

#endif // VIX_P2P_HTTP_HPP
