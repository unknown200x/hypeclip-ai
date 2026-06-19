// HypeClip AI — OBS module entry point.
//
// Lifecycle:
//   obs_module_load        -> load config, register dock, hook frontend events
//   FINISHED_LOADING event  -> PipelineController::startup() (sources now exist)
//   EXIT event              -> PipelineController::shutdown()
//   obs_module_unload       -> final cleanup
//
#include <obs-module.h>
#include <plugin-support.h>   // provided by obs-plugintemplate (PLUGIN_VERSION)

#include "core/Config.hpp"
#include "core/PipelineController.hpp"
#include "core/Log.hpp"

#if defined(HYPECLIP_FRONTEND)
#include <obs-frontend-api.h>
#endif
#if defined(HYPECLIP_QT)
#include "ui/HypeClipDock.hpp"
#endif

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("hypeclip-ai", "en-US")

using namespace hypeclip;

#if defined(HYPECLIP_FRONTEND)
static void on_frontend_event(enum obs_frontend_event event, void*) {
    switch (event) {
        case OBS_FRONTEND_EVENT_FINISHED_LOADING:
            PipelineController::instance().startup();
            break;
        case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
        case OBS_FRONTEND_EVENT_RECORDING_STOPPED: {
            // End-of-session best-of reel — only if enabled (Priority #1 toggle).
            const auto s = Config::instance().get();
            if (s.masterEnabled && s.features.endOfStreamHighlights)
                PipelineController::instance().storage().writeBestOfPlaylist(10);
            break;
        }
        case OBS_FRONTEND_EVENT_EXIT:
            PipelineController::instance().shutdown();
            break;
        default: break;
    }
}
#endif

bool obs_module_load(void) {
    HC_INFO("Loading HypeClip AI %s", PLUGIN_VERSION);

    // Load configuration (Beginner defaults if no file yet).
    char* cfgPath = obs_module_config_path("hypeclip.ini");
    Config::instance().load(cfgPath);
    bfree(cfgPath);

#if defined(HYPECLIP_FRONTEND)
    obs_frontend_add_event_callback(on_frontend_event, nullptr);
#endif

#if defined(HYPECLIP_QT) && defined(HYPECLIP_FRONTEND)
    // Register the dockable UI. obs_frontend_add_dock_by_id (OBS 30+) keeps the
    // dock state persistent across restarts.
    HypeClipDock::registerDock();
#endif

    HC_INFO("HypeClip AI loaded");
    return true;
}

void obs_module_unload(void) {
#if defined(HYPECLIP_FRONTEND)
    obs_frontend_remove_event_callback(on_frontend_event, nullptr);
#endif
    PipelineController::instance().shutdown();
    HC_INFO("HypeClip AI unloaded");
}

const char* obs_module_name(void) { return "HypeClip AI"; }
const char* obs_module_description(void) {
    return "Automatic gaming highlight detection & instant replay. Never miss a moment.";
}
