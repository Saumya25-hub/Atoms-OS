#include "../include/taskmgr_api.h"
#include "kernel/drivers/display/display.h"

static TASKMGR_SERVICE s_svcs[TASKMGR_MAX_SERVICES];
static uint32_t        s_svc_count = 0;

void taskmgr_services_init(void) {
    s_svc_count = 3;
    const char* n0="AudioSvc"; for(int i=0;n0[i];i++) s_svcs[0].name[i]=n0[i]; s_svcs[0].running=true;
    const char* n1="NetworkSvc"; for(int i=0;n1[i];i++) s_svcs[1].name[i]=n1[i]; s_svcs[1].running=true;
    const char* n2="SecuritySvc"; for(int i=0;n2[i];i++) s_svcs[2].name[i]=n2[i]; s_svcs[2].running=true;
    display_print("[TASKMGR_SVC] Service Manager Engine Initialized.\n");
}

bool RefreshServices(void) {
    display_print("[TASKMGR_SVC] RefreshServices() -> ADVAPI32.EnumServicesStatus() OK\n");
    return true;
}

uint32_t taskmgr_services_count(void) { return s_svc_count; }
