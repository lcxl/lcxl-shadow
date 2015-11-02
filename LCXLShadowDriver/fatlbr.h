#ifndef _FAT_LBR_H_
#define _FAT_LBR_H_

////////////////////////////////////////////////////////////////////////////
// FAT specific:
//
// RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec - 1)) / BPB_BytsPerSec;
//
// If(BPB_FATSz16 != 0)
//		FATSz = BPB_FATSz16;
// Else
//		FATSz = BPB_FATSz32;
//
// FirstDataSector = BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors;
// FirstDataSector *= BPB_BytsPerSec; // Logical offset
//
// If(BPB_TotSec16 != 0)
//		TotSec = BPB_TotSec16;
// Else
//		TotSec = BPB_TotSec32;
//
// DataSec = TotSec - (BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors);
// CountofClusters = DataSec / BPB_SecPerClus;
//
// If(CountofClusters < 4085) {
// 	/* Volume is FAT12 */
// } else if(CountofClusters < 65525) {
//     /* Volume is FAT16 */
// } else {
//     /* Volume is FAT32 */
// }

#include <pshpack1.h>

// BIOS Parameter Block structure (FAT12/16/32)

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
	WORD	BytesPerSector;			// 0x00 Count of bytes per sector
	BYTE	SectorsPerCluster;		// 0x02 Number of sectors per allocation unit
	WORD	ReservedSectors;		// 0x03 Number of reserved sectors
	BYTE	Fats;					// 0x05 The count of FAT data structures on the volume
	WORD	RootEntries;			// 0x06 Count of directory entries in the root directory
	WORD	Sectors;				// 0x08 Old 16-bit total count of sectors on the volume
	BYTE	Media;					// 0x0A Media type
	WORD	SectorsPerFat;			// 0x0B Old 16-bit count of sectors occupied by one FAT.	
	WORD	SectorsPerTrack;		// 0x0D Sectors per track for interrupt 0x13
	WORD	Heads;					// 0x0F Number of heads for interrupt 0x13
	DWORD	HiddenSectors;			// 0x11 Count of hidden sectors preceding the partition that contains this FAT volume
	DWORD	LargeSectors;			// 0x15 New 32-bit total count of sectors on the volume
} PACKED_BIOS_PARAMETER_BLOCK;		// 0x19
typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

// Extended BIOS Parameter Block structure (FAT12/16/32)

typedef struct _FAT_EBPB {
	BYTE	PhysicalDriveNumber;	// 0x00 Int 0x13 drive number 
	BYTE	CurrentHead;			// 0x01 Reserved (used by Windows NT)
	BYTE	Signature;				// 0x02 Extended boot signature
	DWORD	Id;						// 0x03 Volume serial number
	BYTE	VolumeLabel[11];		// 0x07 Volume label
	BYTE	SystemId[8];			// 0x12 File system type, not used
} FAT_EBPB, *PFAT_EBPB, *LPFAT_EBPB;// 0x1A

// Extended BIOS Parameter Block structure (FAT32)

typedef struct _FAT_EBPB32 {
	DWORD		LargeSectorsPerFat;			// 0x00 FAT32 32-bit count of sectors occupied by ONE FAT
	WORD		ExtendedFlags;				// 0x04 Extended flags
	WORD		FsVersion;					// 0x06 Version number of the FAT32 volume
	DWORD		RootDirFirstCluster;		// 0x08 Cluster number of the first cluster of the root directory
	WORD		FsInfoSector;				// 0x0C Sector number of FSINFO structure in the reserved area of the FAT32 volume
	WORD		BackupBootSector;			// 0x0E Sector number in the reserved area of the volume of a copy of the boot record
	BYTE		Reserved[12];				// 0x10 Reserved for future expansion
	FAT_EBPB	ebpb;						// 0x1C
} FAT_EBPB32, *PFAT_EBPB32, *LPFAT_EBPB32;	//0x36

typedef struct _NTFS_EBPB {
	UCHAR Unused[4];                                                //  offset = 0x00
	LONGLONG NumberSectors;                                         //  offset = 0x04
	LONGLONG MftStartLcn;                                           //  offset = 0x0C
	LONGLONG Mft2StartLcn;                                          //  offset = 0x14
	CHAR ClustersPerFileRecordSegment;                              //  offset = 0x1C
	UCHAR Reserved0[3];												//  offset = 0x1D
	CHAR DefaultClustersPerIndexAllocationBuffer;                   //  offset = 0x20
	UCHAR Reserved1[3];												//  offset = 0x21
	LONGLONG SerialNumber;                                          //  offset = 0x24
	ULONG Checksum;													//  offset = 0x2C
} NTFS_EBPB, *PNTFS_EBPB;											//  offset = 0x30
// Logical Boot Record structure (volume boot sector)

typedef struct _FAT_LBR {
	BYTE			Jump[3];			//0x000 Jump instruction to boot code
	BYTE			Oem[8];				//0x003 Name string
	PACKED_BIOS_PARAMETER_BLOCK	bpb;	//0x00B BIOS Parameter Block
	union {
		FAT_EBPB	ebpb16;				//0x024 FAT12/16 Extended BIOS Parameter Block(0x1A)
		FAT_EBPB32	ebpb32;				//0x024 FAT32 Extended BIOS Parameter Block(0x36)
		NTFS_EBPB   epbpntfs;			//0x024 NTFS Extended BIOS Parameter Block(0x30)
	};
	BYTE			pbyBootCode[0x1A4];	//0x05A Boot sector code
	WORD			wTrailSig;			//0x1FE Ä©Î²±ê¼Ç£º0xAA55
} FAT_LBR, *PFAT_LBR, *LPFAT_LBR;		//0x200


//
//  The boot sector is the first physical sector (LBN == 0) on the volume.
//  Part of the sector contains a BIOS Parameter Block.  The BIOS in the
//  sector is packed (i.e., unaligned) so we'll supply a unpacking macro
//  to translate a packed BIOS into its unpacked equivalent.  The unpacked
//  BIOS structure is already defined in ntioapi.h so we only need to define
//  the packed BIOS.
//

//
//  Define the Packed and Unpacked BIOS Parameter Block
//


typedef struct _PACKED_BOOT_SECTOR {
	UCHAR Jump[3];                                  // offset = 0x000   0
	UCHAR Oem[8];                                   // offset = 0x003   3
	PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11
	UCHAR PhysicalDriveNumber;                      // offset = 0x024  36
	UCHAR CurrentHead;                              // offset = 0x025  37
	UCHAR Signature;                                // offset = 0x026  38
	UCHAR Id[4];                                    // offset = 0x027  39
	UCHAR VolumeLabel[11];                          // offset = 0x02B  43
	UCHAR SystemId[8];                              // offset = 0x036  54
} PACKED_BOOT_SECTOR;                               // sizeof = 0x03E  62

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

#include <poppack.h>

#endif