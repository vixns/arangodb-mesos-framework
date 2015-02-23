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

#include "ArangoScheduler.h"

#include <boost/lexical_cast.hpp>

#include <atomic>
#include <iostream>
#include <string>

#include <mesos/resources.hpp>

#include "ArangoManager.h"

using namespace std;
using namespace boost;
using namespace mesos;
using namespace arangodb;

// -----------------------------------------------------------------------------
// --SECTION--                                                   local variables
// -----------------------------------------------------------------------------

namespace {
  atomic<uint64_t> NEXT_TASK_ID(1);
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  helper functions
// -----------------------------------------------------------------------------

namespace {
  std::vector<std::string> split (const std::string& value, char separator) {
    vector<std::string> result;
    string::size_type p = 0;
    string::size_type q;

    while ((q = value.find(separator, p)) != std::string::npos) {
      result.emplace_back(value, p, q - p);
      p = q + 1;
    }

    result.emplace_back(value, p);
    return result;
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                             class ArangoScheduler
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

ArangoScheduler::ArangoScheduler (const string& role,
                                  const ExecutorInfo& executor)
  : _role(role),
    _driver(nullptr),
    _executor(executor),
    _manager(nullptr) {
  _manager = new ArangoManager(_role, this);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

ArangoScheduler::~ArangoScheduler () {
  delete _manager;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the manager
////////////////////////////////////////////////////////////////////////////////

ArangoManager* ArangoScheduler::manager () const {
  return _manager;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief sets the driver
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::setDriver (SchedulerDriver* driver) {
  _driver = driver;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief makes a dynamic reservation
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::reserveDynamically (const Offer& offer,
                                          const Resources& resources) const {
  Offer::Operation reserve;
  reserve.set_type(Offer::Operation::RESERVE);
  reserve.mutable_reserve()->mutable_resources()->CopyFrom(resources);

  _driver->acceptOffers({offer.id()}, {reserve});
}

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a persistent disk
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::makePersistent (const Offer& offer,
                                      const Resources& resources) const {
  Offer::Operation reserve;
  reserve.set_type(Offer::Operation::CREATE);
  reserve.mutable_create()->mutable_volumes()->CopyFrom(resources);

  _driver->acceptOffers({offer.id()}, {reserve});
}

////////////////////////////////////////////////////////////////////////////////
/// @brief declines an offer
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::declineOffer (const OfferID& offerId) const {
  LOG(INFO)
  << "DEBUG declining offer " << offerId.value();

  _driver->declineOffer(offerId);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief starts an agency with a given offer
////////////////////////////////////////////////////////////////////////////////

uint64_t ArangoScheduler::startAgencyInstance (const Offer& offer,
                                               const Resources& resources) const {
  uint64_t taskId = NEXT_TASK_ID.fetch_add(1);
  string id = "arangodb:agency:" + lexical_cast<string>(taskId);
  const string offerId = offer.id().value();

  LOG(INFO)
  << "AGENCY launching task " << taskId 
  << " using offer " << offerId 
  << ": " << offer.resources()
  << " and using resources " << resources;

  TaskInfo task;

  task.set_name(id);
  task.mutable_task_id()->set_value(id);
  task.mutable_slave_id()->CopyFrom(offer.slave_id());
  task.mutable_executor()->CopyFrom(_executor);
  task.mutable_resources()->CopyFrom(resources);

  vector<TaskInfo> tasks;
  tasks.push_back(task);

  _driver->launchTasks(offer.id(), tasks);

  return taskId;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                 Scheduler methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been register
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::registered (SchedulerDriver*,
                                  const FrameworkID&,
                                  const MasterInfo&) {
  // TODO(fc) what to do?
  LOG(INFO) << "DEBUG Registered!";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been re-register
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::reregistered (SchedulerDriver*,
                                    const MasterInfo& masterInfo) {
  // TODO(fc) what to do?
  LOG(INFO) << "DEBUG Re-Registered!";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when scheduler has been disconnected
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::disconnected (SchedulerDriver* driver) {
  // TODO(fc) what to do?
  LOG(INFO) << "DEBUG Disconnected!";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when new resources are available
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::resourceOffers (SchedulerDriver* driver,
                                      const vector<Offer>& offers) {
  for (auto& offer : offers) {
    LOG(INFO)
    << "DEBUG offer received " << offer.id().value();

    _manager->addOffer(offer);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when new resources becomes unavailable
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::offerRescinded (SchedulerDriver* driver,
                                      const OfferID& offerId) {
  LOG(INFO)
  << "DEBUG offer rescinded " << offerId.value();

  _manager->removeOffer(offerId);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback when task changes
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::statusUpdate (SchedulerDriver* driver,
                                    const TaskStatus& status) {
  vector<string> taskId = split(status.task_id().value(), ':');

  if (taskId.size() != 3) {
    LOG(ERROR)
    << "corrupt task id '"
    << status.task_id().value() << "' received";
    return;
  }

  uint64_t task = lexical_cast<uint64_t>(taskId[2]);
  auto state = status.state();

  LOG(INFO)
  << "TASK '" << status.task_id().value()
  << "' is in state " << state
  << " with reason " << status.reason()
  << " from source " << status.source()
  << " with message '" << status.message() << "'";

  switch (state) {
    case TASK_STAGING:
      break;

    case TASK_RUNNING:
      _manager->statusUpdate(task, InstanceState::RUNNING);
      break;

    case TASK_STARTING:
      // do nothing
      break;

    case TASK_FINISHED: // TERMINAL. The task finished successfully.
    case TASK_FAILED:   // TERMINAL. The task failed to finish successfully.
    case TASK_KILLED:   // TERMINAL. The task was killed by the executor.
    case TASK_LOST:     // TERMINAL. The task failed but can be rescheduled.
    case TASK_ERROR:    // TERMINAL. The task failed but can be rescheduled.
      _manager->statusUpdate(task, InstanceState::FINISHED);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for messages from executor
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::frameworkMessage (SchedulerDriver* driver,
                                        const ExecutorID& executorId,
                                        const SlaveID& slaveId,
                                        const string& data) {
  // TODO(fc) what to do?
  LOG(INFO) << "Framework Message!";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for slave is down
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::slaveLost (SchedulerDriver* driver,
                                 const SlaveID& sid) {
  // TODO(fc) what to do?
  LOG(INFO) << "Slave Lost!";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief callback for executor goes down
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::executorLost (SchedulerDriver* driver,
                                    const ExecutorID& executorID,
                                    const SlaveID& slaveID,
                                    int status) {
  // TODO(fc) what to do?
  LOG(INFO) << "Executor Lost!";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief error handling
////////////////////////////////////////////////////////////////////////////////

void ArangoScheduler::error (SchedulerDriver* driver,
                             const string& message) {
  // TODO(fc) what to do?
  LOG(ERROR) << "ERROR " << message;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End: