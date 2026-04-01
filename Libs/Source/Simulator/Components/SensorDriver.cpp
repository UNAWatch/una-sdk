/**
 ******************************************************************************
 * @file    SensorDriver.hpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor Driver class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/Simulator/Components/SensorManager.hpp"

#include <algorithm>

#define LOG_MODULE_PRX      "Sensor.Driver"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

/**
 * @brief Construct a new Sensor::Driver.
 *
 * @param sensor        Reference to the physical/virtual sensor implementation.
 * @param t             Sensor type identifier (SDK::Sensor::Type).
 * @param sampleLength  Number of data fields in one sensor sample.
 * @param control       Reference to sensor control callbacks (start/stop/update period/etc.).
 * @param mode          Delivery mode:
 *                      - Mode::PERIOD_BASED: sensor is polled on a periodic timer,
 *                        listeners request sampling period.
 *                      - Mode::EVENT_BASED: sensor produces data on events; period/latency are ignored.
 *
 * @details
 * The driver owns:
 * - A list of DataQueue objects, one per listener.
 * - A shared Sensor::Data buffer (mDataSample), reused for publishing.
 * - The synchronization primitive (mMutex).
 *
 * The driver does NOT own:
 * - The sensor itself (`sensor` reference).
 * - The control interface (`control` reference).
 */
Sensor::Driver::Driver(Interface::ISensor&        sensor,
                       SDK::Sensor::Type          t,
                       uint16_t                   fieldCount,
                       Sensor::ISensorDriverCtrl& control,
                       Mode                       mode)
    : mSensor(sensor)
    , mMutex()
    , mListeners()
    , mType(t)
    , mControl(control)
    , mDataSample(fieldCount)
    , mNewConnectionListener(nullptr)
    , mMode(mode)
    , mRefreshPeriod(1000)
    , mHandle(0)
{
}

/**
 * @brief Get the underlying sensor object.
 *
 * @return Reference to the ISensor implementation registered with this driver.
 */
Interface::ISensor& Sensor::Driver::getSensor()
{
    return mSensor;
}

/**
 * @brief Publish the current contents of mDataSample to listeners.
 *
 * @details
 * This is a convenience wrapper which calls pushData(mDataSample).
 * Expected usage: sensor updates mDataSample (timestamp + values) and then calls pushData().
 */
void Sensor::Driver::pushDataSample()
{
    mMutex.lock();

    if (mNewConnectionListener) {
        mNewConnectionListener->forceData(mDataSample.getData());
    } else {
        for (auto& l : mListeners) {
            l.pushData(mDataSample.getData());
        }
    }

    mMutex.unLock();
}

/**
 * @brief Connect a new listener (thread-safe wrapper).
 *
 * @param listener   Pointer to the consumer that will receive sensor data.
 * @param app        (Optional) App that owns this listener. May be locked/unlocked.
 * @param newPeriod  Requested sampling period in ms (0 = default).
 * @param latency    Requested delivery latency in ms (0 = default = same as period).
 *
 * @return true on success, false on error.
 *
 * @details
 * The method temporarily unlocks the owning app (if provided),
 * calls connectUnSafe(), then locks the app again.
 *
 * See connectUnSafe() for the main connection logic.
 */
