#include "../include/advapi32_api.h"

static HANDLE g_event_log_counter = (HANDLE)0x6000;

HANDLE RegisterEventSourceA(LPCSTR lpUNCServerName, LPCSTR lpSourceName) {
    (void)lpUNCServerName; (void)lpSourceName;
    return (HANDLE)(g_event_log_counter++);
}

BOOL ReportEventA(HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR* lpStrings, LPVOID lpRawData) {
    (void)hEventLog; (void)wType; (void)wCategory; (void)dwEventID; (void)lpUserSid; (void)wNumStrings; (void)dwDataSize; (void)lpStrings; (void)lpRawData;
    return TRUE;
}

BOOL DeregisterEventSource(HANDLE hEventLog) {
    (void)hEventLog;
    return TRUE;
}
