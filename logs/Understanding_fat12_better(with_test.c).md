# Working of FAT 12 with C and C++

So, for this part we will do two things simulataneously, i.e, undertand the mechanism used by FAT12 file system and write the code for same. First we will understand the code provided by Nanobyte in C and then write our own in C++.

---

## FAT DISK Layout

In general FAT disk is divided into 4 regions.

### 1. Reserved Region

The reserved region begins with the boot sector, which contains important parameters such as:

- Byter per sector
- Sectors per cluster
- Sizes and locations of the other regions
- Volume ID and serial number

Our bootloader is also stored in this region.

FAT32 introduces an additional **FS Information Sector**, but this does not exist in FAT12 or FAT16.

---

### 2. File Allocation Table (FAT) Region

This region contains two copies of the File Allocation Table.

Further discussion over it will be done later.

---

### 3. Root Directory Region

This region contains the **root directory**, which is essentially the table of contents of the disk.

Each directory entry contains:

- File name
- File attributes
- First cluster number
- File size
- Other metadata

---

### 4. Data Region

The data region contains the actual contents of files and all subdirectories.

---

## Reading a File 

To understand this topic lets go with an example, we need to look for a particular file called `TEST.TXT` in our disk. The question over here is how does our OS finds that very file and read it?

As seen above we know that all the metadata including the location of file is present in the *Root Directory Region* which will lead our OS to locate and read the file. We now need to first locate the *Root Directory* on the disk and then proceed further (i.e., Start and End of root directory).

### Locating the Root Directory

The root directory is the third region on the disk. The first two regions are the reserved region and the FAT region.

From the boot sector:

- Reserved sectors = 1
- Number of FATs = 2
- Sectors per FAT = 9

The FAT region size is
```
2 x 9 = 18 sectors
```

So the root directory begins at sector:

```c
1 + 18 = 19
```

sector 19.

### Root Directory Size

Each directory entry is 32 bytes long and the directory entry count is 222.

Therefore, total size:
```
222 x 32 = 7168 bytes
```

With 512 bytes per sector:
```
7168 / 512 = 14
```
So we need to read 14 sectors to load the entire root directory.

---

## Directory Entries 

Directory entries store file names in the old **8.3 format**, which allows only 11 characters.

Long file names were introduced in Windows 95 using special directory entries. We will not handle them here, as 8.3 names are more than enough for loading the *kernal*.

---

## Cluster and Sector Calculation

FAT does not know waht a **sector** is, but with **clusters** we can make the job done.

Cluster number to Sector number conversion:

```
sector = data_region_start + (cluster_number-2) x sectors_per_cluster
```

Let us say:

- Data region start = 33
- Cluster number = 3
- Sectors per cluster = 2

```
sector = 33 + (3-2) x 2 = 35
```

So, we now read cluster 3 starting at sector 35.

---

## File Allocation Table

The File Allocation Table (FAT) tells us which cluster comes next.

- FAT12 entries are 12 bits wide 
- FAT16 entries are 16 bits wide
- FAT32 entries are 32 bits wide

If a cluster entry contains a value **≥ 0xFF8**, that marks the end of the file.

---

### Reading Files in Subdirectories

Directories are stored just like files. Their contents follow the same structure as the root directory.

To resolve a part:

1. Split the path into components
2. Start at the root directory 
3. Search for each component
4. Continue until the file is found

---

## Implementing FAT in C

Before writing assembly code, we implement the FAT reader in **C** (later in C++). This allows easier testing and debugging.

The progam:

1. Reads the boot sector
2. Reads the FAT
3. Reads the root directory
4. Finds a file 
5. Follows the cluster chain 
6. Prints the file contents

### Writing and understanding the code

``` c
#include<stdio.h>
#include<stdint.h> //Fixed width types (eg.: uint8_t, uint16_t) critical for disk formats
#include<stdlib.h> //For malloc and free
#include<string.h> //for memcmp
#include<ctype.h> //for type checking (here printable ASCII check)
```
Disk structures must use exact byte sizes, so stdint.h is mandatory.

But why do we need specific *uint8_t* or *uint16_t*, why not just use char, int and long?

This is because size of generally used keywords is not specific and keeps on varying system to system. Disk formats are binay contracts and if the size are not defined critically then

