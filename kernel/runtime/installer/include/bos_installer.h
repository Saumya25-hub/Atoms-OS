#ifndef BOS_INSTALLER_H
#define BOS_INSTALLER_H

#include <stdint.h>
#include <stdbool.h>

bool bos_install_package(const char* bosx_path);
bool bos_launch_installed_app(const char* identifier);

#endif /* BOS_INSTALLER_H */
