/*
 * BOS OS — Phase 3: Production TLS & Security Engine
 * sec_test_suite.h — Automated Certification Test Suite Interface
 */

#ifndef SEC_TEST_SUITE_H
#define SEC_TEST_SUITE_H

#include "kernel/security/include/bos_security.h"

bos_sec_status_t sec_run_certification_test_suite(void);

#endif /* SEC_TEST_SUITE_H */
