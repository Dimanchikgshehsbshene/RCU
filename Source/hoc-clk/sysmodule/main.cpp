#include <switch.h>
#include <stratosphere.hpp>

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x8000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);
}

void __libnx_initheap(void) {
    void*  addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)addr;
    fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    R_ABORT_UNLESS(smInitialize());
    R_ABORT_UNLESS(fsInitialize());
    R_ABORT_UNLESS(fsdevMountSdmc());
    smExit();
}

void __appExit(void) {
    fsdevUnmountAll();
    fsExit();
}

int main(int argc, char** argv) {
    /* TODO: Add sysmodule logic here */

    /* Main loop */
    while (true) {
        svcSleepThread(1000000000ULL);
    }

    return 0;
}
