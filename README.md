# Exception_Handling_Cortex_M3_M4
Code to handle Cortex M3/M4 core exceptions, and some test functions.

This code was written for use on the STM32F413xx, but should be useful reference for other Cortex M3/M4 chips.

- Call exceptionsInit() at the very start of your main application.
- Replace KernelPrintf() with your own printf implementation.
