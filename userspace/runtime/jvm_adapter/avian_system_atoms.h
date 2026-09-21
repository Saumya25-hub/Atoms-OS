/*
 * ATOMS OS — Avian JVM Platform Adapter Header
 * Copyright © 2026 ATOMS OS Project / Saumya Chaudhari
 *
 * Implements the concrete platform bridge between the open-source
 * Avian C++ JVM core (avian::system::System) and the ATOMS OS
 * BOS Ring 3 Unified Runtime Foundation (atoms_runtime.h).
 */

#ifndef ATOMS_AVIAN_SYSTEM_ATOMS_H
#define ATOMS_AVIAN_SYSTEM_ATOMS_H

#include <avian/system/system.h>
#include <avian/system/memory.h>

namespace atoms {
namespace jvm {

avian::system::System* makeAtomsSystem();

} // namespace jvm
} // namespace atoms

#endif // ATOMS_AVIAN_SYSTEM_ATOMS_H
