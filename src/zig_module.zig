/// Verifies if a given physical memory block lies within the safe limits of the physical memory map.
export fn zig_verify_safety(addr: u32, size: u32) bool {
    const memory_limit: u32 = 64 * 1024 * 1024; // 64 MB standard limit
    
    // Check for overflow or out of bounds allocations
    if (addr >= memory_limit) return false;
    if (addr + size > memory_limit) return false;
    
    return true;
}
