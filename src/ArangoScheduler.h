////////////////////////////////////////////////////////////////////////////////
/// @brief scheduler for the ArangoDB framework
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

#ifndef ARANGO_SCHEDULER_H
#define ARANGO_SCHEDULER_H 1

#include "arangodb.pb.h"

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>

// -----------------------------------------------------------------------------
// --SECTION--                                             class ArangoScheduler
// -----------------------------------------------------------------------------

namespace arangodb {

////////////////////////////////////////////////////////////////////////////////
/// @brief scheduler class
////////////////////////////////////////////////////////////////////////////////

  class ArangoScheduler : public mesos::Scheduler {

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

      ArangoScheduler ();

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

      virtual ~ArangoScheduler ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the driver
////////////////////////////////////////////////////////////////////////////////

      void setDriver (mesos::SchedulerDriver*);

////////////////////////////////////////////////////////////////////////////////
/// @brief makes a dynamic reservation
////////////////////////////////////////////////////////////////////////////////

      void reserveDynamically (mesos::Offer const&,
                               mesos::Resources const&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief cancels a dynamic reservation
////////////////////////////////////////////////////////////////////////////////

      void unreserveDynamically (mesos::Offer const&,
                                 mesos::Resources const&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a persistent disk
////////////////////////////////////////////////////////////////////////////////

      void makePersistent (mesos::Offer const&,
                           mesos::Resources const&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief destroys persistent disks
////////////////////////////////////////////////////////////////////////////////

      void destroyPersistent (const mesos::Offer& offer,
                              const mesos::Resources& resources) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief declines an offer
////////////////////////////////////////////////////////////////////////////////

      void declineOffer (mesos::OfferID const&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief starts an agency with a given offer
////////////////////////////////////////////////////////////////////////////////

      mesos::TaskInfo startInstance (std::string const& taskId,
                                     std::string const& name,
                                     TaskCurrent const&,
                                     mesos::ContainerInfo const&,
                                     mesos::CommandInfo const&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief kills an instances
////////////////////////////////////////////////////////////////////////////////

      void killInstance (std::string const& taskId) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief posts an request to the master
////////////////////////////////////////////////////////////////////////////////

      std::string postRequest (std::string const& command,
                               std::string const& body) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief stops the driver
////////////////////////////////////////////////////////////////////////////////

      void stop ();

////////////////////////////////////////////////////////////////////////////////
/// @brief reconciles all tasks
////////////////////////////////////////////////////////////////////////////////

      void reconcileTasks ();

////////////////////////////////////////////////////////////////////////////////
/// @brief reconciles a single task
////////////////////////////////////////////////////////////////////////////////

      void reconcileTask (const mesos::TaskStatus&);

// -----------------------------------------------------------------------------
// --SECTION--                                                 Scheduler methods
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been registered
////////////////////////////////////////////////////////////////////////////////

      void registered (mesos::SchedulerDriver*,
                       const mesos::FrameworkID&,
                       const mesos::MasterInfo&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been re-registered
////////////////////////////////////////////////////////////////////////////////

      void reregistered (mesos::SchedulerDriver*,
                         const mesos::MasterInfo&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been disconnected
////////////////////////////////////////////////////////////////////////////////

      void disconnected (mesos::SchedulerDriver*) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when new resources are available
////////////////////////////////////////////////////////////////////////////////

      void resourceOffers (mesos::SchedulerDriver*,
                           const std::vector<mesos::Offer>&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when new resources become unavailable
////////////////////////////////////////////////////////////////////////////////

      void offerRescinded (mesos::SchedulerDriver*,
                           const mesos::OfferID&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when task changes
////////////////////////////////////////////////////////////////////////////////

      void statusUpdate (mesos::SchedulerDriver*,
                         const mesos::TaskStatus&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for messages from executor
////////////////////////////////////////////////////////////////////////////////

      void frameworkMessage (mesos::SchedulerDriver*,
                             const mesos::ExecutorID&,
                             const mesos::SlaveID&,
                             const std::string& data) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for slave is down
////////////////////////////////////////////////////////////////////////////////

      void slaveLost (mesos::SchedulerDriver*,
                      const mesos::SlaveID&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for executor goes down
////////////////////////////////////////////////////////////////////////////////

      void executorLost (mesos::SchedulerDriver*,
                         const mesos::ExecutorID&,
                         const mesos::SlaveID&,
                         int status) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief error handling
////////////////////////////////////////////////////////////////////////////////

      void error (mesos::SchedulerDriver*,
                  const std::string& message) override;

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief scheduler driver
////////////////////////////////////////////////////////////////////////////////

      mesos::SchedulerDriver* _driver;
  };
}

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------
