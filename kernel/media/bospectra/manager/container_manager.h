/*
 * BOSPECTRA V3 — Container Manager
 * kernel/media/bospectra/manager/container_manager.h
 *
 * Automatic container probing, driver registration, file validation, and stream discovery.
 */

#ifndef BOSPECTRA_CONTAINER_MANAGER_H
#define BOSPECTRA_CONTAINER_MANAGER_H

#include "../include/bospectra_types.h"
#include "../include/bospectra_errors.h"
#include "../container/registry/container_registry.h"
#include "../file/bospectra_file.h"
#include <stdint.h>
#include <stdbool.h>

void bospectra_container_manager_init(void);
void bospectra_container_manager_shutdown(void);

const BOSPECTRA_ContainerDriver* bospectra_container_auto_probe(bospectra_file_id_t file_id);

#endif /* BOSPECTRA_CONTAINER_MANAGER_H */
