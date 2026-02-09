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

#include "src/spine/device/device_local.h"

#include <gtest/gtest.h>

#include <memory>

#include "mocks/common/eebus_timer/eebus_timer_mock.h"
#include "mocks/ship/ship_connection/data_writer_mock.h"
#include "mocks/use_case/api/eg_lp_listener_mock.h"
#include "src/common/array_util.h"
#include "src/common/eebus_malloc.h"
#include "src/common/eebus_timer/eebus_timer.h"
#include "src/common/message_buffer.h"
#include "src/spine/device/device_local.h"
#include "src/spine/device/device_local_internal.h"
#include "src/spine/entity/entity_local.h"
#include "src/use_case/actor/eg/lpc/eg_lpc.h"
#include "tests/src/json.h"
#include "tests/src/use_case/actor/eg/lpc/device_diagnosis_heartbeat_request.inc"
#include "tests/src/use_case/actor/eg/lpc/device_diagnosis_subscription_request.inc"
#include "tests/src/use_case/actor/eg/lpc/discovery_request.inc"
#include "tests/src/use_case/actor/eg/lpc/discovery_response.inc"
#include "tests/src/use_case/actor/eg/lpc/limits_description_reply.inc"
#include "tests/src/use_case/actor/eg/lpc/limits_reply.inc"
#include "tests/src/use_case/actor/eg/lpc/node_management_subscription_request.inc"
#include "tests/src/use_case/actor/eg/lpc/result_data_msg_cnt_ref_11.inc"
#include "tests/src/use_case/actor/eg/lpc/result_data_msg_cnt_ref_3.inc"
#include "tests/src/use_case/actor/eg/lpc/result_data_msg_cnt_ref_5.inc"
#include "tests/src/use_case/actor/eg/lpc/result_data_msg_cnt_ref_6.inc"
#include "tests/src/use_case/actor/eg/lpc/result_data_msg_cnt_ref_8.inc"
#include "tests/src/use_case/actor/eg/lpc/result_data_msg_cnt_ref_9.inc"
#include "tests/src/use_case/actor/eg/lpc/use_case_reply.inc"
#include "tests/src/use_case/actor/eg/lpc/use_case_request.inc"
#include "tests/src/use_case/use_case_test_fixture.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::WithArgs;

class EgLpcTestFixture : public UseCaseTestFixture {
 public:
  EgLpcTestFixture() : UseCaseTestFixture("HEMS", "HEMS", "123456789") {};
  void SetUpUseCase() override {
    uint32_t entity_ids[1] = {static_cast<uint32_t>(VectorGetSize(DEVICE_LOCAL_GET_ENTITIES(device_local_.get())))};

    EntityLocalObject* const entity = EntityLocalCreate(
        device_local_.get(),
        kEntityTypeTypeGridGuard,
        entity_ids,
        ARRAY_SIZE(entity_ids),
        kHeartbeatTimeout
    );

    eg_lpc_listener_mock_.reset(EgLpListenerMockCreate());
    use_case_.reset(EgLpcUseCaseCreate(entity, EG_LP_LISTENER_OBJECT(eg_lpc_listener_mock_.get())));

    DEVICE_LOCAL_ADD_ENTITY(device_local_.get(), entity);
  };

  void TearDownUseCase() override {
    EXPECT_CALL(*eg_lpc_listener_mock_->gmock, Destruct(_)).WillOnce(Return());
    use_case_.reset();
    eg_lpc_listener_mock_.reset();
  };

 protected:
  std::unique_ptr<EgLpListenerMock, decltype(&EgLpListenerMockDelete)> eg_lpc_listener_mock_{
      nullptr,
      EgLpListenerMockDelete
  };

  std::unique_ptr<EgLpUseCaseObject, decltype(&EgLpUseCaseDelete)> use_case_{nullptr, EgLpUseCaseDelete};
};

TEST_F(EgLpcTestFixture, EgLpcTest) {
  // 1. Receive the detailed discovery request and send the response
  HandleMessage(discovery_request);
  // 2. Receive the detailed discovery and send the response
  EXPECT_CALL(*eg_lpc_listener_mock_->gmock, OnRemoteEntityConnect(_, _)).WillOnce(Return());
  HandleMessage(discovery_response);
  // 3. Receive the result with message counter reference 5
  HandleMessage(result_data_msg_cnt_ref_5);
  // 4. Receive the result with message counter reference 6
  HandleMessage(result_data_msg_cnt_ref_6);
  // 5. Receive the result with message counter reference 8
  HandleMessage(result_data_msg_cnt_ref_8);
  // 6. Receive the result with message counter reference 9
  HandleMessage(result_data_msg_cnt_ref_9);
  // 7. Receive the result with message counter reference 11
  HandleMessage(result_data_msg_cnt_ref_11);
  // 8. Receive the Node Management subscription request
  HandleMessage(node_management_subscription_request);
  // 9. Received the usecase discovery and send the response
  HandleMessage(use_case_request);
  // 10. Receive the Use Case reply
  HandleMessage(use_case_reply);

  // 11. Receive the Device Diagnosis subscription request
  HandleMessage(device_diagnosis_subscription_request);
  // 12. Receive the Heartbeat subscription request
  HandleMessage(device_diagnosis_heartbeat_request);
  // 13. Receive the result with message counter reference 3
  HandleMessage(result_data_msg_cnt_ref_3);
  // 14. Receive the Load Control Limit Description reply
  HandleMessage(limits_description_reply);
  // 15. Receive the Load Control Limit reply
  EXPECT_CALL(*eg_lpc_listener_mock_->gmock, OnPowerLimitReceive(_, _, _, _))
      .WillOnce(WithArgs<1, 2, 3>(Invoke([](const ScaledValue* value, const DurationType* duration, bool is_active) {
        ASSERT_NE(value, nullptr);
        EXPECT_EQ(value->value, 4200);
        EXPECT_EQ(value->scale, 0);
        EXPECT_EQ(is_active, false);
      })));
  HandleMessage(limits_reply);
  // 16. Expect the remote entity disconnect event while tearing down the use case
  EXPECT_CALL(*eg_lpc_listener_mock_->gmock, OnRemoteEntityDisconnect(_, _));
}
