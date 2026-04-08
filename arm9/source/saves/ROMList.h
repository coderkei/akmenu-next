/*
    Copyright 2016-2026 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

/* clang-format off */

#ifndef ROMLIST_H
#define ROMLIST_H

#include <stddef.h>

#include <nds/ndstypes.h>

/* AKMENU CHANGES START - don't namespace ROMList */
/*
namespace melonDS
{
*/
/* AKMENU CHANGES END - don't namespace ROMList */
struct ROMListEntry
{
	u32 GameCode;
	u32 ROMSize;
	u32 SaveMemType;
};


extern const ROMListEntry ROMList[];

/// The number of elements in \c ROMList.
extern const size_t ROMListEntryCount;

/* AKMENU CHANGES START - don't namespace ROMList */
/*
}
*/
/* AKMENU CHANGES END - don't namespace ROMList */
#endif // ROMLIST_H
