#include "../include/bsom_api.h"

BSOMObject* BSOM_CreateFileObject(const char* path) {
    return BSOM_CreateObject(path, BSOM_CLASS_FILE);
}

BSOMObject* BSOM_CreateFolderObject(const char* path) {
    return BSOM_CreateObject(path, BSOM_CLASS_FOLDER);
}
