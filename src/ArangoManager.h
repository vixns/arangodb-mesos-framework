////////////////////////////////////////////////////////////////////////////////
/// @brief manager for the ArangoDB framework
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2015 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2015, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGO_MANAGER_H
#define ARANGO_MANAGER_H 1

#include "Caretaker.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>

namespace arangodb {

// -----------------------------------------------------------------------------
// --SECTION--                                              class ReconcileTasks
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief helper class for reconciliation
////////////////////////////////////////////////////////////////////////////////

  class ReconcileTasks {
    public:
      std::string _taskId;
      std::string _slaveId;
      std::chrono::steady_clock::time_point _nextReconcile;
      std::chrono::steady_clock::duration _backoff;
  };

// -----------------------------------------------------------------------------
// --SECTION--                                               class ArangoManager
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief manager class
////////////////////////////////////////////////////////////////////////////////

  class ArangoManager {
    ArangoManager (const ArangoManager&) = delete;
    ArangoManager& operator= (const ArangoManager&) = delete;

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

      ArangoManager ();

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

      ~ArangoManager ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief checks and adds an offer
////////////////////////////////////////////////////////////////////////////////

      bool addOffer (const mesos::Offer&);

////////////////////////////////////////////////////////////////////////////////
/// @brief removes an offer
////////////////////////////////////////////////////////////////////////////////

      void removeOffer (const mesos::OfferID& offerId);

////////////////////////////////////////////////////////////////////////////////
/// @brief status update
////////////////////////////////////////////////////////////////////////////////

      void taskStatusUpdate (const mesos::TaskStatus& status);

////////////////////////////////////////////////////////////////////////////////
/// @brief destroys the cluster
////////////////////////////////////////////////////////////////////////////////

      void destroy ();
      void restart();
      void restartCluster();
      void restartStandalone();

////////////////////////////////////////////////////////////////////////////////
/// @brief endpoints for reading
////////////////////////////////////////////////////////////////////////////////

      std::vector<std::string> coordinatorEndpoints ();

////////////////////////////////////////////////////////////////////////////////
/// @brief endpoints for writing
////////////////////////////////////////////////////////////////////////////////

      std::vector<std::string> dbserverEndpoints ();

////////////////////////////////////////////////////////////////////////////////
/// @brief register newly started task
////////////////////////////////////////////////////////////////////////////////

      void registerNewTask (std::string tid, TaskType taskType, int pos) {
        _task2position[tid] = std::make_pair(taskType, pos);
      }

////////////////////////////////////////////////////////////////////////////////
/// @brief register a secondary server
////////////////////////////////////////////////////////////////////////////////
      bool registerNewSecondary(ArangoState::Lease&, TaskPlan*);

////////////////////////////////////////////////////////////////////////////////
/// @brief register a secondary server
////////////////////////////////////////////////////////////////////////////////
      bool registerNewSecondary(ArangoState::Lease&, std::string const&);

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief main dispatcher
////////////////////////////////////////////////////////////////////////////////

      void dispatch ();

////////////////////////////////////////////////////////////////////////////////
/// @brief prepares the reconciliation of tasks
////////////////////////////////////////////////////////////////////////////////

      void prepareReconciliation ();

////////////////////////////////////////////////////////////////////////////////
/// @brief tries to recover tasks
////////////////////////////////////////////////////////////////////////////////

      void reconcileTasks ();

////////////////////////////////////////////////////////////////////////////////
/// @brief checks for timeout
////////////////////////////////////////////////////////////////////////////////

      bool checkTimeouts ();

////////////////////////////////////////////////////////////////////////////////
/// @brief applies status updates
////////////////////////////////////////////////////////////////////////////////

      void applyStatusUpdates(std::vector<std::string>&);

////////////////////////////////////////////////////////////////////////////////
/// @brief update target
////////////////////////////////////////////////////////////////////////////////

      std::vector<std::string> updateTarget();

////////////////////////////////////////////////////////////////////////////////
/// @brief update target
////////////////////////////////////////////////////////////////////////////////
      void updatePlan(std::vector<std::string> const& cleanedServers);

////////////////////////////////////////////////////////////////////////////////
/// @brief updates server ids
////////////////////////////////////////////////////////////////////////////////

      void updateServerIds();

////////////////////////////////////////////////////////////////////////////////
/// @brief checks available offers
////////////////////////////////////////////////////////////////////////////////

      void checkOutstandOffers ();

////////////////////////////////////////////////////////////////////////////////
/// @brief starts new instances
////////////////////////////////////////////////////////////////////////////////

      bool startNewInstances ();

////////////////////////////////////////////////////////////////////////////////
/// @brief recover task mapping
////////////////////////////////////////////////////////////////////////////////

      void fillKnownInstances (TaskType, const TasksCurrent&);

////////////////////////////////////////////////////////////////////////////////
/// @brief kills all running tasks
////////////////////////////////////////////////////////////////////////////////

      void killAllInstances (std::vector<std::string>&);
      
      void manageClusterRestart();
      bool taskIsGoneOrRestarted(ArangoState::Lease&, TaskType const&, std::string const&);

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief stop flag
////////////////////////////////////////////////////////////////////////////////

      std::atomic<bool> _stopDispatcher;

////////////////////////////////////////////////////////////////////////////////
/// @brief main dispatcher thread
////////////////////////////////////////////////////////////////////////////////

      std::thread* _dispatcher;

////////////////////////////////////////////////////////////////////////////////
/// @brief next implicit reconciliation
////////////////////////////////////////////////////////////////////////////////

      std::chrono::steady_clock::time_point _nextImplicitReconciliation;

////////////////////////////////////////////////////////////////////////////////
/// @brief implicit reconciliation intervall
////////////////////////////////////////////////////////////////////////////////

      std::chrono::steady_clock::duration _implicitReconciliationIntervall;

////////////////////////////////////////////////////////////////////////////////
/// @brief maximal reconciliation intervall
////////////////////////////////////////////////////////////////////////////////

      std::chrono::steady_clock::duration _maxReconcileIntervall;

////////////////////////////////////////////////////////////////////////////////
/// @brief tasks to reconcile
////////////////////////////////////////////////////////////////////////////////

      std::unordered_map<std::string, ReconcileTasks> _reconciliationTasks;

////////////////////////////////////////////////////////////////////////////////
/// @brief positions of tasks
////////////////////////////////////////////////////////////////////////////////

      std::unordered_map<std::string, std::pair<TaskType, int>> _task2position;

////////////////////////////////////////////////////////////////////////////////
/// @brief protects _taskStatusUpdates and _storedOffers
////////////////////////////////////////////////////////////////////////////////

      std::mutex _lock;

////////////////////////////////////////////////////////////////////////////////
/// @brief offers received
////////////////////////////////////////////////////////////////////////////////

      std::unordered_map<std::string, mesos::Offer> _storedOffers;

////////////////////////////////////////////////////////////////////////////////
/// @brief status updates received
////////////////////////////////////////////////////////////////////////////////

      std::vector<mesos::TaskStatus> _taskStatusUpdates;
  };
}

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------
