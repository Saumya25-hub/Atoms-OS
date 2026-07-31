#ifndef STUDIO_H
#define STUDIO_H

/* Main Umbrella Header for BOS Studio Visual IDE */

#include "studio_designer.h"
#include "studio_toolbox.h"
#include "studio_property_grid.h"
#include "studio_editor.h"
#include "studio_project.h"

BOS_Result  BOS_Studio_Initialize(void);
void        BOS_Studio_Shutdown(void);
const char* BOS_Studio_GetVersionString(void);

#endif /* STUDIO_H */
