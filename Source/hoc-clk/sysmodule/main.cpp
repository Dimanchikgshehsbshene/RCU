#include <switch.h>

// Forward declarations for the actual module logic
extern "C" {
    void module_init();
    void module_exit();
}

extern "C" void __appInit(void) {
    // Initialize services needed by the sysmodule
    smInitialize();
    setsysInitialize();
    setcalInitialize();
    pmshellInitialize();
}

extern "C" void __appExit(void) {
    // Cleanup services
    pmshellExit();
    setcalExit();
    setsysExit();
    smExit();
}

extern "C" int main(int argc, char **argv) {
    // Initialize the module
    module_init();
    
    // Main loop - keep the sysmodule running
    while (true) {
        svcSleepThread(1000000000ULL); // Sleep for 1 second
    }
    
    // Cleanup (unreachable in normal operation)
    module_exit();
    return 0;
}