- It may read the wrong bytes
- Misalign the structure 
- Everything after becomes garbage

#### Difference between uint and int

`uint` -> unsigned integer

`int` -> signed integer

In our FAT we will be using `uint` most of the time because disk values like sector counts, cluster numbers, sizes etc cannot be negative.
---

Coming back to the code 

``` c
typedef uint8_t bool;
#define true 1
#define false 0
```

- C(pre C-99 style) doesn't have a native bool
- Uses uint8_t to ensure 1 byte
- Important when passing values around cleanly

---
#### FAT12 Boot Sector Structure
```c
typedef struct{
    uint8_t BootJumpInstruction[3]; // First 3 bytes -> CPU jump instruction
    uint8_t OemIdentifier[8];       //OEM identifier
    uint16_t BytesPerSector;        //Sector size (generally 512)
    uint8_t SectorsPerCluster;      //Cluster= group of sectors
    uint16_t ReservedSectors;       //Usually 1 (boot sector itself)
    uint8_t FatCount;               //Number of FAT copies (usually 2)
    uint16_t DirEntryCount;         //Number of root directory entries
    uint16_t TotalSectors;          //Total sector count
    uint8_t MediaDescriptorType;    //Media ID (0xF0 for floppy)
    uint16_t SectorsPerFat;         //Size of one FAT table
    uint16_t SectorsPerTrack;       
    uint16_t Heads;                 //Geometry info
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;      //Used when disk > 65535 sectors

    //Extended Boot Record
    uint8_t DriveNumber;            //0x00 for floppy and 0x80 for HDD
    uint8_t _Reserved;
    uint8_t Signature;              //Signature must be 0x29
    uint32_t VolumeId;              //Volume serial number
    uint8_t VolumeLabel[11];        //Volume label padded with space
    uint8_t SystemId[8];            //Filesystem ID string("FAT12    ")
} __attribute__((packed)) BootSector;

```
In above code, addition of packed attribute is very crucial because it

- Prevents compiler from adding padding.
- Disk layout ≠ C memory layout unless packed.

---

#### FAT Directory Entry Structure

```c
typedef struct{
    uint8_t Name[11];                //Filename (No null terminator)
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;      //Creation time precision
    uint16_t CreatedTime;           
    uint16_t CreatedDate;
    uint16_t AccessdDate;           //FAT timestamp encoding
    uint16_t FirstClusterHigh;      //Used only in FAT32 (ignored here)
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterlow;       //Starting cluster of the file
    uint32_t Size;                  //File size in bytes
} __attribute__((packed)) DirectoryEntry;

```

So, in the above section we are storing the metadata of the file. File name over here consists of 11 characters (padded with space) which was default in FAT12 system.

---

#### Global Variables

```c
BootSector g_BootSector;
uint8_t* g_Fat = NULL;

DirectoryEntry* g_RootDirectory = NULL;
uint32_t g_RootDirectoryEnd;
```

This here holds the parsed disk structures and g_RootDirectoryEnd loads the first data cluster *LBA*(Logical Block Address).

---

#### Read Boot Sector

```c
bool readBootSector(FILE* disk){
    return fread(&g_BootSector, sizeof(g_BootSector), 1, disk) > 0;
}
```

It reads exactly one boot sectory directly into the `BootSector` struct.

---

#### Read Arbitrary Sectors

```c
bool readSectors(FILE* disk, uint32_t lba, uint32_t count, void* bufferout){
    bool ok = true;
    ok = ok && (fseek(disk, lba * g_BootSector.BytesPerSector, SEEK_SET) == 0);
    ok = ok && (fread(bufferOut, g_BootSector.BytesPerSector, count, disk) == count);
    return ok;
}
```

Over here three important steps are being carried out

1. Converts LBA -> byte offset.
2. `fseek` Jumps to correct disk location.
3. `fread` Reads count sectors.

---

#### Read FAT Table

```c
bool readFat(FILE* disk){
    g_Fat = (uint8_t*) malloc(g_BootSector.SectorPerFat* g_BootSector.BytesPerSector);
    return readSectors(disk, g_BootSector.ReservedSectors, g_BootSector,SectorsPerFat, g_Fat);
}
```

1. `malloc` allocates full FAT table(i.e., FAT table is fully loaded into **RAM**).
2. FAT starts right after boot sector.

