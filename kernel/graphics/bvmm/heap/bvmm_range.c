#include "bvmm_range.h"
#include "kernel/core/memory/heap/include/heap.h"
#include "kernel/core/lib/include/string.h"

bvmm_result_t bvmm_range_init(uint64_t base_address, size_t size_bytes, bvmm_range_manager_t** out_mgr) {
    if (!out_mgr || size_bytes == 0) return BVMM_ERR_INVALID_ARGUMENT;

    bvmm_range_manager_t* mgr = (bvmm_range_manager_t*)kmalloc(sizeof(bvmm_range_manager_t));
    if (!mgr) return BVMM_ERR_OUT_OF_MEMORY;

    memset(mgr, 0, sizeof(bvmm_range_manager_t));
    mgr->base_address       = base_address;
    mgr->total_bytes        = size_bytes;
    mgr->free_bytes         = size_bytes;
    mgr->largest_free_block = size_bytes;

    bvmm_range_node_t* head = (bvmm_range_node_t*)kmalloc(sizeof(bvmm_range_node_t));
    if (!head) {
        kfree(mgr);
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    memset(head, 0, sizeof(bvmm_range_node_t));
    head->start_address   = base_address;
    head->size_bytes      = size_bytes;
    head->allocated       = false;
    head->canary_magic    = BVMM_CANARY_MAGIC;
    head->next            = NULL;
    head->prev            = NULL;

    mgr->head             = head;
    mgr->node_count       = 1;

    *out_mgr = mgr;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_range_alloc(bvmm_range_manager_t* mgr,
                               size_t size,
                               size_t align,
                               bvmm_alloc_policy_t policy,
                               uint64_t* out_offset) {
    if (!mgr || !out_offset || size == 0) return BVMM_ERR_INVALID_ARGUMENT;
    if (align == 0) align = 16;

    bvmm_range_node_t* curr = mgr->head;
    bvmm_range_node_t* best_candidate = NULL;
    size_t best_waste = (size_t)-1;

    while (curr) {
        if (!curr->allocated && curr->size_bytes >= size) {
            /* Compute alignment padding offset */
            uint64_t aligned_addr = (curr->start_address + (align - 1)) & ~(align - 1);
            uint64_t align_padding = aligned_addr - curr->start_address;

            if (curr->size_bytes >= size + align_padding) {
                if (policy == BVMM_POLICY_FIRST_FIT) {
                    best_candidate = curr;
                    break;
                } else if (policy == BVMM_POLICY_BEST_FIT || policy == BVMM_POLICY_EXACT_FIT) {
                    size_t waste = curr->size_bytes - (size + align_padding);
                    if (waste < best_waste) {
                        best_waste = waste;
                        best_candidate = curr;
                        if (waste == 0) break; /* Exact Fit */
                    }
                } else {
                    best_candidate = curr;
                }
            }
        }
        curr = curr->next;
    }

    if (!best_candidate) {
        return BVMM_ERR_OUT_OF_MEMORY;
    }

    uint64_t aligned_start = (best_candidate->start_address + (align - 1)) & ~(align - 1);
    uint64_t padding = aligned_start - best_candidate->start_address;

    /* Handle alignment padding node if padding exists */
    if (padding > 0) {
        bvmm_range_node_t* pad_node = (bvmm_range_node_t*)kmalloc(sizeof(bvmm_range_node_t));
        if (pad_node) {
            memset(pad_node, 0, sizeof(bvmm_range_node_t));
            pad_node->start_address   = best_candidate->start_address;
            pad_node->size_bytes      = padding;
            pad_node->allocated       = false;
            pad_node->canary_magic    = BVMM_CANARY_MAGIC;
            pad_node->prev            = best_candidate->prev;
            pad_node->next            = best_candidate;

            if (best_candidate->prev) {
                best_candidate->prev->next = pad_node;
            } else {
                mgr->head = pad_node;
            }
            best_candidate->prev = pad_node;
            best_candidate->start_address = aligned_start;
            best_candidate->size_bytes   -= padding;
            mgr->node_count++;
        }
    }

    /* Split best_candidate if remaining space exists */
    if (best_candidate->size_bytes > size) {
        bvmm_range_node_t* split_node = (bvmm_range_node_t*)kmalloc(sizeof(bvmm_range_node_t));
        if (split_node) {
            memset(split_node, 0, sizeof(bvmm_range_node_t));
            split_node->start_address = best_candidate->start_address + size;
            split_node->size_bytes    = best_candidate->size_bytes - size;
            split_node->allocated     = false;
            split_node->canary_magic  = BVMM_CANARY_MAGIC;
            split_node->next          = best_candidate->next;
            split_node->prev          = best_candidate;

            if (best_candidate->next) {
                best_candidate->next->prev = split_node;
            }
            best_candidate->next       = split_node;
            best_candidate->size_bytes = size;
            mgr->node_count++;
        }
    }

    best_candidate->allocated       = true;
    best_candidate->alignment_bytes = align;

    mgr->used_bytes += best_candidate->size_bytes;
    if (mgr->free_bytes >= best_candidate->size_bytes) {
        mgr->free_bytes -= best_candidate->size_bytes;
    }

    *out_offset = best_candidate->start_address;
    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_range_free(bvmm_range_manager_t* mgr, uint64_t offset, size_t size) {
    (void)size;
    if (!mgr) return BVMM_ERR_INVALID_ARGUMENT;

    bvmm_range_node_t* curr = mgr->head;
    bvmm_range_node_t* target = NULL;

    while (curr) {
        if (curr->allocated && curr->start_address == offset) {
            target = curr;
            break;
        }
        curr = curr->next;
    }

    if (!target) return BVMM_ERR_INVALID_ARGUMENT;

    target->allocated = false;
    mgr->used_bytes -= target->size_bytes;
    mgr->free_bytes += target->size_bytes;

    /* Merge right neighbor if free */
    if (target->next && !target->next->allocated) {
        bvmm_range_node_t* right = target->next;
        target->size_bytes += right->size_bytes;
        target->next = right->next;
        if (right->next) {
            right->next->prev = target;
        }
        kfree(right);
        mgr->node_count--;
    }

    /* Merge left neighbor if free */
    if (target->prev && !target->prev->allocated) {
        bvmm_range_node_t* left = target->prev;
        left->size_bytes += target->size_bytes;
        left->next = target->next;
        if (target->next) {
            target->next->prev = left;
        }
        kfree(target);
        mgr->node_count--;
        target = left;
    }

    /* Recalculate largest free block */
    curr = mgr->head;
    size_t largest = 0;
    while (curr) {
        if (!curr->allocated && curr->size_bytes > largest) {
            largest = curr->size_bytes;
        }
        curr = curr->next;
    }
    mgr->largest_free_block = largest;

    return BVMM_SUCCESS;
}

bvmm_result_t bvmm_range_query_largest(const bvmm_range_manager_t* mgr, size_t* out_largest) {
    if (!mgr || !out_largest) return BVMM_ERR_INVALID_ARGUMENT;
    *out_largest = mgr->largest_free_block;
    return BVMM_SUCCESS;
}

void bvmm_range_destroy(bvmm_range_manager_t* mgr) {
    if (!mgr) return;

    bvmm_range_node_t* curr = mgr->head;
    while (curr) {
        bvmm_range_node_t* next = curr->next;
        kfree(curr);
        curr = next;
    }

    kfree(mgr);
}
