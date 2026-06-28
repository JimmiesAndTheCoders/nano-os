#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct FrameValidation {
  uint32_t bitmap_index;
  uint32_t bit_offset;
  bool is_valid;
} FrameValidation;

/**
 * Verification function used by kernel.cpp diagnostics
 */
bool rust_verify_pmm(uint32_t total_frames, uint32_t free_frames);

/**
 * Spatial safety validator used by pmm.c
 */
struct FrameValidation rust_validate_frame(uint32_t addr);
