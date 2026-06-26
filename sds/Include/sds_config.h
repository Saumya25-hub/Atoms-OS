#ifndef SDS_CONFIG_H
#define SDS_CONFIG_H

// Global switch to enable or disable the SDS logging pipeline completely.
// If set to 0, all SDS macros resolve to no-ops, providing zero runtime overhead.
#ifndef SDS_ENABLE_DEBUG
#define SDS_ENABLE_DEBUG 1
#endif

// Global switch for assertions.
#ifndef SDS_ENABLE_ASSERTIONS
#define SDS_ENABLE_ASSERTIONS 1
#endif

// Max string lengths to prevent heap allocations
#define SDS_MAX_MESSAGE_LEN   256
#define SDS_MAX_FIX_LEN       256
#define SDS_MAX_FORMATTED_LEN 1024

#endif // SDS_CONFIG_H
