/**
 * Host unit tests for SDK::Sensor::Connection — the app-side sensor-subscription
 * wrapper. These pin the fix for the field bug where a Running activity started
 * before GPS fix recorded a full activity (HR/speed/distance) but ZERO position:
 * the app's one GPS_LOCATION subscribe, issued during the busy app-startup
 * window, could lose the 100 ms ack round-trip and was never retried, so the app
 * ignored location for the whole session.
 *
 * The race can't be reproduced on demand on hardware, so we drive the client
 * logic deterministically here with a scripted comm that reproduces the exact
 * timeout/FAIL replies (sendMessage returns true even on a response timeout,
 * mirroring the device DualAppComm — see DualAppComm.cpp).
 */

#include <cstdint>

#include <gtest/gtest.h>

#include "KernelTestDoubles.hpp"

#include "SDK/Kernel/KernelProviderService.hpp"
#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/SensorLayerMessages.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"

using namespace SDK::TestSupport;

namespace {

// Kernel comm whose replies to the sensor subscribe/connect/disconnect messages
// are scripted per test, so we can reproduce the ack-timeout race the fix
// targets without any hardware or real IPC.
class ScriptedComm : public StubAppComm {
public:
    // Script: number of leading calls that "time out". A timeout returns true
    // (send succeeded, only the response timed out) with result != SUCCESS,
    // exactly as the device comm does.
    int      defaultTimeouts = 0;   // RequestDefault (subscribe) timeouts before success
    int      connectTimeouts = 0;   // RequestConnect timeouts before success
    bool     connectFails    = false;  // RequestConnect replies FAIL (not a timeout)
    uint32_t handle          = 42;  // handle a successful RequestDefault returns

    // Observability.
    int      defaultCalls         = 0;
    int      connectCalls         = 0;
    int      disconnectCalls      = 0;
    uint32_t lastDisconnectHandle = 0;

    void reset()
    {
        defaultTimeouts = 0;
        connectTimeouts = 0;
        connectFails    = false;
        handle          = 42;
        defaultCalls         = 0;
        connectCalls         = 0;
        disconnectCalls      = 0;
        lastDisconnectHandle = 0;
    }

    bool sendMessage(SDK::MessageBase* msg, uint32_t /*timeoutMs*/) override
    {
        using namespace SDK::Message::Sensor;
        switch (msg->getType()) {
        case SDK::MessageType::REQUEST_SENSOR_LAYER_GET_DEFAULT: {
            ++defaultCalls;
            if (defaultTimeouts > 0) {
                --defaultTimeouts;
                msg->setResult(SDK::MessageResult::TIMEOUT);
                return true;  // device returns true even on a response timeout
            }
            static_cast<RequestDefault*>(msg)->handle = handle;
            msg->setResult(SDK::MessageResult::SUCCESS);
            return true;
        }
        case SDK::MessageType::REQUEST_SENSOR_LAYER_CONNECT: {
            ++connectCalls;
            if (connectTimeouts > 0) {
                --connectTimeouts;
                msg->setResult(SDK::MessageResult::TIMEOUT);
                return true;
            }
            msg->setResult(connectFails ? SDK::MessageResult::FAIL
                                        : SDK::MessageResult::SUCCESS);
            return true;
        }
        case SDK::MessageType::REQUEST_SENSOR_LAYER_DISCONNECT: {
            ++disconnectCalls;
            lastDisconnectHandle = static_cast<RequestDisconnect*>(msg)->handle;
            msg->setResult(SDK::MessageResult::SUCCESS);
            return true;
        }
        default:
            return true;
        }
    }
};

// One kernel + scripted comm installed process-wide. KernelProviderService
// latches the first CreateInstance(), and Connection resolves its kernel from
// that singleton, so a single shared harness is the only workable shape; each
// test resets the comm's script.
struct Harness {
    StubSystem         system;
    StubLogger         logger;
    StubAppMemory      memory;
    ScriptedComm       comm;
    InMemoryFileSystem fs;
    SDK::Kernel        kernel;

    Harness()
        : kernel(system, logger, memory, comm, fs)
    {
        SDK::KernelProviderService::CreateInstance(&kernel);
    }
};

ScriptedComm& scriptedComm()
{
    static Harness h;  // constructed + installed on first use
    return h.comm;
}

constexpr SDK::Sensor::Type kGps = SDK::Sensor::Type::GPS_LOCATION;

}  // namespace