---

#### Read Root Directory

```c
bool readRootDirectory(File* disk){
    uint32_t lba = g_BootSector.ReservedSectors + g_BootSector.SectorsPerFat * g_BootSector.FatCount;
    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    uint32_t sectors = (size / g_BootSector.BytesPerSector);
    if(size % g_BootSector.BytesPerSector > 0)
        sectors++;
    
    g_RootDirectoryEnd = lba + sectors;
    g_RootDirectory = (DirectoryEntry*) malloc(sectors*g_BootSector.BytesPerSector);
    return readSectors(disk, lba, sectors, g_RootDirectory);
}
```

``` csharp
[Boot]
[FAT1]
[FAT2]
[ROOT DIR]  ← here
[DATA]
```

- The size variable above here sotes the size of **Root Directory** in bytes.
- Then using `if` condition we seek for the First data cluster.

---

#### Find File by Name

```c
DirectoryEntry* findFile(const char* name){
    for(uint32_t i = 0 ; i < g_BootSector.DirEtryCount; i++){
        if(memcmp(name, g_RootDirectory[i].Name, 11) == 0)
            return &g_RootDirectory[i];
    }

    return NULL;
}
```

FAT filenames are:

- 8 characters name
- 3 characters extension
- No *Dot*
- No *NULL*
- Padded with *Spaces*

So, `memcmp` compares raw 11 bytes and if it matches then returns the pointer to that directory entry.

---

#### Read File

```c
bool readFile(DIrectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer){
    bool ok = true;
    uint16_t currentCluster = fileEntry -> FirstClusterLow;
    do{
        uint32_t lba = g_RootDirectoryEnd + (currentCluster - 2)* g_BootSector.SectorsPerCluster;
        ok = ok && readSectors(disk, lba, g_BootSector.SectorsPerCluster, outputBuffer);
        outputBuffer += g_BootSector.SectorsPerCluster * g_BootSector.BytePerSector;
        
        uint32_t fatIndex = currentCluster * 3 / 2;
        if(currentCluster % 2 == 0)
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
        else
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
    } while(ok && currentCluster < 0x0FF8);

    return ok;
}
```

- `currentCluster` starts the cluster chain
- Why `-2`? -> FAT clusters start at 2 and cluster 0 & 1 are reserved
- FAT 12 uses 12 bit entries so every 2 cluster = 3 bytes
- Event cluster -> lower 12 bits
- Odd cluster -> upper 12 bits
- After `0x0FF8` its end of chain markers

---

#### Main fuction

```c 
int mian(int argc, char** argv){
    if(argc < 3){
        printf("Syntax: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FILE* disk = fopen(argv[1], "rb");
    if(!disk){
        fprintf(stderr, "Cannot open disk image %s!\n", argv[1]);
        return -1;
    }

    if(!readBootSector(disk)){
        fprintf(stderr, "Could not read boot sector!\n");
        return -2;
    }

    if(!readFat(disk)){
        fprints(stderr, "Could not read FAT!\n");
        free(g_FAT);
        return -3;
    }

    if(!readRootDirectory(disk)){
        fprintf(stderr, "Could not read FAT!\n");
        free(g_Fat);
        free(g_RootDirectory);
        return -4;
    }

    DirectoryEntry* fileEntry = findFile(argv[2]);
    if (!fileEntry){
        fprintf(stderr, "Could not find file %s!\n", argv[2]);
        free(g_Fat);
        free(g_RootDirectory);
        return -5;
    }

    uint8_t* buffer = (uint8_t*) malloc(fileEntry->Size + g_BootSector.BytesPerSector);
    if (!readFile(fileEntry, disk, buffer)){
        fprintf(stderr, "Could not read file %s!\n", argv[2]);
        free(g_Fat);
        free(g_RootDirectory);
        free(buffer);
        return -5;
    }

    for (size_t i = 0; i < fileEntry->Size; i++){
        if (isprint(buffer[i])) fputc(buffer[i], stdout);
        else printf("<%02x>", buffer[i]);
    }
    printf("\n");

    free(buffer);
    free(g_Fat);
    free(g_RootDirectory);
    return 0;
}
```

**1. Argument Validation**

The program checks whether at least 3 arguments are provided.

