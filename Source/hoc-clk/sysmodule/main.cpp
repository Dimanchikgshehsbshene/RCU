#include <switch.h>

extern "C" {
    /* Called before main() */
    void __appInit(void) {
        /* Initialize services here if needed */
    }

    /* Called after main() returns */
    void __appExit(void) {
        /* Cleanup services here if needed */
    }
}

int main(int argc, char **argv) {
    /* Main application logic goes here */
    
    /* For a sysmodule, we typically want to run indefinitely */
    while (true) {
        /* Main loop */
        svcSleepThread(1000000000ULL); /* Sleep for 1 second */
    }
    
    return 0;
}