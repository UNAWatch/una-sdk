#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#if defined(SIMULATOR)
    #include "touchgfx/canvas_widget_renderer/CanvasWidgetRenderer.hpp"
    #include "Windows.h"
    #include <chrono>
    #include <ctime>
#endif

Model::Model()
    : modelListener(0)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    LOG_INFO("Initializing Model: Setting up kernel reference and command processor callbacks.");
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);
    LOG_DEBUG("Model initialization complete: Callbacks for app lifecycle and custom messages are set.");

#if defined(SIMULATOR)
    LOG_INFO("Application is running through simulator! \n");

    std::string fileStoreDir = SDK::Simulator::KernelHolder::Get().getFsPath();
    LOG_INFO("Path to files created by app:\n"
        "       [%s]\n", fileStoreDir.c_str());

    LOG_INFO("\n"
        "       Keys:                       \n"
        "       ----------------------------\n"
        "       1   L1,                     \n"
        "       2   L2,                     \n"
        "       3   R1,                     \n"
        "       4   R2,                     \n"
        "       z   L1+R2                   \n"
    );
#endif
}

FrontendApplication& Model::application()
{
    LOG_DEBUG("Retrieving reference to the frontend application instance for GUI interactions.");
    return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
}

void Model::tick()
{
    LOG_DEBUG("Tick method called: Processing periodic updates and checking for GUI invalidation needs.");

    if (mInvalidate) {
        LOG_DEBUG("Invalidation flag is set: Resetting flag and triggering application redraw to update GUI.");
        mInvalidate = false;
        application().invalidate();
    } else {
        LOG_DEBUG("No invalidation needed: GUI is up-to-date.");
    }
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application: Initiating shutdown sequence.");
    // Cleanup resources

    LOG_DEBUG("Cleaning up resources: Unsetting app lifecycle and custom message callbacks to prevent further interactions.");
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(nullptr);

    LOG_INFO("Calling kernel system exit: Terminating the application process. Note: In simulator, this sets a flag and completes current loop.");
    mKernel.sys.exit(); // No return for real app

    // !!! For TouchGFX Simulator !!!
    // This function only sets a flag.
    // The current TouchGFX loop will be completed, meaning that depending
    // on where this function was called, Model::tick(), Model::handleKeyEvent(),
    // as well as handleTickEvent() and handleKeyEvent() for the
    // current screen will be called.
}

// IUserApp implementation
void Model::onStart()
{
    LOG_INFO("Application lifecycle event: onStart - Application is starting up, perform initial setup if needed.");
}

void Model::onResume()
{
    LOG_INFO("Application lifecycle event: onResume - Application is resuming from suspension, preparing to redraw the screen.");

    // Redraw screen
    LOG_DEBUG("Setting invalidation flag to trigger a full screen redraw upon next tick.");
    mInvalidate = true;
}

void Model::onStop()
{
    LOG_INFO("Application lifecycle event: onStop - Application is stopping, perform cleanup if necessary.");
}

void Model::onSuspend()
{
    LOG_INFO("Application lifecycle event: onSuspend - Application is being suspended, prepare for background operation.");
}

// Events from Service

