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

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>

// -----------------------------------------------------------------------------
// --SECTION--                                             class ArangoScheduler
// -----------------------------------------------------------------------------

namespace arangodb {
  using namespace mesos;
  using namespace std;

  class ArangoManager;

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

      ArangoScheduler (const string& _role,
                       const ExecutorInfo&);

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

      virtual ~ArangoScheduler ();

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the manager
////////////////////////////////////////////////////////////////////////////////

      ArangoManager* manager () const;

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the driver
////////////////////////////////////////////////////////////////////////////////

      void setDriver (SchedulerDriver*);

////////////////////////////////////////////////////////////////////////////////
/// @brief makes a dynamic reservation
////////////////////////////////////////////////////////////////////////////////

      void reserveDynamically (const Offer&, const Resources&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a persistent disk
////////////////////////////////////////////////////////////////////////////////

      void makePersistent (const Offer&, const Resources&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief declines an offer
////////////////////////////////////////////////////////////////////////////////

      void declineOffer (const OfferID&) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief starts an agency with a given offer
////////////////////////////////////////////////////////////////////////////////

      void startInstance (const string& taskId,
                          const string& name,
                          const Offer&,
                          const Resources&,
                          const string& arguments) const;

////////////////////////////////////////////////////////////////////////////////
/// @brief kills an instances
////////////////////////////////////////////////////////////////////////////////

      void killInstance (const string& name,
                         const string& taskId) const;

// -----------------------------------------------------------------------------
// --SECTION--                                                 Scheduler methods
// -----------------------------------------------------------------------------

    public:

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been register
////////////////////////////////////////////////////////////////////////////////

      void registered (SchedulerDriver*,
                       const FrameworkID&,
                       const MasterInfo&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been re-register
////////////////////////////////////////////////////////////////////////////////

      void reregistered (SchedulerDriver*,
                         const MasterInfo&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been disconnected
////////////////////////////////////////////////////////////////////////////////

      void disconnected (SchedulerDriver*) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when new resources are available
////////////////////////////////////////////////////////////////////////////////

      void resourceOffers (SchedulerDriver*,
                           const vector<Offer>&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when new resources become unavailable
////////////////////////////////////////////////////////////////////////////////

      void offerRescinded (SchedulerDriver*,
                           const OfferID&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when task changes
////////////////////////////////////////////////////////////////////////////////

      void statusUpdate (SchedulerDriver*,
                         const TaskStatus&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for messages from executor
////////////////////////////////////////////////////////////////////////////////

      void frameworkMessage (SchedulerDriver*,
                             const ExecutorID&,
                             const SlaveID&,
                             const string& data) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for slave is down
////////////////////////////////////////////////////////////////////////////////

      void slaveLost (SchedulerDriver*,
                      const SlaveID&) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for executor goes down
////////////////////////////////////////////////////////////////////////////////

      void executorLost (SchedulerDriver*,
                         const ExecutorID&,
                         const SlaveID&,
                         int status) override;

////////////////////////////////////////////////////////////////////////////////
/// @brief error handling
////////////////////////////////////////////////////////////////////////////////

      void error (SchedulerDriver*,
                  const string& message) override;

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

    private:

////////////////////////////////////////////////////////////////////////////////
/// @brief role
////////////////////////////////////////////////////////////////////////////////

      string _role;

////////////////////////////////////////////////////////////////////////////////
/// @brief scheduler driver
////////////////////////////////////////////////////////////////////////////////

      SchedulerDriver* _driver;

////////////////////////////////////////////////////////////////////////////////
/// @brief executor info
////////////////////////////////////////////////////////////////////////////////

      const ExecutorInfo _executor;

////////////////////////////////////////////////////////////////////////////////
/// @brief ArangoDB manager
////////////////////////////////////////////////////////////////////////////////

      ArangoManager* _manager;
  };
}

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
