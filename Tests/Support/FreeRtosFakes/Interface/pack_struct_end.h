/* Stub of FreeRTOS-Plus-TCP's compiler-portable struct-packing helper.
 *
 * Real Plus-TCP installs ship per-compiler variants under
 * source/portable/Compiler/<GCC|IAR|MSVC|...>/. The GCC variant supplies
 * __attribute__((packed)) here so the preceding struct definition is closed
 * with a packing attribute. We provide a compiler-independent ';' instead -
 * it terminates the struct without imposing any packing, which is fine
 * because tests never run the IP stack and never observe the layout.
 *
 * See pack_struct_start.h for the broader rationale.
 */
;