bool Sensor::Driver::connect(SDK::Interface::ISensorDataListener* listener,
                             float                                newPeriod,
                             uint32_t                             latency)
{
    Sensor::DataQueue* newConnectionListener = nullptr;

    {
        mMutex.lock();

        // EVENT_BASED sensors ignore period/latency requests.
        if (mMode == Mode::EVENT_BASED) {
            newPeriod = mControl.sdcGetMinPeriod(this);
            latency   = static_cast<uint32_t>(mControl.sdcGetMinPeriod(this));
        }

        // Reject negative requested period.
        if (newPeriod < 0.0f) {
            mMutex.unLock();
            return false;
        }

        // Default period if caller gave 0.
        if (newPeriod == 0) {
            newPeriod = 1000;
        }

        // Enforce sensor's own min period.
        if (newPeriod < mControl.sdcGetMinPeriod(this)) {
            newPeriod = mControl.sdcGetMinPeriod(this);
        }

        // Enforce global lower bound.
        if (newPeriod < sMinPeriod) {
            newPeriod = sMinPeriod;
        }

        // Default latency: same as period.
        if (latency == 0) {
            latency = static_cast<uint32_t>(newPeriod);
        }

        ///////////////////////////////////////////////////////////////////////////////
        //// Check if this listener is already registered.
        //// This means it is reconnecting with new parameters, not a new connection.
        ///////////////////////////////////////////////////////////////////////////////

        auto queue = std::find_if(mListeners.begin(), mListeners.end(),
                               [listener](const DataQueue& q) {
                                   return q.getListener() == listener;
                               });

        if (queue != mListeners.end()) {
            // Listener already exists — update its queue and (if needed) the driver period.
            newPeriod = updatePeriod(newPeriod, latency);

            if (newPeriod <= 0.0f) {
                mMutex.unLock();
                return false;
            }

            queue->reinit(newPeriod, latency);
            mMutex.unLock();
            return true;
        }

        ///////////////////////////////////////
        //// New listener case
        ///////////////////////////////////////

        if (mListeners.size() == 0) {
            ///////////////////////////////////////
            //// It is the first listener
            ///////////////////////////////////////

            newPeriod = mControl.sdcStart(this, newPeriod);
            if (newPeriod <= 0) {
                LOG_ERROR("failed to start the driver\n");
                mMutex.unLock();
                return false;
            }

            mRefreshPeriod = calcUpdatePeriod(newPeriod, latency);

            Sensor::Manager::getInstance().regSensor(&mSensor);
        } else {
            ///////////////////////////////////////
            //// Sensor is already active: maybe
            //// tighten the global period.
            ///////////////////////////////////////

            newPeriod = updatePeriod(newPeriod, latency);

            if (newPeriod <= 0.0f) {
                mMutex.unLock();
                return false;
            }
        }

        // Actually append a new DataQueue for this listener.
        newConnectionListener = &mListeners.emplace_back(*this,
                                                         mDataSample.getFieldCount(),
                                                         listener,
                                                         newPeriod,
                                                         latency);
    } // <- mutex released here

    // Immediately feed snapshot data to the new listener if this is an event-based sensor.
    // The idea: a listener wants the "current state" right away (e.g. current battery level).
    if (mMode == Mode::EVENT_BASED) {
        mNewConnectionListener = newConnectionListener;
        mControl.sdcNewConnection(this);
        mNewConnectionListener = nullptr;
    }
    mMutex.unLock();
    return true;
}

/**
 * @brief Disconnect a listener from this driver.
 *
 * @param listener Pointer to the listener that should stop receiving data.
 *
 * @details
 * Steps:
 * 1. Lock mutex.
 * 2. Find the listener's DataQueue in mListeners.
 * 3. Remove that queue.
 * 4. Temporarily unlock the owning app (if any), call disconnectUnSafe()
 *    to update sensor state, then re-lock the app.
 *
 * Note:
 * If the last listener is removed, the driver will stop the sensor and
 * unregister it from Sensor::Manager.
 */
void Sensor::Driver::disconnect(SDK::Interface::ISensorDataListener* listener)
{
    mMutex.lock();

    auto it = std::find_if(mListeners.begin(), mListeners.end(),
                           [listener](const DataQueue& q) {
                               return q.getListener() == listener;
                           });

    if (it == mListeners.end()) {
        // Listener not found — nothing to do.
        mMutex.unLock();
        return;
    }

    // Remember the period currently associated with this queue.
    const float oldMinPeriod = it->getPeriod();

    mListeners.erase(it);

    // Last listener gone?
    if (mListeners.size() == 0) {
        Sensor::Manager::getInstance().unRegSensor(&mSensor);
        mControl.sdcStop(this);
        mMutex.unLock();
        return;
    }

    // There are still listeners.
    // Check if we can increase the sampling period.
    Sensor::DataQueue* queue = findListenerWithMinPeriod();
    float newMinPeriod = queue->getPeriod();
    if (oldMinPeriod < newMinPeriod) {
        applyPeriod(newMinPeriod, queue->getLatency());
    }

    mMutex.unLock();
}

float Sensor::Driver::calcUpdatePeriod(float period, uint32_t latency) const
{
    uint32_t capacity      = Sensor::DataQueue::computeCapacity(period, latency);
    float    refreshPeriod = capacity * period;

    LOG_DEBUG("P=%u C=%u\n", static_cast<uint32_t>(period), capacity);

    return refreshPeriod;
}

/**
 * @brief Get the SDK sensor type associated with this driver.
 *
 * @return Sensor type enum value.
 */
SDK::Sensor::Type Sensor::Driver::getType() const
{
    return mType;
}

float Sensor::Driver::getRefreshPeriod() const
{
    return mRefreshPeriod;
}

/**
 * @brief Get the number of registered listeners.
 *
 * @return Current size of mListeners as uint32_t.
 */