// Baseline: a clean subscribe+connect succeeds in one attempt.
TEST(SensorConnection, ConnectsWhenKernelSucceeds)
{
    ScriptedComm& comm = scriptedComm();
    comm.reset();

    SDK::Sensor::Connection conn(kGps, 1000, 1000);
    EXPECT_TRUE(conn.connect());
    EXPECT_TRUE(conn.isConnected());
    EXPECT_TRUE(conn.isValid());
    EXPECT_EQ(comm.defaultCalls, 1);
    EXPECT_EQ(comm.connectCalls, 1);
    EXPECT_TRUE(conn.matchesDriver(42));
}

// The field bug + fix. A subscribe (RequestDefault) timeout must leave the
// connection un-resolved (no handle, RequestConnect never sent) — and a later
// retry must fully recover it. Pre-fix the app never retried, so a lost startup
// subscribe stranded position data for the entire session.
TEST(SensorConnection, RecoversAfterSubscribeTimeout)
{
    ScriptedComm& comm = scriptedComm();
    comm.reset();
    comm.defaultTimeouts = 1;  // first RequestDefault times out, then succeeds

    SDK::Sensor::Connection conn(kGps, 1000, 1000);

    // First attempt: subscribe times out.
    EXPECT_FALSE(conn.connect());
    EXPECT_FALSE(conn.isConnected());
    EXPECT_FALSE(conn.isValid());
    EXPECT_EQ(comm.defaultCalls, 1);
    EXPECT_EQ(comm.connectCalls, 0);  // never reached RequestConnect

    // Retry (what the app's 1 Hz loop now does): subscribe + connect succeed.
    EXPECT_TRUE(conn.connect());
    EXPECT_TRUE(conn.isConnected());
    EXPECT_TRUE(conn.isValid());
    EXPECT_EQ(comm.defaultCalls, 2);
    EXPECT_EQ(comm.connectCalls, 1);
}

// The connect() `send() && ok()` fix. A RequestConnect whose reply only times
// out must leave isConnected() false so the caller retries. Pre-fix
// (`send() || ok()`) latched connected on a timed-out ack — the actual field
// failure mode — because send() returns true on a response timeout.
TEST(SensorConnection, ConnectTimeoutDoesNotLatchConnected)
{
    ScriptedComm& comm = scriptedComm();
    comm.reset();
    comm.connectTimeouts = 1;  // subscribe ok; first RequestConnect times out

    SDK::Sensor::Connection conn(kGps, 1000, 1000);

    EXPECT_FALSE(conn.connect());  // would be TRUE under the old `send() || ok()`
    EXPECT_FALSE(conn.isConnected());
    EXPECT_TRUE(conn.isValid());   // handle already resolved
    EXPECT_EQ(comm.defaultCalls, 1);
    EXPECT_EQ(comm.connectCalls, 1);

    // Retry connects without re-subscribing (handle is already held).
    EXPECT_TRUE(conn.connect());
    EXPECT_TRUE(conn.isConnected());
    EXPECT_EQ(comm.defaultCalls, 1);
    EXPECT_EQ(comm.connectCalls, 2);
}

// A FAIL reply to RequestConnect must likewise not latch connected.
TEST(SensorConnection, ConnectFailDoesNotLatchConnected)
{
    ScriptedComm& comm = scriptedComm();
    comm.reset();
    comm.connectFails = true;

    SDK::Sensor::Connection conn(kGps, 1000, 1000);
    EXPECT_FALSE(conn.connect());
    EXPECT_FALSE(conn.isConnected());
}

// The disconnect() hardening (review finding #1). disconnect() must send
// RequestDisconnect whenever a handle is held, even if a connect-ack timeout
// left mIsConnected false while the kernel registered the listener late.
// Pre-fix disconnect() gated on mIsConnected and leaked the listener — and any
// duty-cycled hardware behind it — until app stop.
TEST(SensorConnection, DisconnectReleasesHandleEvenIfConnectFlagFalse)
{
    ScriptedComm& comm = scriptedComm();
    comm.reset();
    comm.connectTimeouts = 99;  // every RequestConnect times out: handle held, flag false

    SDK::Sensor::Connection conn(kGps, 1000, 1000);
    EXPECT_FALSE(conn.connect());
    EXPECT_FALSE(conn.isConnected());
    EXPECT_TRUE(conn.isValid());

    conn.disconnect();
    EXPECT_EQ(comm.disconnectCalls, 1);  // would be 0 under the old mIsConnected gate
    EXPECT_EQ(comm.lastDisconnectHandle, 42u);
}
