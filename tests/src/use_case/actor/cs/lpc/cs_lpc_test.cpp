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
 * @brief Currently it is not a regular unit test but more a "sand box"
 * to feed the SPINE Device with specific datagrams and check the outgoing messages printed.
 * @note Remember to enable the message printing in PrintMessage() before getting started
 */

#include "src/use_case/actor/cs/lpc/cs_lpc.h"

#include <gtest/gtest.h>

#include <memory>

#include "mocks/common/eebus_timer/eebus_timer_mock.h"
#include "mocks/ship/ship_connection/data_writer_mock.h"
#include "mocks/use_case/api/cs_lp_listener_mock.h"
#include "src/common/array_util.h"
#include "src/common/eebus_malloc.h"
#include "src/common/eebus_timer/eebus_timer.h"
#include "src/common/message_buffer.h"
#include "src/spine/device/device_local.h"
#include "src/spine/device/device_local_internal.h"
#include "src/spine/entity/entity_local.h"
#include "tests/src/json.h"
#include "tests/src/use_case/actor/cs/lpc/device_configuration_binding_request.inc"
#include "tests/src/use_case/actor/cs/lpc/device_configuration_description_request.inc"
#include "tests/src/use_case/actor/cs/lpc/device_configuration_key_value_list_request.inc"
#include "tests/src/use_case/actor/cs/lpc/device_configuration_subscription_request.inc"
#include "tests/src/use_case/actor/cs/lpc/device_diagnosis_heartbeat_reply.inc"
#include "tests/src/use_case/actor/cs/lpc/device_diagnosis_heartbeat_request.inc"
#include "tests/src/use_case/actor/cs/lpc/device_diagnosis_subscription_request.inc"
#include "tests/src/use_case/actor/cs/lpc/discovery_request.inc"
#include "tests/src/use_case/actor/cs/lpc/discovery_response.inc"
#include "tests/src/use_case/actor/cs/lpc/electrical_connection_subscription_request.inc"
#include "tests/src/use_case/actor/cs/lpc/failsafe_duration_write.inc"
#include "tests/src/use_case/actor/cs/lpc/failsafe_power_limit_write.inc"
#include "tests/src/use_case/actor/cs/lpc/heartbeat_notify.inc"
#include "tests/src/use_case/actor/cs/lpc/limits_request.inc"
#include "tests/src/use_case/actor/cs/lpc/limits_write.inc"
#include "tests/src/use_case/actor/cs/lpc/load_control_binding_request.inc"
#include "tests/src/use_case/actor/cs/lpc/load_control_description_request.inc"
#include "tests/src/use_case/actor/cs/lpc/load_control_subscription_request.inc"
#include "tests/src/use_case/actor/cs/lpc/node_management_subscription_request.inc"
#include "tests/src/use_case/actor/cs/lpc/result_data_msg_cnt_ref_3.inc"
#include "tests/src/use_case/actor/cs/lpc/result_data_msg_cnt_ref_5.inc"
#include "tests/src/use_case/actor/cs/lpc/use_case_reply.inc"
#include "tests/src/use_case/actor/cs/lpc/use_case_request.inc"
#include "tests/src/use_case/use_case_test_fixture.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::WithArgs;

class CsLpcTestFixture : public UseCaseTestFixture {
 public:
  CsLpcTestFixture() : UseCaseTestFixture("HeatPump", "HeatPump", "123456789") {};
  void SetUpUseCase() override {
    uint32_t entity_ids[1] = {static_cast<uint32_t>(VectorGetSize(DEVICE_LOCAL_GET_ENTITIES(device_local_.get())))};

    EntityLocalObject* const entity = EntityLocalCreate(
        device_local_.get(),
        kEntityTypeTypeCompressor,
        entity_ids,
        ARRAY_SIZE(entity_ids),
        kHeartbeatTimeout
    );

    cs_lpc_listener_mock_.reset(CsLpListenerMockCreate());
    use_case_.reset(CsLpcUseCaseCreate(entity, 0, CS_LP_LISTENER_OBJECT(cs_lpc_listener_mock_.get())));
    const ScaledValue limit = {4200, 0};
    CsLpcSetActiveConsumptionPowerLimit(use_case_.get(), &limit, false, true);

    DEVICE_LOCAL_ADD_ENTITY(device_local_.get(), entity);
  };

  void TearDownUseCase() override {
    EXPECT_CALL(*cs_lpc_listener_mock_->gmock, Destruct(_)).WillOnce(Return());
    use_case_.reset();
    cs_lpc_listener_mock_.reset();
  };

 protected:
  std::unique_ptr<CsLpListenerMock, decltype(&CsLpListenerMockDelete)> cs_lpc_listener_mock_{
      nullptr,
      CsLpListenerMockDelete
  };

