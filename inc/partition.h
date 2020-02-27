#ifndef JOS_INC_PARTITION_H
#define JOS_INC_PARTITION_H
#include <inc/types.h>

/*
 * This file contains definitions for x86 partition tables, and comes from
 * Mike Mammarella.
 */

// Offset of 1st partition descriptor in a partition sector
#define PTABLE_OFFSET		446
// 2-byte partition table magic number location and value
#define PTABLE_MAGIC_OFFSET	510
#define PTABLE_MAGIC		"\x55\xAA"

// Partition type constants
#define PTYPE_JOS_KERN		0x27	// JOS kernel
#define PTYPE_JOSFS		0x28	// JOS file system
// Extended partition identifiers
#define PTYPE_DOS_EXTENDED	0x05
#define PTYPE_W95_EXTENDED	0x0F
#define PTYPE_LINUX_EXTENDED	0x85

struct Partitiondesc {
	uint8_t boot;
	uint8_t chs_begin[3];
	uint8_t type;
	uint8_t chs_end[3];
	uint32_t lba_start;
	uint32_t lba_length;
};

#endif