uint32_t Sensor::Driver::getCountOfListeners()
{
    mMutex.lock();

    auto value = static_cast<uint32_t>(mListeners.size());
    
    mMutex.unLock();

    return value;
}

/**
 * @brief Access the shared Sensor::Data buffer used for publishing.
 *
 * @return Reference to the internal mDataSample buffer.
 *
 * @details
 * Typical usage pattern:
 * - Sensor writes into getData(): timestamp + value fields.
 * - Sensor then calls pushData() to broadcast it.
 */
Sensor::DataSample& Sensor::Driver::getDataSample()
{
    return mDataSample;
}

/**
 * @brief Get the minimum allowed period for this sensor.
 *
 * @return Minimum period in ms, as reported by the control interface.
 *
 * @details
 * This delegates to ISensorDriverCtrl::sdcGetMinPeriod().
 * It represents the fastest sampling the sensor is allowed to run at.
 */
float Sensor::Driver::getMinPeriod()
{
    return mControl.sdcGetMinPeriod(this);
}

void Sensor::Driver::setHandle(uint8_t value)
{
    mHandle = value;
}

uint8_t Sensor::Driver::getHandle()
{
    return mHandle;
}

const char* Sensor::Driver::getDescription()
{
    return mControl.sdcGetDescription(this);
}

/**
 * @brief Find the listener with the smallest sampling period request.
 *
 * @return Pointer to the DataQueue with the minimal period, or nullptr if no listeners.
 *
 * @details
 * The "minimum period" is effectively the highest-priority / fastest
 * refresh request. The driver tries to honor the fastest requested rate
 * among all listeners.
 */
Sensor::DataQueue* Sensor::Driver::findListenerWithMinPeriod()
{
    if (mListeners.empty()) {
        assert(false);
        return nullptr;
    }

    auto minIt = mListeners.begin();
    for (auto it = std::next(mListeners.begin()); it != mListeners.end(); ++it) {
        if (it->getPeriod() < minIt->getPeriod()) {
            minIt = it;
        }
    }

    return &(*minIt);
}

float Sensor::Driver::applyPeriod(float requestedPeriod, uint32_t latency)
{
    const float adjustedPeriod = mControl.sdcUpdatePeriod(this, requestedPeriod);
    if (adjustedPeriod <= 0.0f) {
        LOG_ERROR("failed to update the driver period\n");
        return 0.0f;
    }

    const float refreshPeriod = calcUpdatePeriod(adjustedPeriod, latency);

    setPeriod(refreshPeriod);

    return adjustedPeriod;
}

/**
 * @brief Update the driver's global sampling period if a better value is requested.
 *
 * @param requestedPeriod Desired new sampling period in milliseconds.
 * @param latency         Maximum allowed latency in milliseconds used to derive
 *                        the internal refresh period.
 *
 * @return The effective period accepted by the driver (in milliseconds),
 *         or 0.0f if the update failed.
 *
 * @details
 * The method performs the following steps:
 * - Finds the listener with the minimal currently configured period.
 * - If the requested period is not shorter than this minimal listener period,
 *   no changes are applied and the requested value is returned.
 * - Otherwise, calls mControl.sdcUpdatePeriod() to let the sensor control
 *   logic validate and possibly adjust the requested period.
 * - If the control logic rejects the request (returns <= 0.0f), an error is
 *   logged and 0.0f is returned without modifying the current period.
 * - If the adjusted period matches the minimal listener period, the current
 *   configuration is already optimal and the adjusted value is returned.
 * - Otherwise, calcUpdatePeriod() is used with the adjusted period and
 *   the given latency to obtain the internal refresh period, which is then
 *   applied via setPeriod().
 */
float Sensor::Driver::updatePeriod(float requestedPeriod, uint32_t latency)
{
    Sensor::DataQueue* minListener = findListenerWithMinPeriod();

    const float minListenerPeriod = minListener->getPeriod();

    if (requestedPeriod >= minListenerPeriod) {
        return requestedPeriod;
    }

    return applyPeriod(requestedPeriod, latency);
}

/**
 * @brief Store the new current sampling period and notify Sensor::Manager.
 *
 * @param period New active period in ms for this driver/sensor.
 *
 * @details
 * After updating mPeriod, we call Sensor::Manager::updateRefreshPeriod()
 * so that the central manager can recalculate global scheduling
 * (for example, next wake-up deadlines).
 */
void Sensor::Driver::setPeriod(float period)
{
    mRefreshPeriod = period;

    Sensor::Manager::getInstance().updatePeriod();
}
