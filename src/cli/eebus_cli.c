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
 * @brief EEBUS CLI implementation
 */

#include <stdio.h>

#include "src/cli/eebus_cli.h"
#include "src/cli/eebus_cli_eg_lp.h"
#include "src/cli/eebus_cli_internal.h"
#include "src/cli/eebus_cli_ma_mpc.h"
#include "src/common/array_util.h"
#include "src/common/eebus_errors.h"
#include "src/common/eebus_malloc.h"
#include "src/common/string_util.h"

static void Destruct(EebusCliObject* self);
static void
SetEgLpc(EebusCliObject* self, EgLpUseCaseObject* eg_lpc_use_case, const EntityAddressType* remote_entity_address);
static void
SetMaMpc(EebusCliObject* self, MaMpcUseCaseObject* ma_mpc_use_case, const EntityAddressType* remote_entity_address);
static void HandleCmd(const EebusCliObject* self, char* cmd);

static const EebusCliInterface eebus_cli_methods = {
    .destruct   = Destruct,
    .set_eg_lpc = SetEgLpc,
    .set_ma_mpc = SetMaMpc,
    .handle_cmd = HandleCmd,
};

static EebusError EebusCliConstruct(EebusCli* self);
static void EebusCliHandleCmdEgLpc(const EebusCli* self, const char* const* tokens, size_t num_tokens);

EebusError EebusCliConstruct(EebusCli* self) {
  // Override "virtual functions table"
  EEBUS_CLI_INTERFACE(self) = &eebus_cli_methods;

  self->eg_lpc_cli = NULL;

  self->ma_mpc             = NULL;
  self->ma_mpc_entity_addr = NULL;

  return kEebusErrorOk;
}

EebusCliObject* EebusCliCreate(void) {
  EebusCli* const eebus_cli = (EebusCli*)EEBUS_MALLOC(sizeof(EebusCli));
  if (eebus_cli == NULL) {
    return NULL;
  }

  if (EebusCliConstruct(eebus_cli) != kEebusErrorOk) {
    EebusCliDelete(EEBUS_CLI_OBJECT(eebus_cli));
    return NULL;
  }

  return EEBUS_CLI_OBJECT(eebus_cli);
}

void Destruct(EebusCliObject* self) {
  EebusCli* const eebus_cli = EEBUS_CLI(self);

  EntityAddressDelete((EntityAddressType*)eebus_cli->ma_mpc_entity_addr);
  eebus_cli->ma_mpc_entity_addr = NULL;

  EgLpCliDelete(eebus_cli->eg_lpc_cli);
  eebus_cli->eg_lpc_cli = NULL;
}

void SetEgLpc(
    EebusCliObject* self,
    EgLpUseCaseObject* eg_lpc_use_case,
    const EntityAddressType* remote_entity_address
) {
  EebusCli* const eebus_cli = EEBUS_CLI(self);

  // Release the previously created CLI instance
  if (eebus_cli->eg_lpc_cli != NULL) {
    EgLpCliDelete(eebus_cli->eg_lpc_cli);
    eebus_cli->eg_lpc_cli = NULL;
  }

  // Copy the new entity address if not NULL
  if (remote_entity_address != NULL) {
    eebus_cli->eg_lpc_cli = EgLpCliCreate(kEnergyDirectionTypeConsume, eg_lpc_use_case, remote_entity_address);
  }
}

void SetMaMpc(
    EebusCliObject* self,
    MaMpcUseCaseObject* ma_mpc_use_case,
    const EntityAddressType* remote_entity_address
) {
  EebusCli* const eebus_cli = EEBUS_CLI(self);

  eebus_cli->ma_mpc = ma_mpc_use_case;

  // Release the old entity adress
  if (eebus_cli->ma_mpc_entity_addr != NULL) {
    EntityAddressDelete((EntityAddressType*)eebus_cli->ma_mpc_entity_addr);
    eebus_cli->ma_mpc_entity_addr = NULL;
  }

  // Copy the new entity address if not NULL
  if (remote_entity_address != NULL) {
    eebus_cli->ma_mpc_entity_addr = EntityAddressCopy(remote_entity_address);
  }
}

void EebusCliHandleCmdEgLpc(const EebusCli* self, const char* const* tokens, size_t num_tokens) {
  if (self->eg_lpc_cli == NULL) {
    printf("EG LPC Use Case not set in CLI handler\n");
    return;
  }

  EgLpCliHandleCmdEgLp(self->eg_lpc_cli, tokens, num_tokens);
}

void HandleCmd(const EebusCliObject* self, char* cmd) {
  const EebusCli* const eebus_cli = EEBUS_CLI(self);

  const char* tokens[10] = {0};
  size_t num_tokens      = 0;

  static const char delimiters[] = " \t\n";

  char* p = NULL;

  tokens[num_tokens++] = StringToken(cmd, delimiters, &p);
  if (tokens[0] == NULL) {
    return;
  }

  while (num_tokens < ARRAY_SIZE(tokens)) {
    char* const token = StringToken(NULL, delimiters, &p);
    if (token == NULL) {
      break;
    }

    if (num_tokens >= ARRAY_SIZE(tokens)) {
      printf("Too many arguments specified!\n");
      break;
    }

    tokens[num_tokens++] = token;
  }

  if (strcmp(tokens[0], "eg_lpc") == 0) {
    EebusCliHandleCmdEgLpc(eebus_cli, tokens, num_tokens);
  } else if (strcmp(tokens[0], "ma_mpc") == 0) {
    EebusCliHandleCmdMaMpc(eebus_cli, tokens, num_tokens);
  } else {
    printf("Unknown command: %s\n", tokens[0]);
    return;
  }
}
