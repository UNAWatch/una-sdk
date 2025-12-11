/**
 * @file ConcreteMessages.hpp
 * @date 02-12-2024
 * @author Kernel Communication Module
 * @brief Concrete message structures for kernel-application communication
 *
 * This file defines specific message types used in the system.
 * Each message inherits from MessageBase and adds payload fields.
 *
 * Shared between kernel and applications.
 *
 * Message design guidelines:
 * - Keep messages small (prefer multiple small messages over one large)
 * - For request-response: include both request and response fields
 * - Use fixed-size types (uint32_t, float, etc.) for portability
 * - Avoid pointers in message payload
 * - Consider alignment and padding
 */

#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"

namespace SDK::Message::Sensor
{

struct RequestDefault : public MessageBase {
    RequestDefault()
        : MessageBase(MessageType::REQUEST_SENSOR_LAYER_GET_DEFAULT)
        , id(SDK::Sensor::Type::UNKNOWN)
        , handle(0)
    {}

    SDK::Sensor::Type id;

    // Response field
    uint32_t handle;
};

struct RequestList : public MessageBase {
    RequestList()
        : MessageBase(MessageType::REQUEST_SENSOR_LAYER_GET_LIST)
        , id(SDK::Sensor::Type::UNKNOWN)
        , handles{}
        , handlesCount(0)
    {}

    SDK::Sensor::Type id;

    // Response fields
    uint32_t handles[10];
    uint32_t handlesCount;
};

struct RequestGetDesc : public MessageBase {
    RequestGetDesc()
        : MessageBase(MessageType::REQUEST_SENSOR_LAYER_GET_DESCRIPTOR)
        , handle(0)
        , desc{}
    {}

    uint32_t handle;

    // Response fields
    char desc[32];
};

struct RequestConnect : public MessageBase {
    RequestConnect()
        : MessageBase(MessageType::REQUEST_SENSOR_LAYER_CONNECT)
        , handle(0)
        , listener(nullptr)
        , period(0)
        , latency(0)
    {}

    uint32_t                             handle;
    SDK::Interface::ISensorDataListener* listener;
    float                                period;
    uint32_t                             latency;
};

struct RequestDisconnect : public MessageBase {
    RequestDisconnect()
        : MessageBase(MessageType::REQUEST_SENSOR_LAYER_DISCONNECT)
        , handle(0)
    {}

    uint32_t                             handle;
    SDK::Interface::ISensorDataListener* listener;
};

/**
 * @brief Sensor layer event message
 *
 */
struct EventData1 : public MessageBase {
    EventData1()
        : MessageBase(MessageType::EVENT_SENSOR_LAYER_DATA_1)
        , handle(0)
        , data{}
        , count(0)
        , stride(0)
    {}

    uint8_t           handle;
    SDK::Sensor::Data data[10];
    uint16_t          count;
    uint16_t          stride;
};

struct EventData2 : public MessageBase {
    EventData2()
        : MessageBase(MessageType::EVENT_SENSOR_LAYER_DATA_2)
        , handle(0)
        , data{}
        , count(0)
        , stride(0)
    {}

    uint8_t           handle;
    SDK::Sensor::Data data[20];
    uint16_t          count;
    uint16_t          stride;
};

struct EventData3 : public MessageBase {
    EventData3()
        : MessageBase(MessageType::EVENT_SENSOR_LAYER_DATA_3)
        , handle(0)
        , data{}
        , count(0)
        , stride(0)
    {}

    uint8_t           handle;
    SDK::Sensor::Data data[30];
    uint16_t          count;
    uint16_t          stride;
};

struct EventData4 : public MessageBase {
    EventData4()
        : MessageBase(MessageType::EVENT_SENSOR_LAYER_DATA_4)
        , handle(0)
        , data{}
        , count(0)
        , stride(0)
    {}

    uint8_t           handle;
    SDK::Sensor::Data data[40];
    uint16_t          count;
    uint16_t          stride;
};

} // namespace SDK::Message::Sensor