bool Model::customMessageHandler(SDK::MessageBase *msg)
{
    LOG_DEBUG("Custom message handler invoked: Processing incoming message from service, type ID: %d", msg->getType());

    switch (msg->getType()) {
        /*
         * HR_VALUES Message Handler:
         * - Receives HR updates from service via SDK messaging
         * - Extracts heartRate and trustLevel from message
         * - Calls modelListener to update GUI display
         * To enable: Uncomment case, ensure HRValues struct defined in Commands.hpp,
         * and implement updateHR() in ModelListener/View
         */
        // case CustomMessage::HR_VALUES:  {
        //     LOG_DEBUG("Processing HR_VALUES message: Updating heart rate data in GUI.");
        //     auto* m = static_cast<CustomMessage::HRValues*>(msg);
        //     LOG_DEBUG("Heart rate: %.1f, Trust level: %.1f", m->heartRate, m->trustLevel);
        //     // modelListener->updateHR(m->heartRate, m->trustLevel);  // Update GUI
        // } break;

        case CustomMessage::IMAGE_LIST: {
            LOG_DEBUG("Processing IMAGE_LIST message: Received list of available images from service.");
            auto* m = static_cast<CustomMessage::ImageListMsg*>(msg);
            LOG_DEBUG("Image list contains %zu filenames, notifying model listener to update GUI.", m->filenames.size());
            modelListener->updateImageList(m->filenames);
            LOG_DEBUG("Model listener updated with image list.");
        } break;

        case CustomMessage::IMAGE_LOADED: {
            LOG_DEBUG("Processing IMAGE_LOADED message: Image loading result received.");
            auto* m = static_cast<CustomMessage::ImageLoadedMsg*>(msg);
            LOG_DEBUG("Image '%s' loaded with success: %s, notifying model listener.", m->filename.c_str(), m->success ? "true" : "false");
            modelListener->updateImageLoaded(m->filename, m->success);
            LOG_DEBUG("Model listener notified of image load status.");
        } break;

        case CustomMessage::IMAGE_METADATA: {
            LOG_DEBUG("Processing IMAGE_METADATA message: Metadata for image received.");
            auto* m = static_cast<CustomMessage::ImageMetadataMsg*>(msg);
            LOG_DEBUG("Metadata for '%s': %dx%d pixels, %u bytes, modified %s, render time %u ms. Updating GUI.", m->filename.c_str(), m->width, m->height, m->fileSize, m->lastModified.c_str(), m->renderTimeMs);
            modelListener->updateImageMetadata(m->filename, m->width, m->height, m->fileSize, m->lastModified, m->renderTimeMs);
            LOG_DEBUG("Model listener updated with image metadata.");
        } break;

        default:
            LOG_DEBUG("Unknown message type received: %d, no action taken.", msg->getType());
            break;
    }

    LOG_DEBUG("Custom message handler completed: Returning true to indicate message processed.");
    return true;
}

// IGuiBackend implementation
bool Model::receiveGuiEvent(AppType::B2GEvent::Data &data)
{
    LOG_DEBUG("receiveGuiEvent called: Backend-to-GUI events are not processed in this implementation, returning false.");
    // Not used in this direction
    return false;
}

bool Model::sendEventToBackend(const AppType::G2BEvent::Data &data)
{
    LOG_INFO("Sending GUI-to-backend event: Processing event variant to communicate with service layer.");
    // Send event to backend via kernel message
    std::visit([this](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AppType::G2BEvent::RequestImageList>) {
            LOG_DEBUG("Event type: RequestImageList - Requesting the list of available images from the service.");
            // Send message to service to request image list
            if (auto msg = mKernel.comm.allocateMessage<CustomMessage::RequestImageListMsg>()) {
                LOG_DEBUG("Message allocated successfully, sending RequestImageListMsg to service.");
                mKernel.comm.sendMessage(msg);
            } else {
                LOG_ERROR("Failed to allocate RequestImageListMsg: Unable to send request.");
            }
        } else if constexpr (std::is_same_v<T, AppType::G2BEvent::SelectImage>) {
            LOG_DEBUG("Event type: SelectImage - Selecting image '%s' for processing.", arg.filename.c_str());
            // Send select image message
            if (auto msg = mKernel.comm.allocateMessage<CustomMessage::SelectImageMsg>()) {
                msg->filename = arg.filename;
                LOG_DEBUG("Message allocated with filename '%s', sending SelectImageMsg to service.", msg->filename.c_str());
                mKernel.comm.sendMessage(msg);
            } else {
                LOG_ERROR("Failed to allocate SelectImageMsg for '%s': Unable to send selection.", arg.filename.c_str());
            }
        } else if constexpr (std::is_same_v<T, AppType::G2BEvent::RequestMetadata>) {
            LOG_DEBUG("Event type: RequestMetadata - Requesting metadata for image '%s'.", arg.filename.c_str());
            // Send request metadata message
            if (auto msg = mKernel.comm.allocateMessage<CustomMessage::RequestImageMetadataMsg>()) {
                msg->filename = arg.filename;
                LOG_DEBUG("Message allocated with filename '%s', sending RequestImageMetadataMsg to service.", msg->filename.c_str());
                mKernel.comm.sendMessage(msg);
            } else {
                LOG_ERROR("Failed to allocate RequestImageMetadataMsg for '%s': Unable to send metadata request.", arg.filename.c_str());
            }
        }
    }, data);
    LOG_DEBUG("Event processing completed: Returning true to indicate successful dispatch attempt.");
    return true;
}
