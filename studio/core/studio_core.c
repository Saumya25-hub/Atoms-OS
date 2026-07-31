#include "studio/include/studio.h"

extern void BOS_Studio_ToolboxInit(void);
extern void BOS_Studio_PropertyGridInit(void);
extern void BOS_Studio_CodeEditorInit(void);

BOS_Result BOS_Studio_Initialize(void) {
    BOS_Studio_ToolboxInit();
    BOS_Studio_PropertyGridInit();
    BOS_Studio_CodeEditorInit();
    return BOS_SUCCESS;
}

void BOS_Studio_Shutdown(void) {
    /* Cleanup IDE resources */
}

const char* BOS_Studio_GetVersionString(void) {
    return "BOS Studio Visual IDE v1.0.0-PHASE5";
}
