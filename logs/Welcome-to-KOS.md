# K-OS

## Welcome to KOS

Last time I made and tested the operating which does absolutely nothing. Now, I will be adding some string to it displaying "Welcome to KOS". 

So, before directly going into the details of code required to display a string first let us understand how does the memory works.

---
### CPU registers
---

All processors have a number of registers, which are really small pieces of memory that can be written and read very fast, and are built into the CPU.

Registers are of the following types:
1. **General Purpose Register:** it can be used for almost any purpose (RAX, RBX, RCX, RDX etc).

2. **Index Register:** generally used for storing the index or pointers (RSI, RDI).

3. **Program Counter:** it is a special register which keeps track of the memory location of ongoing instruction begins at (RIP).

4. **Segment Register:** it is used for tracking the currently active memory segment (CS, DS, ES, FS, GS, SS).

5. **Flag Registers:** it contains some special flags set by various instructions.

6. **Special Purpose Register:** these are CPU registers reserved for specific internal roles, not for general arithmetic or free use.

---
### Real Memory Model
---
The 8086 CPU had a 20-bit address bus, which approximately means about 1 MB of memory. At that time computers only had 64-128 KB of memory compared to which this was very huge. So, the engineers back then decided to use a segment and offset addressing scheme for addressing memory.

#### Segment:Offset Addressing Scheme:

Here we use two 16-bit values (1 segment and 1 offset). Each segment contains 64 KB of memory, where each byte can be accessed by using the offset value. Segments overlap every 16 bytes.

The above in mathematical term means:
```bash
linear_address= segment * 16 + offset
```
There are some special registers which are used to specify the actively used segments:

- **CS:** it contains the code, which is the segment the processor executes code from.
- **IP:** it is the program counter which only gives us the offset.
- **DS and ES:** these are data segments.
- **SS:** it contains the current stack register.

---
### BIOS Interrupt Calls
---

To display text on the screen, we use BIOS interrupt calls. The interrupt 0x10 is used for video services. By setting specific values in registers before calling this interrupt, we can perform various display operations.

For printing a single character:
- **AH = 0x0E:** Teletype output mode
- **AL:** Character to display
- **BH:** Page number (usually 0)
- **INT 0x10:** Triggers the BIOS video interrupt

---
### Implementation Changes
---

#### The `puts` Procedure

A new subroutine called `puts` was added to handle string printing. This procedure:

1. **Saves registers:** Pushes `si`, `ax`, and `bx` to preserve their values
2. **Loads characters:** Uses `lodsb` instruction to load the next byte from the address in `si` into `al`
3. **Checks for null terminator:** Tests if the character is 0 (end of string)
4. **Prints character:** Sets up registers for BIOS interrupt and calls `INT 0x10`
5. **Loops:** Continues until the null terminator is reached
6. **Restores registers:** Pops the saved registers and returns

#### Main Program Flow

The `main` procedure now:

1. **Initializes segments:** Sets `DS`, `ES`, and `SS` to 0 for proper memory addressing
2. **Sets up stack:** Points the stack pointer `SP` to 0x7C00
3. **Loads message:** Moves the address of `msg_hello` into `si`
4. **Calls puts:** Invokes the string printing routine
5. **Halts:** Enters an infinite loop to keep the system running

#### Key Instructions

- **lodsb:** Load string byte - loads a byte from the address in `si` into `al` and increments `si`
- **or al, al:** Tests if `al` is zero (null terminator check)
- **jz .done:** Jumps if zero flag is set (end of string reached)
- **%define endl:** Macro defining carriage return (0x0D) and line feed (0x0A) for newlines

#### Message Data

```nasm
msg_hello: db 'Welcome to KOS', endl, 0
```

This declares the string data followed by a newline sequence and a null terminator for the string termination.

---
### Code Structure Summary
---

The updated [src/main.asm](../src/main.asm) now includes:
- A reusable `puts` routine for string output
- Proper register initialization in `main`
- A greeting message displayed at boot
- Maintains the bootloader signature and padding

This implementation demonstrates fundamental concepts of bare-metal programming including interrupt handling, string processing, and BIOS interaction.
