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
