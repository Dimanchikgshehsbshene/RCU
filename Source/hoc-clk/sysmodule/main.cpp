#include <switch.h>

extern "C" void __appInit(void)
{
    // Initialize services here if needed
}

extern "C" void __appExit(void)
{
    // Clean up services here if needed
}

int main(int argc, char **argv)
{
    // Main sysmodule logic goes here
    // This is a minimal entry point to satisfy the linker

    // Keep the sysmodule running
    while (true)
    {
        svcSleepThread(1000000000ULL);
    }

    return 0;
}