If fewer arguments are given: - It prints the correct syntax. - It exits
with error code `-1`.

This prevents accessing invalid memory if required arguments are
missing.

2. Opening the Disk Image

The disk image is opened in binary read mode.

Purpose: - To access raw disk bytes without modification.

If opening fails: - An error message is printed. - The program exits
safely.

Binary mode is necessary because disk images contain raw sector data.

3. Reading the Boot Sector

The function `readBootSector(disk)` is called.

This: - Reads the first sector of the disk. - Stores it in the global
`g_BootSector` structure.

The boot sector contains critical filesystem metadata such as: - Bytes
per sector - Sectors per FAT - Root directory size - Number of FATs

Without this information, the filesystem layout cannot be determined.

If reading fails: - An error message is printed. - The program exits.

 4. Reading the FAT Table

The function `readFat(disk)` is called.

This: - Allocates memory. - Loads the File Allocation Table (FAT) into
RAM.

The FAT table is required to: - Follow cluster chains. - Determine where
file data continues. - Detect end-of-file markers.

If this step fails: - Allocated memory is freed. - The program exits
safely.

 5. Reading the Root Directory

The function `readRootDirectory(disk)` is called.

This: - Calculates the root directory location. - Allocates memory. -
Loads directory entries into memory.

Each directory entry contains: - File name (11-byte FAT format) - File
size - Starting cluster - File attributes

Without this step, the program cannot locate files.

6. Searching for the Requested File

The function `findFile(argv[2])` searches the root directory.

It: - Iterates through directory entries. - Compares each 11-byte
filename.

If the file is not found: - An error message is printed. - Memory is
freed. - The program exits.

This step identifies the file's starting cluster and size.

7. Allocating Memory for File Content

Memory is allocated using:

`malloc(fileEntry->Size + BytesPerSector)`

The allocation size includes: - The file size. - One extra sector as a
safety margin.

The extra space ensures safe cluster-based reading without overflow.

8. Reading the File Data

The function `readFile(fileEntry, disk, buffer)` is called.

It performs the following steps:

1. Reads the starting cluster.
2. Converts cluster number to sector number.
3. Reads cluster data into memory.
4. Uses FAT table to find the next cluster.
5. Continues until End-of-Chain marker is reached.

This reconstructs the complete file in memory.

If reading fails: - Memory is freed. - The program exits.



9. Printing File Contents

A loop runs from `0` to `fileEntry->Size`.

For each byte: - If printable ASCII → print normally. - If non-printable
→ print in hexadecimal format `<xx>`.

This ensures binary characters do not corrupt terminal output.


10. Printing a Newline

After printing file contents, a newline is printed for formatting.


11. Freeing Allocated Memory

The program frees: - File buffer - FAT memory - Root directory memory

This prevents memory leaks.



12. Program Termination

The function returns `0`.

This indicates successful execution.

## Writing code in `C++`

As we have already written our code in C and understood it, writing the same in `C++` should not be that tough. So, without going into much detail I'm putting down the code:

```cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cctype>
#include <iomanip>

using namespace std;

#pragma pack(push, 1)

struct BootSector {
    uint8_t BootJumpInstruction[3]; 
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t TotalSectorsLarge;

    uint8_t DriveNumber;
    uint8_t Reserved;
    uint8_t Signature;
    uint32_t VolumeID;
    uint8_t VolumeLabel[11];
    uint8_t FileSystemType[8];
};

struct DirectoryEntry {
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenth;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
};

#pragma pack(pop)

BootSector g_BootSector;
vector<uint8_t> g_Fat;
vector<DirectoryEntry> g_RootDirectory;
uint32_t g_RootDirectoryEnd;

// Convert "file.txt" → "FILE    TXT"
void formatFileName(const char* input, char output[11]) {
    memset(output, ' ', 11);

    int i = 0, j = 0;

    // name part
    while (input[i] && input[i] != '.' && j < 8) {
        output[j++] = toupper(input[i++]);
    }

    // skip dot
    if (input[i] == '.') i++;

    j = 8;

    // extension
    while (input[i] && j < 11) {
        output[j++] = toupper(input[i++]);
    }
}

// Read Boot Sector
bool readBootSector(ifstream& disk) {
    disk.read(reinterpret_cast<char*>(&g_BootSector), sizeof(g_BootSector));
    return !disk.fail();
}

// Read sectors
bool readSectors(ifstream& disk, uint32_t lba, uint32_t count, uint8_t* buffer) {
    disk.seekg(lba * g_BootSector.BytesPerSector, ios::beg);
    disk.read(reinterpret_cast<char*>(buffer), count * g_BootSector.BytesPerSector);
    return !disk.fail();
}

// Read FAT
bool readFat(ifstream& disk) {
    uint32_t size = g_BootSector.SectorsPerFat * g_BootSector.BytesPerSector;
    g_Fat.resize(size);
    return readSectors(disk, g_BootSector.ReservedSectors,
                       g_BootSector.SectorsPerFat, g_Fat.data());
}

// Read Root Directory
bool readRootDirectory(ifstream& disk) {
    uint32_t lba = g_BootSector.ReservedSectors +
                   g_BootSector.SectorsPerFat * g_BootSector.FatCount;

    uint32_t size = sizeof(DirectoryEntry) * g_BootSector.DirEntryCount;
    uint32_t sectors = size / g_BootSector.BytesPerSector;

    if (size % g_BootSector.BytesPerSector)
        sectors++;

    g_RootDirectoryEnd = lba + sectors;

    vector<uint8_t> buffer(sectors * g_BootSector.BytesPerSector);

    if (!readSectors(disk, lba, sectors, buffer.data()))
        return false;

    g_RootDirectory.resize(g_BootSector.DirEntryCount);
    memcpy(g_RootDirectory.data(), buffer.data(), size);

    return true;
}

// Find file
DirectoryEntry* findFile(const char* name) {
    for (auto& entry : g_RootDirectory) {
        if (memcmp(name, entry.Name, 11) == 0)
            return &entry;
    }
    return nullptr;
}

// Read file
bool readFile(DirectoryEntry* fileEntry, ifstream& disk, vector<uint8_t>& output) {
    uint16_t currentCluster = fileEntry->FirstClusterLow;

    while (currentCluster < 0x0FF8) {
        uint32_t lba = g_RootDirectoryEnd +
                       (currentCluster - 2) * g_BootSector.SectorsPerCluster;

        size_t oldSize = output.size();
        output.resize(oldSize +
            g_BootSector.SectorsPerCluster * g_BootSector.BytesPerSector);

        if (!readSectors(disk, lba, g_BootSector.SectorsPerCluster,
                         output.data() + oldSize))
            return false;

        uint32_t fatIndex = currentCluster * 3 / 2;
        uint16_t next;

        if (currentCluster % 2 == 0)
            next = *(uint16_t*)(&g_Fat[fatIndex]) & 0x0FFF;
        else
            next = *(uint16_t*)(&g_Fat[fatIndex]) >> 4;

        currentCluster = next;
    }

    return true;
}

// Main
int main(int argc, char** argv) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <disk.img> <file>\n";
        return -1;
    }

    ifstream disk(argv[1], ios::binary);
    if (!disk) {
        cerr << "Cannot open disk image\n";
        return -1;
    }

    if (!readBootSector(disk)) {
        cerr << "Failed to read boot sector\n";
        return -2;
    }

    if (!readFat(disk)) {
        cerr << "Failed to read FAT\n";
        return -3;
    }

    if (!readRootDirectory(disk)) {
        cerr << "Failed to read root directory\n";
        return -4;
    }

    char formattedName[11];
    formatFileName(argv[2], formattedName);

    DirectoryEntry* file = findFile(formattedName);
    if (!file) {
        cerr << "File not found\n";
        return -5;
    }

    vector<uint8_t> buffer;
    if (!readFile(file, disk, buffer)) {
        cerr << "Failed to read file\n";
        return -6;
    }

    for (uint32_t i = 0; i < file->Size; i++) {
        if (isprint(buffer[i]))
            cout << (char)buffer[i];
        else {
            cout << "<"
                 << hex << uppercase << setw(2) << setfill('0')
                 << (int)buffer[i]
                 << dec << ">";
        }
    }

    cout << endl;
    return 0;
}
```
The above code contains the same logic as that of in `C`, next time we will look into converting the same `C++` code into `ASSEMBLY` instructions and execute it.