  std::unique_ptr<CsLpUseCaseObject, decltype(&CsLpUseCaseDelete)> use_case_{nullptr, CsLpUseCaseDelete};
};

TEST_F(CsLpcTestFixture, CsLpcTest) {
  // 1. Receive the detailed discovery request and send the response
  HandleMessage(discovery_request);
  // 2. Receive the detailed discovery and send the response
  HandleMessage(discovery_response);
  // 3. Receive the Node Management dubscription request
  HandleMessage(node_management_subscription_request);
  // 4. Receive the result with message counter reference 3
  HandleMessage(result_data_msg_cnt_ref_3);
  // 5. Receive the Use Case reply
  HandleMessage(use_case_reply);
  // 6. Receive the result with message counter reference 5
  HandleMessage(result_data_msg_cnt_ref_5);

  // 7. Receive the use case discovery and send the response
  HandleMessage(use_case_request);
  // 8. Receive the load control subscription request and send the response
  HandleMessage(load_control_subscription_request);
  // 9. Receive the load control binding request and send the response
  HandleMessage(load_control_binding_request);
  // 10. Receive the load control description read request and send the response
  HandleMessage(load_control_description_request);
  // 11. Receive the device configuration subscription request and send the response
  HandleMessage(device_configuration_subscription_request);
  // 12. Receive the device configuration binding request and send the response
  HandleMessage(device_configuration_binding_request);
  // 13. Receive the device configuration description request and send the response
  HandleMessage(device_configuration_description_request);
  // 14. Receive the Device Diagnosis subscription request and send the response
  HandleMessage(device_diagnosis_subscription_request);
  // 15. Receive the Electrical Connection subscription request
  HandleMessage(electrical_connection_subscription_request);
  // 16. Receive the Heartbeat subscription request
  HandleMessage(device_diagnosis_heartbeat_request);
  // 17. Receive the Heartbeat reply
  HandleMessage(device_diagnosis_heartbeat_reply);
  // 18. Receive the Limits request
  HandleMessage(limits_request);
  // 19. Receive the Device Configuration Key Value List request and send the response
  HandleMessage(device_configuration_key_value_list_request);
  // 20. Receive the Load Control Limits write and process the new data
  EXPECT_CALL(*cs_lpc_listener_mock_->gmock, OnPowerLimitReceive(_, _, _, _))
      .WillOnce(
          WithArgs<1, 2, 3>(Invoke([](const ScaledValue* power_limit, const DurationType* duration, bool is_active) {
            ASSERT_NE(power_limit, nullptr);
            ASSERT_NE(duration, nullptr);
            EXPECT_EQ(power_limit->value, 100);
            EXPECT_EQ(power_limit->scale, 0);
            EXPECT_EQ(duration->hours, 1);
            EXPECT_EQ(duration->minutes, 2);
            EXPECT_EQ(duration->seconds, 3);
            EXPECT_EQ(is_active, true);
          }))
      );
  HandleMessage(limits_write);

  // 21. Recieve the Failsafe Consumption Active Power Limit write and process the new data
  EXPECT_CALL(*cs_lpc_listener_mock_->gmock, OnFailsafePowerLimitReceive(_, _))
      .WillOnce(WithArgs<1>(Invoke([](const ScaledValue* power_limit) {
        ASSERT_NE(power_limit, nullptr);
        EXPECT_EQ(power_limit->value, 14);
        EXPECT_EQ(power_limit->scale, 1);
      })));
  HandleMessage(failsafe_power_limit_write);

  // 22. Recieve the Failsafe Duration Minimum write and process the new data
  EXPECT_CALL(*cs_lpc_listener_mock_->gmock, OnFailsafeDurationReceive(_, _))
      .WillOnce(WithArgs<1>(Invoke([](const DurationType* duration) {
        ASSERT_NE(duration, nullptr);
        EXPECT_EQ(duration->hours, 1);
        EXPECT_EQ(duration->minutes, 2);
        EXPECT_EQ(duration->seconds, 5);
      })));
  HandleMessage(failsafe_duration_write);

  // 23. Set the Consumption Nominal Maximum value
  const ScaledValue consumption_nominal_max_set{700, 1};
  CsLpcSetConsumptionNominalMax(use_case_.get(), &consumption_nominal_max_set);

  ScaledValue consumption_nominal_max_get = {0};
  // 24. Get the Consumption Nominal Maximum value
  const EebusError err = CsLpcGetConsumptionNominalMax(use_case_.get(), &consumption_nominal_max_get);
  EXPECT_EQ(err, kEebusErrorOk);
  EXPECT_EQ(consumption_nominal_max_get.value, 700);
  EXPECT_EQ(consumption_nominal_max_get.scale, 1);

  EXPECT_CALL(*cs_lpc_listener_mock_->gmock, OnHeartbeatReceive(_, _)).WillOnce(Return());
  HandleMessage(heartbeat_notify);
}
