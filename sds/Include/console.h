#ifndef SDS_CONSOLE_H
#define SDS_CONSOLE_H

#include "sds_types.h"

// Formats the diagnostic object into the standard beautiful UI box structure
// and dispatches it to the Core writer.
void _SDS_Console_FormatAndPrint(const SDS_DiagnosticObject* obj);

#endif // SDS_CONSOLE_H
