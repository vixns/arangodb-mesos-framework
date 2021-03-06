////////////////////////////////////////////////////////////////////////////////
/// @brief ArangoDB Mesos Framework
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

#include <libgen.h>

#include <csignal>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include "ArangoManager.h"
#include "ArangoScheduler.h"
#include "ArangoState.h"
#include "CaretakerStandalone.h"
#include "CaretakerCluster.h"
#include "Global.h"
#include "HttpServer.h"

#include <stout/check.hpp>
#include <stout/exit.hpp>
#include <stout/flags.hpp>
#include <stout/numify.hpp>
#include <stout/os.hpp>
#include <stout/stringify.hpp>
#include <stout/net.hpp>

#include "logging/flags.hpp"
#include "logging/logging.hpp"

using namespace std;
using namespace mesos::internal;
using namespace arangodb;

// -----------------------------------------------------------------------------
// --SECTION--                                                 private functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief update from env
////////////////////////////////////////////////////////////////////////////////
static void updateFromEnv (const string& name, string& var) {
  Option<string> env = os::getenv(name);

  if (env.isSome()) {
    var = env.get();
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief update from env
////////////////////////////////////////////////////////////////////////////////

static void updateFromEnv (const string& name, int& var) {
  Option<string> env = os::getenv(name);

  if (env.isSome()) {
    var = atoi(env.get().c_str());
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief update from env
////////////////////////////////////////////////////////////////////////////////

static void updateFromEnv (const string& name, double& var) {
  Option<string> env = os::getenv(name);

  if (env.isSome()) {
    var = atof(env.get().c_str());
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief prints help
////////////////////////////////////////////////////////////////////////////////

static void usage (const string& argv0, const flags::FlagsBase& flags) {
  cerr << "Usage: " << argv0 << " [...]" << "\n"
       << "\n"
       << "Supported options:" << "\n"
       << flags.usage() << "\n"
       << "Supported environment:" << "\n"
       << "  ARANGODB_MODE        overrides '--mode'\n"
       << "  ARANGODB_ASYNC_REPLICATION\n"
          "                       overrides '--async_replication'\n"
       << "  ARANGODB_ROLE        overrides '--role'\n"
       << "  ARANGODB_MINIMAL_RESOURCES_AGENT\n"
       << "                       overrides '--minimal_resources_agent'\n"
       << "  ARANGODB_MINIMAL_RESOURCES_DBSERVER\n"
          "                       overrides '--minimal_resources_dbserver'\n"
       << "  ARANGODB_MINIMAL_RESOURCES_SECONDARY\n"
          "                       overrides '--minimal_resources_secondary'\n"
       << "  ARANGODB_MINIMAL_RESOURCES_COORDINATOR\n"
          "                       overrides '--minimal_resources_coordinator'\n"
       << "  ARANGODB_NR_AGENTS   overrides '--nr_agents'\n"
       << "  ARANGODB_NR_DBSERVERS\n"
       << "                       overrides '--nr_dbservers'\n"
       << "  ARANGODB_NR_COORDINATORS\n"
       << "                       overrides '--nr_coordinators'\n"
       << "  ARANGODB_PRINCIPAL   overrides '--principal'\n"
       << "  ARANGODB_USER        overrides '--user'\n"
       << "  ARANGODB_FRAMEWORK_NAME\n"
       << "                       overrides '--framework_name'\n"
       << "  ARANGODB_FRAMEWORK_PORT\n"
       << "                       overrides '--framework_port'\n"
       << "  ARANGODB_WEBUI       overrides '--webui'\n"
       << "  ARANGODB_WEBUI_PORT  overrides '--webui_port'\n"
       << "  ARANGODB_FAILOVER_TIMEOUT\n"
       << "                       overrides '--failover_timeout'\n"
       << "  ARANGODB_SECONDARIES_WITH_DBSERVERS\n"
       << "                       overrides '--secondaries_with_dbservers'\n"
       << "  ARANGODB_COORDINATORS_WITH_DBSERVERS\n"
       << "                       overrides '--coordinators_with_dbservers'\n"
       << "  ARANGODB_IMAGE       overrides '--arangodb_image'\n"
       << "  ARANGODB_PRIVILEGED_IMAGE\n"
       << "                       overrides '--arangodb_privileged_image'\n"
       << "  ARANGODB_JWT_SECRET\n"
       << "                       overrides '--arangodb_jwt_secret\n"
       << "  ARANGODB_SSL_KEYFILE\n"
       << "                       overrides '--arangodb_ssl_keyfile\n"
       << "  ARANGODB_ENTERPRISE_KEY\n"
       << "                       overrides '--arangodb_enterprise_key'\n"
       << "  ARANGODB_STORAGE_ENGINE\n"
       << "                       overrides '--arangodb_storage_engine\n"
       << "  ARANGODB_ADDITIONAL_AGENT_ARGS\n"
       << "                       overrides '--arangodb_additional_agent_args'\n"
       << "  ARANGODB_ADDITIONAL_DBSERVER_ARGS\n"
       << "                       overrides '--arangodb_additional_dbserver_args'\n"
       << "  ARANGODB_ADDITIONAL_SECONDARY_ARGS\n"
       << "                       overrides '--arangodb_additional_secondary_args'\n"
       << "  ARANGODB_ADDITIONAL_COORDINATOR_ARGS\n"
       << "                       overrides '--arangodb_additional_coordinator_args'\n"
       << "  ARANGODB_ZK          overrides '--zk'\n"
       << "\n"
       << "  MESOS_MASTER         overrides '--master'\n"
       << "  MESOS_SECRET         secret for mesos authentication\n"
       << "  MESOS_AUTHENTICATE   enable authentication\n"
       << "\n";
}

////////////////////////////////////////////////////////////////////////////////
/// @brief string command line argument to bool
////////////////////////////////////////////////////////////////////////////////
bool str2bool(const string in) {
  if (in == "yes" || in == "true" || in == "y") {
    return true;
  } else {
    return false;
  }
}

/* first, here is the code for the signal handler */
void catch_child(int sig_num)
{
  pid_t pid;
  /* when we get here, we know there's a zombie child waiting */
  int child_status;

  pid = waitpid(-1, &child_status, 0);
  LOG(INFO) << "old haproxy(" << pid << ") exited with status " << child_status;
  if (child_status != 0) {
    LOG(INFO) << "Scheduling restart";
    Global::state().setRestartProxy(RESTART_FRESH_START);
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief ArangoDB framework
////////////////////////////////////////////////////////////////////////////////

int main (int argc, char** argv) {

  // ...........................................................................
  // command line options
  // ...........................................................................

  // parse the command line flags
  logging::Flags flags;

  string mode;
  flags.add(&mode,
            "mode",
            "Mode of operation (standalone, cluster)",
            "cluster");

  string async_repl;
  flags.add(&async_repl,
            "async_replication",
            "Flag, whether we run secondaries for asynchronous replication",
            "false");

  string role;
  flags.add(&role,
            "role",
            "Role to use when registering",
            "*");

  string minimal_resources_agent;
  flags.add(&minimal_resources_agent,
            "minimal_resources_agent",
            "Minimal resources to accept for an agent",
            "");

  string minimal_resources_dbserver;
  flags.add(&minimal_resources_dbserver,
            "minimal_resources_dbserver",
            "Minimal resources to accept for a DBServer",
            "");

  string minimal_resources_secondary;
  flags.add(&minimal_resources_secondary,
            "minimal_resources_secondary",
            "Minimal resources to accept for a secondary DBServer",
            "");

  string minimal_resources_coordinator;
  flags.add(&minimal_resources_coordinator,
            "minimal_resources_coordinator",
            "Minimal resources to accept for a coordinator",
            "");

  int nragents;
  flags.add(&nragents,
            "nr_agents",
            "Number of agents in agency",
            1);

  int nrdbservers;
  flags.add(&nrdbservers,
            "nr_dbservers",
            "Initial number of DBservers in cluster",
            2);

  int nrcoordinators;
  flags.add(&nrcoordinators,
            "nr_coordinators",
            "Initial number of coordinators in cluster",
            1);

  string principal;
  flags.add(&principal,
            "principal",
            "Principal for persistent volumes",
            "arangodb");

  string frameworkUser;
  flags.add(&frameworkUser,
            "user",
            "User for the framework",
            "");

  string frameworkName;
  flags.add(&frameworkName,
            "framework_name",
            "custom framework name",
            "arangodb");

  string webui;
  flags.add(&webui,
            "webui",
            "URL to advertise for external access to the UI",
            "");

  int frameworkPort;
  flags.add(&frameworkPort,
            "framework_port",
            "framework http port",
            Global::frameworkPort());
  
  int webuiPort;
  flags.add(&webuiPort,
            "webui_port",
            "webui http port",
            Global::webuiPort());

  double failoverTimeout;
  flags.add(&failoverTimeout,
            "failover_timeout",
            "failover timeout in seconds",
            60 * 60 * 24 * 10);
  
  double declineOfferRefuseSeconds;
  flags.add(&declineOfferRefuseSeconds,
            "refuse_seconds",
            "number of seconds to refuse an offer if declined",
            20);

  int offerLimit;
  flags.add(&offerLimit,
            "offer_limit",
            "number of offers we are accepting",
            10);

  string resetState;
  flags.add(&resetState,
            "reset_state",
            "ignore any old state",
            "false");

  string secondariesWithDBservers;
  flags.add(&secondariesWithDBservers,
            "secondaries_with_dbservers",
            "run secondaries only on agents with DBservers",
            "false");

  string coordinatorsWithDBservers;
  flags.add(&coordinatorsWithDBservers,
            "coordinators_with_dbservers",
            "run coordinators only on agents with DBservers",
            "false");

  string secondarySameServer;
  flags.add(&secondarySameServer,
            "secondary_same_server",
            "allow to run a secondary on same agent as its primary",
            "false");
  
  string arangoDBImage;
  flags.add(&arangoDBImage,
            "arangodb_image",
            "ArangoDB docker image to use",
            "");
  
  string arangoDBForcePullImage;
  flags.add(&arangoDBForcePullImage,
            "arangodb_force_pull_image",
            "force pulling the ArangoDB image",
            "true");


  string arangoDBPrivilegedImage;
  flags.add(&arangoDBPrivilegedImage,
            "arangodb_privileged_image",
            "start the arangodb image privileged",
            "false");
  // address of master and zookeeper
  string master;
  flags.add(&master,
            "master",
            "ip:port of master to connect",
            "");
  
  string arangoDBEnterpriseKey;
  flags.add(&arangoDBEnterpriseKey,
            "arangodb_enterprise_key",
            "enterprise key for arangodb",
            "");
  
  string arangoDBStorageEngine;
  flags.add(&arangoDBStorageEngine,
            "arangodb_storage_engine",
            "storage engine to choose",
            "auto");

  string arangoDBJwtSecret;
  flags.add(&arangoDBJwtSecret,
            "arangodb_jwt_secret",
            "secret for internal cluster communication",
            "");

  string arangoDBSslKeyfile;
  flags.add(&arangoDBSslKeyfile,
            "arangodb_ssl_keyfile",
            "SSL keyfile to use (optional - specify .pem file base64 encoded)",
            "");
  
  string arangoDBEncryptionKeyfile;
  flags.add(&arangoDBEncryptionKeyfile,
            "arangodb_encryption_keyfile",
            "data encryption keyfile to use (optional - specify 32 byte aes keyfile base64 encoded)",
            "");

  string arangoDBAdditionalAgentArgs;
  flags.add(&arangoDBAdditionalAgentArgs,
            "arangodb_additional_agent_args",
            "additional command line arguments to be passed when starting an agent",
            "");

  string arangoDBAdditionalDBServerArgs;
  flags.add(&arangoDBAdditionalDBServerArgs,
            "arangodb_additional_dbserver_args",
            "additional command line arguments to be passed when starting a dbserver",
            "");

  string arangoDBAdditionalSecondaryArgs;
  flags.add(&arangoDBAdditionalSecondaryArgs,
            "arangodb_additional_secondary_args",
            "additional command line arguments to be passed when starting a secondary",
            "");

  string arangoDBAdditionalCoordinatorArgs;
  flags.add(&arangoDBAdditionalCoordinatorArgs,
            "arangodb_additional_coordinator_args",
            "additional command line arguments to be passed when starting an coordinator",
            "");

  string zk;
  flags.add(&zk,
            "zk",
            "zookeeper for state",
            "");

  Try<Nothing> load = flags.load(None(), argc, argv);

  if (load.isError()) {
    cerr << load.error() << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  if (flags.help) {
    usage(argv[0], flags);
    exit(EXIT_SUCCESS);
  }

  updateFromEnv("ARANGODB_MODE", mode);
  updateFromEnv("ARANGODB_ASYNC_REPLICATION", async_repl);
  updateFromEnv("ARANGODB_ROLE", role);
  updateFromEnv("ARANGODB_MINIMAL_RESOURCES_AGENT", minimal_resources_agent);
  updateFromEnv("ARANGODB_MINIMAL_RESOURCES_DBSERVER", minimal_resources_dbserver);
  updateFromEnv("ARANGODB_MINIMAL_RESOURCES_SECONDARY",  minimal_resources_secondary);
  updateFromEnv("ARANGODB_MINIMAL_RESOURCES_COORDINATOR", minimal_resources_coordinator);
  updateFromEnv("ARANGODB_NR_AGENTS", nragents);

  if (nragents < 1) {
    nragents = 1;
  }

  updateFromEnv("ARANGODB_NR_DBSERVERS", nrdbservers);

  if (nrdbservers < 1) {
    nrdbservers = 1;
  }

  updateFromEnv("ARANGODB_NR_COORDINATORS", nrcoordinators);

  if (nrcoordinators < 1) {
    nrcoordinators = 1;
  }

  updateFromEnv("ARANGODB_PRINCIPAL", principal);
  updateFromEnv("ARANGODB_USER", frameworkUser);
  updateFromEnv("ARANGODB_FRAMEWORK_NAME", frameworkName);
  updateFromEnv("ARANGODB_WEBUI", webui);
  updateFromEnv("ARANGODB_WEBUI_PORT", webuiPort);
  updateFromEnv("ARANGODB_FRAMEWORK_PORT", frameworkPort);
  updateFromEnv("ARANGODB_FAILOVER_TIMEOUT", failoverTimeout);
  updateFromEnv("ARANGODB_DECLINE_OFFER_REFUSE_SECONDS", declineOfferRefuseSeconds);
  updateFromEnv("ARANGODB_OFFER_LIMIT", offerLimit);
  updateFromEnv("ARANGODB_RESET_STATE", resetState);
  updateFromEnv("ARANGODB_SECONDARIES_WITH_DBSERVERS", secondariesWithDBservers);
  updateFromEnv("ARANGODB_COORDINATORS_WITH_DBSERVERS", coordinatorsWithDBservers);
  updateFromEnv("ARANGODB_IMAGE", arangoDBImage);
  updateFromEnv("ARANGODB_FORCE_PULL_IMAGE", arangoDBForcePullImage);
  updateFromEnv("ARANGODB_PRIVILEGED_IMAGE", arangoDBPrivilegedImage);
  updateFromEnv("ARANGODB_ENTERPRISE_KEY", arangoDBEnterpriseKey);
  updateFromEnv("ARANGODB_JWT_SECRET", arangoDBJwtSecret);
  updateFromEnv("ARANGODB_SSL_KEYFILE", arangoDBSslKeyfile);
  updateFromEnv("ARANGODB_STORAGE_ENGINE", arangoDBStorageEngine);
  updateFromEnv("ARANGODB_ENCRYPTION_KEYFILE", arangoDBEncryptionKeyfile);
  updateFromEnv("ARANGODB_ADDITIONAL_AGENT_ARGS", arangoDBAdditionalAgentArgs);
  updateFromEnv("ARANGODB_ADDITIONAL_DBSERVER_ARGS", arangoDBAdditionalDBServerArgs);
  updateFromEnv("ARANGODB_ADDITIONAL_SECONDARY_ARGS", arangoDBAdditionalSecondaryArgs);
  updateFromEnv("ARANGODB_ADDITIONAL_COORDINATOR_ARGS", arangoDBAdditionalCoordinatorArgs);

  updateFromEnv("MESOS_MASTER", master);
  updateFromEnv("ARANGODB_ZK", zk);

  if (master.empty()) {
    cerr << "Missing master, either use flag '--master' or set 'MESOS_MASTER'" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  if (arangoDBImage.empty()) {
    cerr << "Missing image, please provide an arangodb image to run on the agents via '--arangodb_image' or set 'ARANGODB_IMAGE'" << endl;
    usage(argv[0], flags);
    exit(EXIT_FAILURE);
  }

  logging::initialize(argv[0], flags, true); // Catch signals.

  Global::setArangoDBImage(arangoDBImage);
  LOG(INFO) << "ArangoDB Image: " << Global::arangoDBImage();

  if (mode == "standalone") {
    Global::setMode(OperationMode::STANDALONE);
  }
  else if (mode == "cluster") {
    Global::setMode(OperationMode::CLUSTER);
  }
  else {
    cerr << argv[0] << ": expecting mode '" << mode << "' to be "
         << "standalone, cluster" << "\n";
  }
  LOG(INFO) << "Mode: " << mode;

  Global::setAsyncReplication(str2bool(async_repl));
  LOG(INFO) << "Asynchronous replication flag: " << Global::asyncReplication();

  Global::setFrameworkName(frameworkName);

  Global::setSecondariesWithDBservers(str2bool(secondariesWithDBservers));
  LOG(INFO) << "SecondariesWithDBservers: " << Global::secondariesWithDBservers();

  Global::setCoordinatorsWithDBservers(str2bool(coordinatorsWithDBservers));
  LOG(INFO) << "CoordinatorsWithDBservers: " << Global::coordinatorsWithDBservers();
  
  Global::setSecondarySameServer(str2bool(secondarySameServer));
  LOG(INFO) << "SecondarySameServer: " << Global::secondarySameServer();
  
  Global::setArangoDBForcePullImage(str2bool(arangoDBForcePullImage));
  LOG(INFO) << "ArangoDBForcePullImage: " << Global::arangoDBForcePullImage();

  Global::setArangoDBPrivilegedImage(str2bool(arangoDBPrivilegedImage));
  LOG(INFO) << "ArangoDBPrivilegedImage: " << Global::arangoDBPrivilegedImage();

  LOG(INFO) << "Minimal resources agent: " << minimal_resources_agent;
  Global::setMinResourcesAgent(minimal_resources_agent);
  LOG(INFO) << "Minimal resources DBserver: " << minimal_resources_dbserver;
  Global::setMinResourcesDBServer(minimal_resources_dbserver);
  LOG(INFO) << "Minimal resources secondary DBserver: " 
            << minimal_resources_secondary;
  Global::setMinResourcesSecondary(minimal_resources_secondary);
  LOG(INFO) << "Minimal resources coordinator: " 
            << minimal_resources_coordinator;
  Global::setMinResourcesCoordinator(minimal_resources_coordinator);
  LOG(INFO) << "Number of agents in agency: " << nragents;
  Global::setNrAgents(nragents);
  LOG(INFO) << "Number of DBservers: " << nrdbservers;
  Global::setNrDBServers(nrdbservers);
  LOG(INFO) << "Number of coordinators: " << nrcoordinators;
  Global::setNrCoordinators(nrcoordinators);
  LOG(INFO) << "Framework port: " << frameworkPort;
  Global::setFrameworkPort(frameworkPort);
  LOG(INFO) << "WebUI port: " << webuiPort;
  Global::setWebuiPort(webuiPort);
  LOG(INFO) << "ArangoDB Enterprise Key: " << arangoDBEnterpriseKey;
  Global::setArangoDBEnterpriseKey(arangoDBEnterpriseKey);
  LOG(INFO) << "ArangoDB JWT Secret: " << arangoDBJwtSecret;
  Global::setArangoDBJwtSecret(arangoDBJwtSecret);
  LOG(INFO) << "ArangoDB SSL Keyfile: " << arangoDBSslKeyfile;
  Global::setArangoDBSslKeyfile(arangoDBSslKeyfile);
  LOG(INFO) << "ArangoDB Storage Engine: " << arangoDBStorageEngine;
  Global::setArangoDBStorageEngine(arangoDBStorageEngine);
  LOG(INFO) << "ArangoDB Encryption Keyfile: " << arangoDBEncryptionKeyfile;
  Global::setArangoDBEncryptionKeyfile(arangoDBEncryptionKeyfile);
  LOG(INFO) << "ArangoDB additional agent args: " << arangoDBAdditionalAgentArgs;
  Global::setArangoDBAdditionalAgentArgs(arangoDBAdditionalAgentArgs);
  LOG(INFO) << "ArangoDB additional dbserver args: " << arangoDBAdditionalAgentArgs;
  Global::setArangoDBAdditionalDBServerArgs(arangoDBAdditionalDBServerArgs);
  LOG(INFO) << "ArangoDB additional secondary args: " << arangoDBAdditionalSecondaryArgs;
  Global::setArangoDBAdditionalSecondaryArgs(arangoDBAdditionalSecondaryArgs);
  LOG(INFO) << "ArangoDB additional coordinator args: " << arangoDBAdditionalCoordinatorArgs;
  Global::setArangoDBAdditionalCoordinatorArgs(arangoDBAdditionalCoordinatorArgs);

  // ...........................................................................
  // state
  // ...........................................................................

  LOG(INFO) << "zookeeper: " << zk;

  ArangoState state(frameworkName, zk);
  state.init();

  if (resetState == "true" || resetState == "y" || resetState == "yes") {
    state.destroy();
  }
  else {
    state.load();
  }
  
  
  if (!state.createReverseProxyConfig()) {
    LOG(ERROR) << "Couldn't create reverse proxy config";
    exit(EXIT_FAILURE);
  }

  struct sigaction new_action;
  new_action.sa_handler = catch_child;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction(SIGCHLD, &new_action, nullptr);
  Global::setState(&state);
  state.setRestartProxy(RESTART_FRESH_START);

  // ...........................................................................
  // framework
  // ...........................................................................

  // create the framework
  mesos::FrameworkInfo framework;
  framework.set_user(frameworkUser);
  LOG(INFO) << "framework user: " << frameworkUser;
  framework.set_checkpoint(true);

  framework.set_name(frameworkName);
  LOG(INFO) << "framework name: " << frameworkName;

  framework.set_role(role);
  LOG(INFO) << "role: " << role;

  if (0.0 < failoverTimeout) {
    framework.set_failover_timeout(failoverTimeout);
  }
  else {
    failoverTimeout = 0.0;
  }

  LOG(INFO) << "failover timeout: " << failoverTimeout;
  
  {
    auto lease = Global::state().lease();
    if (lease.state().has_framework_id()) {
      framework.mutable_id()->CopyFrom(lease.state().framework_id());
    }
  }

  // ...........................................................................
  // http server
  // ...........................................................................

  if (webui.empty()) {
    Try<string> hostnameTry = net::hostname();
    string hostname = hostnameTry.get();

    webui = "http://" + hostname + ":" + std::to_string(webuiPort);
  }

  LOG(INFO) << "webui url: " << webui << " (local port is " << webuiPort << ")";
  LOG(INFO) << "framework listening on port: " << frameworkPort;

  framework.set_webui_url(webui);

  Option<string> mesosCheckpoint =  os::getenv("MESOS_CHECKPOINT");

  if (mesosCheckpoint.isSome()) {
    framework.set_checkpoint(numify<bool>(mesosCheckpoint).get());
  }

  // ...........................................................................
  // global options
  // ...........................................................................

  Global::setRole(role);
  Global::setPrincipal(principal);
  Global::setDeclineOfferRefuseSeconds(declineOfferRefuseSeconds);
  LOG(INFO) << "refuse seconds: " << Global::declineOfferRefuseSeconds();
  Global::setOfferLimit(offerLimit);
  LOG(INFO) << "offer limit: " << Global::offerLimit();


  // ...........................................................................
  // Caretaker
  // ...........................................................................

  unique_ptr<Caretaker> caretaker;

  switch (Global::mode()) {
    case OperationMode::STANDALONE:
      caretaker.reset(new CaretakerStandalone);
      break;

    case OperationMode::CLUSTER:
      caretaker.reset(new CaretakerCluster);
      break;
  }

  Global::setCaretaker(caretaker.get());

  // ...........................................................................
  // manager
  // ...........................................................................

  ArangoManager* manager = new ArangoManager();
  Global::setManager(manager);

  // ...........................................................................
  // scheduler
  // ...........................................................................

  // create the scheduler
  ArangoScheduler scheduler;

  mesos::MesosSchedulerDriver* driver;

  Option<string> mesosAuthenticate = os::getenv("MESOS_AUTHENTICATE");

  if (mesosAuthenticate.isSome() && mesosAuthenticate.get() == "true") {
    cout << "Enabling authentication for the framework" << endl;

    if (principal.empty()) {
      EXIT(EXIT_FAILURE) << "Expecting authentication principal in the environment";
    }

    Option<string> mesosSecret = os::getenv("MESOS_SECRET");

    if (mesosSecret.isNone()) {
      EXIT(EXIT_FAILURE) << "Expecting authentication secret in the environment";
    }

    mesos::Credential credential;
    credential.set_principal(principal);
    credential.set_secret(mesosSecret.get());

    framework.set_principal(principal);
    driver = new mesos::MesosSchedulerDriver(&scheduler, framework, master, credential);
  }
  else {
    framework.set_principal(principal);
    driver = new mesos::MesosSchedulerDriver(&scheduler, framework, master);
  }

  scheduler.setDriver(driver);

  Global::setScheduler(&scheduler);

  // ...........................................................................
  // run
  // ...........................................................................

  // and the http server
  HttpServer http;

  // start and wait
  LOG(INFO) << "http port: " << Global::frameworkPort();
  http.start(Global::frameworkPort());

  int status = driver->run() == mesos::DRIVER_STOPPED ? 0 : 1;

  // ensure that the driver process terminates
  driver->stop();

  delete driver;
  delete manager;

  sleep(30);   // Wait some more time before terminating the process to
               // allow the user to use 
               //   dcos package uninstall arangodb
               // to remove the Marathon job
  http.stop();

  return status;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------
