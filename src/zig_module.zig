const std = @import("std");

const vfs = @cImport({
    @cInclude("vfs.h");
});

const VfsError = error{
    InvalidPath,
    NotFound,
    NotADirectory,
};

fn getMountAt(path: []const u8) ?*vfs.vfs_node_t {
    var i: usize = 0;
    // Cast mount_count to usize
    const count = @as(usize, @intCast(vfs.mount_count));
    while (i < count) : (i += 1) {
        // FIX: Cast the fixed-size array [128]u8 to a sentinel-terminated pointer [*:0]const u8
        const mount_path_ptr = @as([*:0]const u8, @ptrCast(&vfs.mount_table[i].path));
        const mount_path = std.mem.span(mount_path_ptr);
        
        if (std.mem.eql(u8, mount_path, path)) {
            return vfs.mount_table[i].root_node;
        }
    }
    return null;
}

fn resolvePathInternal(path_ptr: [*:0]const u8) VfsError!*vfs.vfs_node_t {
    const path = std.mem.span(path_ptr);

    if (path.len == 0 or path[0] != '/') return VfsError.InvalidPath;
    
    // Convert C pointers to Zig single-item pointers for field access
    var current_node: *vfs.vfs_node_t = vfs.vfs_root orelse return VfsError.NotFound;
    if (path.len == 1) return current_node;

    // Use tokenizeScalar for single-character delimiters
    var it = std.mem.tokenizeScalar(u8, path, '/');
    
    var path_buffer: [256]u8 = undefined;
    var path_len: usize = 0;

    while (it.next()) |token| {
        if (path_len + token.len + 1 > 256) return VfsError.InvalidPath;
        
        path_buffer[path_len] = '/';
        @memcpy(path_buffer[path_len + 1 ..][0..token.len], token);
        path_len += token.len + 1;
        
        const current_path = path_buffer[0..path_len];

        if (getMountAt(current_path)) |mount_node| {
            current_node = mount_node;
        } else {
            if (current_node.finddir == null) return VfsError.NotADirectory;
            
            var token_c: [64]u8 = undefined;
            if (token.len >= 63) return VfsError.InvalidPath;
            @memcpy(token_c[0..token.len], token);
            token_c[token.len] = 0;

            // FIX: Ensure finddir return and arguments are cast correctly
            const next_c_node = current_node.finddir.?(current_node, @as([*:0]const u8, @ptrCast(&token_c))) orelse return VfsError.NotFound;
            const next_node: *vfs.vfs_node_t = @ptrCast(next_c_node);

            if (current_node != vfs.vfs_root and (current_node.flags & vfs.VFS_MOUNTPOINT) == 0) {
                vfs.vfs_close(current_node);
            }
            current_node = next_node;
        }
    }

    return current_node;
}

export fn zig_vfs_resolve_path(path_ptr: [*:0]const u8) ?*vfs.vfs_node_t {
    return resolvePathInternal(path_ptr) catch return null;
}

export fn zig_verify_safety(addr: u32, size: u32) bool {
    const memory_limit: u32 = 64 * 1024 * 1024;
    if (addr >= memory_limit) return false;
    if (addr + size > memory_limit) return false;
    return true;
}

pub fn panic(msg: []const u8, error_return_trace: ?*std.builtin.StackTrace, ret_addr: ?usize) noreturn {
    _ = msg;
    _ = error_return_trace;
    _ = ret_addr;
    while (true) {
        asm volatile ("hlt");
    }
}