/**
 *
 *  @file p2p_http/p2p_http.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Internal aggregation header for the Vix P2P HTTP module.
 *
 *  This file includes all components related to the P2P HTTP layer,
 *  including route registration, options, and middleware.
 *
 *  For most use cases, prefer:
 *    #include <vix/p2p_http.hpp>
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_P2P_HTTP_MODULE_HPP
#define VIX_P2P_HTTP_MODULE_HPP

#include <vix/p2p_http/P2PHttp.hpp>
#include <vix/p2p_http/P2PHttpOptions.hpp>
#include <vix/p2p_http/RouteOptions.hpp>

// middleware
#include <vix/p2p_http/middleware/AuthHook.hpp>
#include <vix/p2p_http/middleware/HeavyTag.hpp>

#endif // VIX_P2P_HTTP_MODULE_HPP
