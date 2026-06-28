#![no_std]

use core::panic::PanicInfo;

// We use #[allow(dead_code)] and remove 'pub' from constants we don't want 
// cbindgen to export to the C header, as they are already in pmm.h
const PAGE_SIZE: u32 = 4096;
const MEMORY_LIMIT: u32 = 64 * 1024 * 1024;
const TOTAL_BLOCKS: u32 = MEMORY_LIMIT / PAGE_SIZE;

#[repr(C)]
pub struct FrameValidation {
    pub bitmap_index: u32,
    pub bit_offset: u32,
    pub is_valid: bool,
}

/// Verification function used by kernel.cpp diagnostics
#[no_mangle]
pub extern "C" fn rust_verify_pmm(total_frames: u32, free_frames: u32) -> bool {
    if free_frames > total_frames {
        return false;
    }
    true
}

/// Spatial safety validator used by pmm.c
#[no_mangle]
pub extern "C" fn rust_validate_frame(addr: u32) -> FrameValidation {
    if addr >= MEMORY_LIMIT || addr % PAGE_SIZE != 0 {
        return FrameValidation { bitmap_index: 0, bit_offset: 0, is_valid: false };
    }

    let total_bit_index = addr / PAGE_SIZE;

    if total_bit_index >= TOTAL_BLOCKS {
        return FrameValidation { bitmap_index: 0, bit_offset: 0, is_valid: false };
    }

    FrameValidation {
        bitmap_index: total_bit_index / 32,
        bit_offset: total_bit_index % 32,
        is_valid: true,
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}