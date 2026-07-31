#include "framework/include/bos_ui.h"

extern void BOS_Theme_Init(void);
extern void BOS_Animation_Init(void);

void BOS_UI_Framework_Init(void) {
    BOS_Theme_Init();
    BOS_Animation_Init();
}
