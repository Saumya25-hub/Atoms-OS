#ifndef BOTREE_NAMESPACE_H
#define BOTREE_NAMESPACE_H

#include "botree_types.h"

// Virtual Shell Namespace URIs
#define BDE_URI_THIS_PC        "virtual://ThisPC"
#define BDE_URI_DESKTOP        "virtual://Desktop"
#define BDE_URI_DOCUMENTS      "virtual://Documents"
#define BDE_URI_DOWNLOADS      "virtual://Downloads"
#define BDE_URI_PICTURES       "virtual://Pictures"
#define BDE_URI_MUSIC          "virtual://Music"
#define BDE_URI_VIDEOS         "virtual://Videos"
#define BDE_URI_RECYCLE_BIN    "virtual://RecycleBin"
#define BDE_URI_NETWORK        "virtual://Network"
#define BDE_URI_USB            "virtual://USB"
#define BDE_URI_CONTROL_PANEL  "virtual://ControlPanel"
#define BDE_URI_SETTINGS       "virtual://Settings"

// Namespace Engine Public Functions
bool    BDe_NamespaceIsVirtual(const char* path);
int32_t BDe_NamespaceResolve(const char* virtual_uri, char* out_physical_path, size_t max_len);
int32_t BDe_NamespaceGetObjects(const char* namespace_uri, BDeDirEntry** out_entries, uint32_t* out_count);

#endif // BOTREE_NAMESPACE_H
