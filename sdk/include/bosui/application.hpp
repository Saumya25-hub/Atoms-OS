#ifndef BOSUI_APPLICATION_HPP
#define BOSUI_APPLICATION_HPP

#include "platform/include/bos_runtime.h"

namespace bosui {

class Application {
public:
    Application(const char* name, const char* version = "1.0.0") {
        BOS_AppConfig cfg = {};
        cfg.app_name = name;
        cfg.version = version;
        BOS_InitApplication(&cfg, &m_app);
    }

    int Run() {
        if (!m_app) return -1;
        return (BOS_RunApplication(m_app) == BOS_SUCCESS) ? m_app->exit_code : -1;
    }

    static void Quit(int code = 0) {
        BOS_QuitApplication(nullptr, code);
    }

    BOS_App* GetNativeApp() const { return m_app; }

private:
    BOS_App* m_app{nullptr};
};

} // namespace bosui

#endif /* BOSUI_APPLICATION_HPP */
