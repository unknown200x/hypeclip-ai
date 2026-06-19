#pragma once
// Small UI helpers: enumerate OBS audio sources / scenes / transitions and a
// shared dark stylesheet. Host builds return empty lists so UI code stays
// compile-clean without OBS.
#include <QString>
#include <QStringList>
#include <string>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs.h>
#endif
#if defined(HYPECLIP_FRONTEND)
#include <obs-frontend-api.h>
#endif

namespace hypeclip { namespace ui {

inline QStringList audioSources() {
    QStringList out;
#if defined(HYPECLIP_HAVE_OBS)
    struct Ctx { QStringList* o; } ctx{&out};
    obs_enum_sources([](void* p, obs_source_t* src) -> bool {
        auto* c = static_cast<Ctx*>(p);
        if (obs_source_get_output_flags(src) & OBS_SOURCE_AUDIO)
            c->o->append(QString::fromUtf8(obs_source_get_name(src)));
        return true;
    }, &ctx);
#endif
    return out;
}

inline QStringList sceneNames() {
    QStringList out;
#if defined(HYPECLIP_FRONTEND)
    char** names = obs_frontend_get_scene_names();
    if (names) { for (char** n = names; *n; ++n) out.append(QString::fromUtf8(*n)); bfree(names); }
#endif
    return out;
}

inline QStringList transitionNames() {
    QStringList out;
#if defined(HYPECLIP_FRONTEND)
    obs_frontend_source_list ts = {};
    obs_frontend_get_transitions(&ts);
    for (size_t i = 0; i < ts.sources.num; ++i)
        out.append(QString::fromUtf8(obs_source_get_name(ts.sources.array[i])));
    obs_frontend_source_list_free(&ts);
#endif
    if (out.isEmpty()) out << "Fade" << "Cut";   // sane fallback
    return out;
}

// Refined dark styling layered on top of OBS's theme.
inline const char* darkStyle() {
    return
    "QWidget{color:#d6d8de;font-size:12px;}"
    "QGroupBox{border:1px solid #2c2f37;border-radius:8px;margin-top:14px;padding:8px;}"
    "QGroupBox::title{subcontrol-origin:margin;left:10px;color:#9aa0aa;}"
    "QPushButton{background:#2a2d35;border:1px solid #3a3e48;border-radius:6px;padding:5px 12px;}"
    "QPushButton:hover{background:#343843;}"
    "QPushButton#primary{background:#2f8f5b;border:none;color:#fff;font-weight:600;}"
    "QPushButton#danger{background:#7a3030;border:none;color:#fff;}"
    "QCheckBox{spacing:7px;}"
    "QListWidget,QTableWidget,QComboBox,QSpinBox,QLineEdit{background:#1f2228;border:1px solid #2c2f37;border-radius:6px;}"
    "QTabBar::tab{padding:6px 12px;}";
}

}} // namespace hypeclip::ui
