/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
#define KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP

#include <memory>

#include <boost/asio/io_context.hpp>

#include "clock/clock.hpp"

namespace soralog {
  class LoggingSystem;
}

namespace kagome {
  namespace application {
    class AppConfiguration;
    class ChainSpec;
    class AppStateManager;
  }  // namespace application

  namespace application::mode {
    class RecoveryMode;
  }

  namespace metrics {
    class Exposer;
    class MetricsWatcher;
  }  // namespace metrics

  namespace network {
    class Router;
    class PeerManager;
    class SyncProtocolObserver;
  }  // namespace network

  namespace api {
    class ApiService;
  }

  namespace consensus::babe {
    class Babe;
  }

  namespace consensus::grandpa {
    class Grandpa;
  }

  namespace blockchain {
    class BlockStorage;
  }

  namespace storage::trie {
    class TrieStorage;
  }

  namespace telemetry {
    class TelemetryService;
  }

}  // namespace kagome

namespace kagome::injector {

  /**
   * Dependency injector for a universal node. Provides all major components
   * required by the kagome application.
   */
  class KagomeNodeInjector final {
   public:
    explicit KagomeNodeInjector(const application::AppConfiguration &);

    std::shared_ptr<application::ChainSpec> injectChainSpec();
    std::shared_ptr<blockchain::BlockStorage> injectBlockStorage();
    std::shared_ptr<application::AppStateManager> injectAppStateManager();
    std::shared_ptr<boost::asio::io_context> injectIoContext();
    std::shared_ptr<metrics::Exposer> injectOpenMetricsService();
    std::shared_ptr<network::Router> injectRouter();
    std::shared_ptr<network::PeerManager> injectPeerManager();
    std::shared_ptr<api::ApiService> injectRpcApiService();
    std::shared_ptr<clock::SystemClock> injectSystemClock();
    std::shared_ptr<consensus::babe::Babe> injectBabe();
    std::shared_ptr<network::SyncProtocolObserver> injectSyncObserver();
    std::shared_ptr<consensus::grandpa::Grandpa> injectGrandpa();
    std::shared_ptr<soralog::LoggingSystem> injectLoggingSystem();
    std::shared_ptr<storage::trie::TrieStorage> injectTrieStorage();
    std::shared_ptr<metrics::MetricsWatcher> injectMetricsWatcher();
    std::shared_ptr<telemetry::TelemetryService> injectTelemetryService();

    std::shared_ptr<application::mode::RecoveryMode> injectRecoveryMode();

   protected:
    std::shared_ptr<class KagomeNodeInjectorImpl> pimpl_;
  };

}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_APPLICATION_INJECTOR_HPP
