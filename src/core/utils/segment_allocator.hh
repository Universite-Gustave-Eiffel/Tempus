/**
 *   Copyright (C) 2012-2015 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <string>

///
/// This set of functions are used to easily seggregate dynamic memory allocations
/// in a fixed contiguous segment of memory.
/// Once enabled, global new/delete operators are overloaded such that new allocations are made
/// in this segment of memory. It can then be dumped directyl to a file.
/// The segment is reloaded to the very same memory address (thanks to mmap).
/// This way, raw pointers can be handled easily.
///
/// It relies on the fact that there is a very low probability on a 64bit architecture that
/// a segment cannot be mapped to a fixed given address

namespace SegmentAllocator
{

///
/// Initialize the segment allocator with an empty segment of the given size
/// The allocation in it is not enabled until enable(true) is called
void init( size_t size );

///
/// Release (unmap) the memory segment
void release();

///
/// Enable or disable the allocation in the segment
/// If disabled, new and delete will (de)allocate on the normal heap
/// If enabled, new and delete will (de)allocate on the segment
void enable( bool enabled );

///
/// Dump the current segment into a file, that can be reloaded by init
/// @param first_ptr address of a reference object
void dump( const std::string& filename, void* first_ptr );

///
/// Map a memory segment from a given file resulting from a dump
/// It returns the address of a reference object
void* init( const std::string& filename );

} // SegmentAllocator
