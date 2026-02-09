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
 * @brief EEBUS CLI EG LP commands handling implementation
 */

#include <inttypes.h>
#include <stdio.h>

#include "src/cli/eebus_cli_eg_lp.h"
#include "src/cli/eebus_cli_internal.h"
#include "src/common/string_util.h"
#include "src/use_case/model/scaled_value.h"

struct EgLpCli {
  /** EG LP instance to deal with */
  EgLpUseCaseObject* eg_lp;
  /** EG LP remote entity address to communicate with */
  const EntityAddressType* entity_addr;
  /** Command name ("eg_lpc" or "eg_lpp") */
  const char* cmd_name;
  /** Command name in uppercase ("EG LPC" or "EG LPP") */
  const char* cmd_name_caps;
};

static EebusError EgLpCliConstruct(
    EgLpCli* self,
    EnergyDirectionType energy_direction,
    EgLpUseCaseObject* eg_lp,
    const EntityAddressType* entity_addr
);
static void EgLpCliDestruct(EgLpCli* self);

static void HandleCmdGetPowerLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdGetFailsafeLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdGetFailsafeDuration(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdGet(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdSetPowerLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdSetFailsafeLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdSetFailsafeDuration(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdSet(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdStart(const EgLpCli* self, const char* const* tokens, size_t num_tokens);
static void HandleCmdStop(const EgLpCli* self, const char* const* tokens, size_t num_tokens);

EebusError EgLpCliConstruct(
    EgLpCli* self,
    EnergyDirectionType energy_direction,
    EgLpUseCaseObject* eg_lp,
    const EntityAddressType* entity_addr
) {
  self->eg_lp         = eg_lp;
  self->entity_addr   = NULL;
  self->cmd_name      = NULL;
  self->cmd_name_caps = NULL;

  if ((eg_lp == NULL) || (entity_addr == NULL)) {
    return kEebusErrorInputArgumentNull;
  }

  self->entity_addr = EntityAddressCopy(entity_addr);
  if (self->entity_addr == NULL) {
    return kEebusErrorMemoryAllocate;
  }

  if (energy_direction == kEnergyDirectionTypeConsume) {
    self->cmd_name      = "eg_lpc";
    self->cmd_name_caps = "EG LPC";
  } else {
    self->cmd_name      = "eg_lpp";
    self->cmd_name_caps = "EG LPP";
  }

  return kEebusErrorOk;
}

EgLpCli*
EgLpCliCreate(EnergyDirectionType energy_direction, EgLpUseCaseObject* eg_lp, const EntityAddressType* entity_addr) {
  EgLpCli* cli_eg_lp = (EgLpCli*)EEBUS_MALLOC(sizeof(EgLpCli));
  if (cli_eg_lp == NULL) {
    return NULL;
  }

  if (EgLpCliConstruct(cli_eg_lp, energy_direction, eg_lp, entity_addr) != kEebusErrorOk) {
    EgLpCliDelete(cli_eg_lp);
    return NULL;
  }

  return cli_eg_lp;
}

void EgLpCliDestruct(EgLpCli* self) {
  EntityAddressDelete((EntityAddressType*)self->entity_addr);
  self->entity_addr = NULL;
}

void EgLpCliDelete(EgLpCli* eebus_cli_eg_lp) {
  if (eebus_cli_eg_lp != NULL) {
    EgLpCliDestruct(eebus_cli_eg_lp);
    EEBUS_FREE(eebus_cli_eg_lp);
  }
}

//-------------------------------------------------------------------------------------------//
//
// EG LP Getters Handling
//
//-------------------------------------------------------------------------------------------//
void HandleCmdGetPowerLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  LoadLimit limit = {0};
  if (EgLpGetActivePowerLimit(self->eg_lp, self->entity_addr, &limit) != kEebusErrorOk) {
    printf("Getting power limit failed\n");
    return;
  }

  const char* const value_str = ScaledValueToString(&limit.value);
  if (value_str == NULL) {
    printf("Converting power limit to string failed\n");
    return;
  }

  printf(
      "Power Limit: value=%s, duration=%" PRId32 "h, active=%s\n",
      value_str,
      limit.duration.hours,
      limit.is_active ? "true" : "false"
  );

  StringDelete((char*)value_str);
}

void HandleCmdGetFailsafeLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  ScaledValue power_limit = {0};

  if (EgLpGetFailsafeActivePowerLimit(self->eg_lp, self->entity_addr, &power_limit) != kEebusErrorOk) {
    printf("Getting failsafe limit failed\n");
    return;
  }

  const char* const value_str = ScaledValueToString(&power_limit);
  if (value_str == NULL) {
    printf("Converting failsafe limit to string failed\n");
    return;
  }

  printf("Failsafe Active Power Limit: value=%s\n", value_str);

  StringDelete((char*)value_str);
}

void HandleCmdGetFailsafeDuration(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  DurationType duration = {0};

  if (num_tokens != 3) {
    printf("Insufficient arguments for %s get failsafe_duration command\n", self->cmd_name);
    return;
  }

  if (EgLpGetFailsafeDurationMinimum(self->eg_lp, self->entity_addr, &duration) != kEebusErrorOk) {
    printf("Getting failsafe duration failed\n");
    return;
  }

  EebusDurationPrint("Failsafe Duration Minimum: %s\n", &duration);
}

void HandleCmdGet(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  if (num_tokens != 3) {
    printf("Insufficient arguments for %s get command\n", self->cmd_name);
    return;
  }

  const char* subcommand = tokens[2];
  if (strcmp(subcommand, "power_limit") == 0) {
    HandleCmdGetPowerLimit(self, tokens, num_tokens);
  } else if (strcmp(subcommand, "failsafe_limit") == 0) {
    HandleCmdGetFailsafeLimit(self, tokens, num_tokens);
  } else if (strcmp(subcommand, "failsafe_duration") == 0) {
    HandleCmdGetFailsafeDuration(self, tokens, num_tokens);
  } else {
    printf("Unknown subcommand for %s get: %s\n", self->cmd_name, subcommand);
  }
}

//-------------------------------------------------------------------------------------------//
//
// EG LP Setters Handling
//
//-------------------------------------------------------------------------------------------//
void HandleCmdSetPowerLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  // Example:
  if (num_tokens != 6) {
    printf("Insufficient arguments for %s set power_limit command\n", self->cmd_name);
    return;
  }

  LoadLimit limit = {0};

  if (ScaledValueParse(tokens[3], &limit.value) != kEebusErrorOk) {
    printf("Invalid limit value: %s\n", tokens[3]);
    return;
  }

  if (EebusDurationParse(tokens[4], &limit.duration) != kEebusErrorOk) {
    printf("Invalid duration value: %s\n", tokens[4]);
    return;
  }

  if (strcmp(tokens[5], "true") == 0) {
    limit.is_active = true;
  } else if (strcmp(tokens[5], "false") == 0) {
    limit.is_active = false;
  } else {
    printf("Invalid active flag value: %s\n", tokens[5]);
    return;
  }

  if (EgLpSetActivePowerLimit(self->eg_lp, self->entity_addr, &limit) != kEebusErrorOk) {
    printf("Setting power limit failed\n");
  }
}

void HandleCmdSetFailsafeLimit(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  ScaledValue power_limit = {0};

  if (num_tokens != 4) {
    printf("Insufficient arguments for %s set failsafe_limit command\n", self->cmd_name);
    return;
  }

  if (ScaledValueParse(tokens[3], &power_limit) != kEebusErrorOk) {
    printf("Invalid value for failsafe_limit: %s\n", tokens[3]);
    return;
  }

  const EntityAddressType* entity_addr = self->entity_addr;
  if (EgLpSetFailsafeActivePowerLimit(self->eg_lp, entity_addr, &power_limit) != kEebusErrorOk) {
    printf("Setting failsafe limit failed\n");
  }
}

void HandleCmdSetFailsafeDuration(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  if (num_tokens != 4) {
    printf("Insufficient arguments for %s set failsafe_duration command\n", self->cmd_name);
    return;
  }

  DurationType duration = {0};
  if (EebusDurationParse(tokens[3], &duration) != kEebusErrorOk) {
    printf("Invalid value for failsafe_duration: %s\n", tokens[3]);
    return;
  }

  if (EgLpSetFailsafeDurationMinimum(self->eg_lp, self->entity_addr, &duration) != kEebusErrorOk) {
    printf("Setting failsafe duration failed\n");
  }
}

void HandleCmdSet(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  if (num_tokens < 3) {
    printf("Insufficient arguments for %s set command\n", self->cmd_name);
    return;
  }

  const char* subcommand = tokens[2];
  if (strcmp(subcommand, "power_limit") == 0) {
    HandleCmdSetPowerLimit(self, tokens, num_tokens);
  } else if (strcmp(subcommand, "failsafe_limit") == 0) {
    HandleCmdSetFailsafeLimit(self, tokens, num_tokens);
  } else if (strcmp(subcommand, "failsafe_duration") == 0) {
    HandleCmdSetFailsafeDuration(self, tokens, num_tokens);
  } else {
    printf("Unknown subcommand for %s set: %s\n", self->cmd_name, subcommand);
  }
}

//-------------------------------------------------------------------------------------------//
//
// EG LP Start/Stop Handling
//
//-------------------------------------------------------------------------------------------//
void HandleCmdStart(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  if (num_tokens < 3) {
    printf("Insufficient arguments for %s start command\n", self->cmd_name);
    return;
  }

  const char* const subcommand = tokens[2];

  if (strcmp(subcommand, "heartbeat") == 0) {
    EgLpStartHeartbeat(self->eg_lp);
    printf("%s Heartbeat started\n", self->cmd_name_caps);
  } else {
    printf("Unknown subcommand for %s start: %s\n", self->cmd_name, subcommand);
  }
}

void HandleCmdStop(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  if (num_tokens < 3) {
    printf("Insufficient arguments for %s stop command\n", self->cmd_name);
    return;
  }

  const char* const subcommand = tokens[2];

  if (strcmp(subcommand, "heartbeat") == 0) {
    EgLpStopHeartbeat(self->eg_lp);
    printf("%s Heartbeat stopped\n", self->cmd_name_caps);
  } else {
    printf("Unknown subcommand for %s stop: %s\n", self->cmd_name, subcommand);
  }
}

void EgLpCliHandleCmdEgLp(const EgLpCli* self, const char* const* tokens, size_t num_tokens) {
  if (num_tokens < 2) {
    printf("Insufficient arguments for %s command\n", self->cmd_name);
    return;
  }

  if (strcmp(tokens[1], "set") == 0) {
    HandleCmdSet(self, tokens, num_tokens);
  } else if (strcmp(tokens[1], "get") == 0) {
    HandleCmdGet(self, tokens, num_tokens);
  } else if (strcmp(tokens[1], "start") == 0) {
    HandleCmdStart(self, tokens, num_tokens);
  } else if (strcmp(tokens[1], "stop") == 0) {
    HandleCmdStop(self, tokens, num_tokens);
  } else {
    printf("Unknown subcommand for %s: %s\n", self->cmd_name, tokens[1]);
  }
}
