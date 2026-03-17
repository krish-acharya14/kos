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
