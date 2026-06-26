#ifndef SDS_DATABASE_H
#define SDS_DATABASE_H

#include "sds_types.h"

// Initialize the database (internal)
void SDS_Database_Init(void);

// Lookup a diagnostic code in the central error registry.
// If found, populates out_entry and returns true.
// If not found, returns false and out_entry is set to default text.
bool SDS_Database_Lookup(const char* diagnostic_code, SDS_DatabaseEntry* out_entry);

#endif // SDS_DATABASE_H
