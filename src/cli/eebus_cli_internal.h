/*
 * Copyright 2025 NIBE AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @file
 * @brief EEBUS CLI private declarations
 */

#ifndef SRC_CLI_EEBUS_CLI_INTERNAL_H_
#define SRC_CLI_EEBUS_CLI_INTERNAL_H_

#include "src/cli/eebus_cli_interface.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct EgLpCli EgLpCli;

typedef struct EebusCli EebusCli;

struct EebusCli {
  /** Implements the EEBUS CLI Interface */
  EebusCliObject obj;

  /** EG LPC CLI instance to deal with */
  EgLpCli* eg_lpc_cli;

  /** MA MPC instance to deal with */
  MaMpcUseCaseObject* ma_mpc;
  /** MA MPC remote entity address to communicate with */
  const EntityAddressType* ma_mpc_entity_addr;
};

#define EEBUS_CLI(obj) ((EebusCli*)(obj))

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // SRC_CLI_EEBUS_CLI_INTERNAL_H_
