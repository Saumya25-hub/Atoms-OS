#include "../Include/sds_database.h"
#include <stddef.h>

static const SDS_DatabaseEntry g_error_database[] = {
    {
        "BV-GR-0001", 
        "Framebuffer Pointer is NULL.", 
        "Initialize Video Driver before BOVISUAL_Init().",
        SDS_SEVERITY_FATAL,
        "Graphics",
        "WIKI-BV-0001"
    },
    {
        "RK-NAV-0003", 
        "Screen navigation target invalid.", 
        "Ensure the Screen ID exists in the active UI manifest.",
        SDS_SEVERITY_ERROR,
        "Rock Navigation",
        "WIKI-RK-0003"
    },
    {
        "AT-PAR-0012", 
        "Syntax Error in .bosvisual file.", 
        "Check line syntax in Atoms Visual Forge source for unclosed braces.",
        SDS_SEVERITY_ERROR,
        "ACE Parser",
        "WIKI-AT-0012"
    }
};

static const uint32_t g_database_count = sizeof(g_error_database) / sizeof(g_error_database[0]);

void SDS_Database_Init(void) {
}

bool SDS_Database_Lookup(const char* diagnostic_code, SDS_DatabaseEntry* out_entry) {
    if (!diagnostic_code || !out_entry) return false;

    for (uint32_t i = 0; i < g_database_count; i++) {
        const char* s1 = g_error_database[i].code;
        const char* s2 = diagnostic_code;
        bool match = true;
        
        while (*s1 && *s2) {
            if (*s1 != *s2) { match = false; break; }
            s1++; s2++;
        }
        if (match && *s1 == '\0' && *s2 == '\0') {
            *out_entry = g_error_database[i];
            return true;
        }
    }

    out_entry->code = diagnostic_code;
    out_entry->description = "Unknown Diagnostic Code.";
    out_entry->fix = "Refer to the SDS documentation for unregistered codes.";
    out_entry->default_level = SDS_SEVERITY_WARNING;
    out_entry->owner = "Unknown Module";
    out_entry->wiki_id = "N/A";
    
    return false;
}
