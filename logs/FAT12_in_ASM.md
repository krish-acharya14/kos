
# Working of FAT12 with C, C++ and Bootloader (Assembly) — Story Mode

---

## So where are we right now?

Till now we have already:

- Understood FAT12 layout  
- Written code in C  
- Rewritten it in C++  
- Successfully read files from disk image  

Now comes the real shift...

> Till now OS was helping us  
> Now... **we become the OS**

---

## Let’s change perspective

Earlier:

```text
You → Program → OS → Disk
```

Now:

```text
You → Bootloader → BIOS → Disk
```

No safety  
No library  
No abstraction  

Everything = manual

---

## What exactly is our Bootloader doing?

Think of it like this:

> “A tiny program whose only job is to bring the real OS (kernel) to life”

Steps:

1. Setup CPU environment  
2. Talk to BIOS  
3. Read disk sectors  
4. Understand FAT12  
5. Find `KERNEL.BIN`  
6. Load it into memory  
7. Jump to it  

---

## Entry Point — Where execution begins

```asm
org 0x7C00
bits 16
```

Why?

- BIOS loads bootloader at **0x7C00**
- CPU starts executing from here

So this is literally:

> “Birth point of our OS”

---

## First Problem — CPU State is Unknown

We cannot trust anything.

So we reset everything.

```asm
mov ax, 0
mov ds, ax
mov es, ax
```

 Now data access is predictable

---

## Stack Setup

```asm
mov ss, ax
mov sp, 0x7C00
```

Why here?

Because:

- Stack grows downward  
- Bootloader is small  
- Safe temporary region  

---

## Weird Trick — Why this exists?

```asm
push es
push word .after
retf
```

Looks strange right?

Here’s the story:

Some BIOS load code at:

```text
0000:7C00
```

Some at:

```text
07C0:0000
```

This fixes that inconsistency.

> Basically forcing CPU to correct address

---

## Saving Boot Drive

```asm
mov [ebr_drive_number], dl
```

BIOS already gave us:

```text
DL = drive number
```

We store it because:

> “We will need it again for disk reads”

---

## First Output Ever 

```asm
mov si, msg_loading
call puts
```

And boom...

```text
Loading...
```

Your OS just spoke 

---

## Talking to Disk (BIOS)

```asm
mov ah, 08h
int 13h
```

BIOS returns:

- sectors per track  
- number of heads  

We store it:

```asm
mov [bdb_sectors_per_track], cx
mov [bdb_heads], dh
```

---

## Now Think — Where is Root Directory?

We already know from theory:

```text
Root = Reserved + FATs
```

Assembly version:

```asm
mov ax, [bdb_sectors_per_fat]
mov bl, [bdb_fat_count]
mul bx
add ax, [bdb_reserved_sectors]
```

 This gives LBA of root directory

---

## Size of Root Directory

Each entry = 32 bytes

```asm
shl ax, 5
div word [bdb_bytes_per_sector]
```

If remainder exists:

```asm
inc ax
```

---

## Reading Root Directory

```asm
mov cl, al
mov bx, buffer
call disk_read
```

Now:

```text
buffer = root directory in RAM
```

---

## Searching for Kernel

Now comes the detective work 

```asm
"KERNEL  BIN"
```

Why like this?

Because FAT uses:

```text
8 + 3 format (no dot, padded spaces)
```

Comparison:

```asm
repe cmpsb
```

 Compare 11 bytes

---

## FOUND IT 

```asm
mov ax, [di + 26]
```

Offset 26 = starting cluster

We store it:

```asm
kernel_cluster = ax
```

---

## Load FAT Table

```asm
call disk_read
```

Now:

```text
buffer = FAT table
```

---

## The Real Game Begins — Loading Kernel

Loop:

```asm
cluster → sector → read → next cluster
```

---

## Cluster to Sector Conversion

```text
sector = data_start + (cluster - 2)
```

Here simplified:

```asm
add ax, 31
```

---

## Reading Data

```asm
call disk_read
```

Then move forward:

```asm
add bx, [bdb_bytes_per_sector]
```

---

## FAT Magic (Most Important Part)

```asm
fatIndex = cluster * 3 / 2
```

Why?

Because FAT12 = 12-bit entries

---

## Even vs Odd Cluster

```text
Even → lower 12 bits  
Odd  → upper 12 bits
```

Assembly:

```asm
and ax, 0x0FFF
shr ax, 4
```

---

## End Condition

```asm
cmp ax, 0x0FF8
```

If ≥ → end of file

---

## Final Step — Jump to Kernel

```asm
jmp 0x2000:0
```

This is HUGE.

> Control leaves bootloader  
> Kernel takes over  

---

## Error Handling

If something fails:

```asm
print message → wait key → reboot
```

---

## Printing Function (puts)

```asm
int 0x10
```

BIOS video interrupt

Prints characters one by one

---

## Disk Read

```asm
int 13h
```

This is your:

```text
fread() of assembly world
```

---

## LBA to CHS Conversion

BIOS doesn’t understand LBA.

So we convert:

```text
LBA → Cylinder, Head, Sector
```

---

## Boot Signature

```asm
dw 0xAA55
```

Without this:

```text
BIOS: "Not bootable"
```

---

## Big Picture (Very Important)

We just did:

```text
C → logic
C++ → structure
Assembly → execution
```
