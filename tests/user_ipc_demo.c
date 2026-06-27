#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "string.h"

int main(int argc, char** argv, char** envp) {
    (void)argc; (void)argv; (void)envp;
    printf("\n--- User-Space IPC System Verification ---\n");
    
    // 1. Pipe Operations
    printf("Configuring Pipe ID 2...\n");
    if (pipe_create(2) == 0) {
        const char* pipe_msg = "Data stream transmitted via User-Space Pipe 2!";
        pipe_write(2, pipe_msg, strlen(pipe_msg));
        
        char pipe_buffer[64];
        memset(pipe_buffer, 0, sizeof(pipe_buffer));
        int read_bytes = pipe_read(2, pipe_buffer, sizeof(pipe_buffer) - 1);
        printf("Retrieved from Pipe 2 (%d bytes): '%s'\n", read_bytes, pipe_buffer);
    } else {
        printf("Error: Could not allocate Pipe 2.\n");
    }
    
    // 2. Mailbox Operations
    printf("\nConfiguring Mailbox ID 2...\n");
    const char* mbox_msg = "Mailbox parcel dispatched from user-space!";
    if (mbox_send(2, mbox_msg, strlen(mbox_msg)) == 0) {
        char mbox_buffer[64];
        memset(mbox_buffer, 0, sizeof(mbox_buffer));
        int rcv_bytes = mbox_recv(2, mbox_buffer, sizeof(mbox_buffer) - 1);
        printf("Received via Mailbox 2 (%d bytes): '%s'\n", rcv_bytes, mbox_buffer);
    } else {
        printf("Error: Mailbox 2 access failed.\n");
    }
    
    printf("--- IPC Verification Completed ---\n");
    return 0;
}