/*******************************************************************************
 * Copyright IBM Corp. and others 1991
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#if !defined(HEAPLINKEDFREEHEADER_HPP_)
#define HEAPLINKEDFREEHEADER_HPP_

#include "omrcomp.h"
#include "modronbase.h"
/* #include "ModronAssertions.h" -- removed for now because it causes a compile error in TraceOutput.cpp on xlC */

#include "AtomicOperations.hpp"

/**
 *  This class supercedes the J9GCModronLinkedFreeHeader struct used
 * in older versions of Modron.
 *
 * @ingroup GC_Base_Core
 */

class MM_HeapLinkedFreeHeader
{
public:
protected:
	/* NOTE: New layout - pointer to object points to END of chunk
	 * Memory layout: [...free memory...][_beginPtr][_next]
	 * The object pointer points to the location after _next
	 *
	 * IMPORTANT: The _next and _size fields below are UNUSED in the new layout.
	 * They are kept only for compatibility with DDR and debugging tools.
	 * The actual data is stored at (this - 2*sizeof(uintptr_t)) for beginning pointer
	 * and (this - sizeof(uintptr_t)) for next pointer.
	 *
	 * DO NOT ACCESS THESE FIELDS DIRECTLY - use the accessor methods instead.
	 */
	uintptr_t _next;  /**< UNUSED - kept for DDR compatibility only */
	uintptr_t _size;  /**< UNUSED - kept for DDR compatibility only */

private:

	/*
	 * CMVC 130331
	 * The following private member functions must be declared before the public functions
	 * due to a bug in an old version of GCC used on the HardHat Linux platforms. These
	 * functions can't be in-lined if they're not declared before the point they're used.
	 */
private:
	/**
	 * Get pointer to where the beginning pointer is stored.
	 *
	 * @return pointer to the location storing the beginning pointer
	 */
	MMINLINE uintptr_t*
	getBeginPtrLocation()
	{
		return (uintptr_t*)((uintptr_t)this - 2 * sizeof(uintptr_t));
	}

	/**
	 * Get pointer to where the next pointer is stored.
	 *
	 * @return pointer to the location storing the next pointer
	 */
	MMINLINE uintptr_t*
	getNextPtrLocation()
	{
		return (uintptr_t*)((uintptr_t)this - sizeof(uintptr_t));
	}

	/**
	 * Fetch the value encoded in the next pointer.
	 * Since the encoding may be different depending on the header shape
	 * other functions in this class should use this helper rather than
	 * reading the field directly.
	 *
	 * @return the decoded next pointer, with any tag bits intact
	 */
	MMINLINE uintptr_t
	getNextImpl(bool compressed)
	{
		uintptr_t result = *getNextPtrLocation();
#if defined(OMR_GC_COMPRESSED_POINTERS) && !defined(OMR_ENV_LITTLE_ENDIAN)
		if (compressed) {
			/* On big endian compressed, the pointer has been stored
			 * endian-flipped, so flip it back. See MM_HeapLinkedFreeHeader::setNextImpl.
			 */
			result = (result >> 32) | (result << 32);
		}
#endif /* defined(OMR_GC_COMPRESSED_POINTERS) && !defined(OMR_ENV_LITTLE_ENDIAN) */
		return result;
	}

	/**
	 * Encoded the specified value as the next pointer.
	 * Since the encoding may be different depending on the header shape
	 * other functions in this class should use this helper rather than
	 * writing the field directly.
	 *
	 * @parm value the value to be stored
	 */
	MMINLINE void
	setNextImpl(uintptr_t value, bool compressed)
	{
#if defined(OMR_GC_COMPRESSED_POINTERS) && !defined(OMR_ENV_LITTLE_ENDIAN)
		if (compressed) {
			/* On big endian compressed, endian flip the pointer so that the
			 * tag bits appear in the compressed class slot.
			 */
			value = (value >> 32) | (value << 32);
		}
#endif /* defined(OMR_GC_COMPRESSED_POINTERS) && !defined(OMR_ENV_LITTLE_ENDIAN) */
		*getNextPtrLocation() = value;
	}

public:
	/**
	 * Get pointer to the beginning of the free chunk.
	 * The object pointer points to the end, so we subtract 2 pointer sizes.
	 *
	 * @return pointer to the beginning of the chunk
	 */
	MMINLINE void*
	getBase()
	{
		return (void*)((uintptr_t)this - 2 * sizeof(uintptr_t));
	}

	/**
	 * Convert a pointer to a dead object to a HeapLinkedFreeHeader.
	 * NOTE: In the new layout, the pointer should point to the END of the chunk.
	 * If you have a pointer to the beginning, you need to add the size first.
	 */
	static MMINLINE MM_HeapLinkedFreeHeader *getHeapLinkedFreeHeader(void* pointer) { return (MM_HeapLinkedFreeHeader*)pointer; }
	
	/**
	 * Create a HeapLinkedFreeHeader from a base address and size.
	 * This is the preferred way to create a header when you know the base and size.
	 * @param[in] addrBase the beginning of the free chunk
	 * @param[in] size the size of the free chunk
	 * @return pointer to the header (at the end of the chunk)
	 */
	static MMINLINE MM_HeapLinkedFreeHeader *getHeapLinkedFreeHeaderFromBase(void* addrBase, uintptr_t size) {
		return (MM_HeapLinkedFreeHeader*)((uintptr_t)addrBase + size);
	}

	/**
	 * Get the next free header in the linked list of free entries
	 * @return the next entry or NULL
	 */
	MMINLINE MM_HeapLinkedFreeHeader *getNext(bool compressed) {
		/* Assert_MM_true(0 != ((getNextImpl(compressed)) & J9_GC_OBJ_HEAP_HOLE)); */
		return (MM_HeapLinkedFreeHeader*)((getNextImpl(compressed)) & ~((uintptr_t)J9_GC_OBJ_HEAP_HOLE_MASK));
	}

	/**
	 * Set the next free header in the linked list and mark the receiver as a multi-slot hole
	 * Set to NULL to terminate the list.
	 */
	MMINLINE void setNext(MM_HeapLinkedFreeHeader* freeEntryPtr, bool compressed) { setNextImpl(((uintptr_t)freeEntryPtr) | ((uintptr_t)J9_GC_MULTI_SLOT_HOLE), compressed); }

	/**
	 * Set the next free header in the linked list, preserving the type of the receiver
	 * Set to NULL to terminate the list.
	 */
	MMINLINE void updateNext(MM_HeapLinkedFreeHeader* freeEntryPtr, bool compressed) { setNextImpl(((uintptr_t)freeEntryPtr) | (getNextImpl(compressed) & (uintptr_t)J9_GC_OBJ_HEAP_HOLE_MASK), compressed); }

	/**
	 * Get the size in bytes of this free entry. The size is measured
	 * from the beginning of the chunk to the end (where this pointer points).
	 * @return size in bytes
	 */
	MMINLINE uintptr_t getSize()
	{
		void* beginPtr = (void*)(*getBeginPtrLocation());
		return (uintptr_t)this - (uintptr_t)beginPtr;
	}

	/**
	 * Set the size in bytes of this free entry by updating the beginning pointer.
	 * The end pointer (this) stays fixed.
	 */
	MMINLINE void setSize(uintptr_t size)
	{
		*getBeginPtrLocation() = (uintptr_t)this - size;
	}

	/**
	 * Expand this entry by the specified number of bytes.
	 * This moves the beginning pointer backward (to lower address).
	 */
	MMINLINE void expandSize(uintptr_t increment)
	{
		*getBeginPtrLocation() -= increment;
	}

	/**
	 * Return the address immediately following the free section
	 * described by this header. Since the pointer already points to the end,
	 * this is just the current pointer location.
	 * @return address following this free section
	 */
	MMINLINE MM_HeapLinkedFreeHeader* afterEnd() { return this; }

	/**
	 * Mark the specified region of memory as walkable dark matter
	 * @param[in] addrBase the address where the hole begins
	 * @param[in] freeEntrySize the number of bytes to be consumed (must be a multiple of sizeof(uintptr_t))
	 * @return The header written (pointer to END of chunk) or null if the space was too small and was filled with single-slot holes
	 */
	MMINLINE static MM_HeapLinkedFreeHeader*
	fillWithHoles(void* addrBase, uintptr_t freeEntrySize, bool compressed)
	{
		MM_HeapLinkedFreeHeader *freeEntry = NULL;
		if (freeEntrySize < 2 * sizeof(uintptr_t)) {
			/* Too small for the new header format, fill with single-slot holes */
			/* Assert_MM_true(0 == (freeEntrySize % sizeof(uintptr_t))); */
			void* currentAddr = addrBase;
			while (0 != freeEntrySize) {
				/* For single slot holes, we still write at the beginning */
				*((uintptr_t*)currentAddr) = J9_GC_SINGLE_SLOT_HOLE;
				currentAddr = (void*)(((uintptr_t*)currentAddr) + 1);
				freeEntrySize -= sizeof(uintptr_t);
			}
		} else {
			/* this is too big to use single slot holes so generate an AOL-style hole */
			/* The free entry pointer points to the END of the chunk */
			void* addrEnd = (void*)((uintptr_t)addrBase + freeEntrySize);
			freeEntry = (MM_HeapLinkedFreeHeader *)addrEnd;

			/* Write the beginning pointer at (end - 2*sizeof(uintptr_t)) */
			*((uintptr_t*)((uintptr_t)addrEnd - 2 * sizeof(uintptr_t))) = (uintptr_t)addrBase;
			
			/* Write the next pointer at (end - sizeof(uintptr_t)) */
			uintptr_t nextValue = (uintptr_t)NULL | ((uintptr_t)J9_GC_MULTI_SLOT_HOLE);
#if defined(OMR_GC_COMPRESSED_POINTERS) && !defined(OMR_ENV_LITTLE_ENDIAN)
			if (compressed) {
				nextValue = (nextValue >> 32) | (nextValue << 32);
			}
#endif
			*((uintptr_t*)((uintptr_t)addrEnd - sizeof(uintptr_t))) = nextValue;
		}
		return freeEntry;
	}

	/**
	 * Links a new free header in at the head of the free list.
	 * @param[in/out] currentHead Input pointer is set to nextHead on return
	 * @param[in] nextHead Pointer to the new head of list (pointing to END of chunk), with nextHead->_next pointing to previous value of currentHead on return
	 * @note Caller must have already set up the beginning pointer for nextHead
	 */
	MMINLINE static void
	linkInAsHead(volatile uintptr_t *currentHead, MM_HeapLinkedFreeHeader* nextHead, bool compressed)
	{
		uintptr_t oldValue, newValue;
		do {
			oldValue = *currentHead;
			newValue = MM_AtomicOperations::lockCompareExchange(currentHead, oldValue, (uintptr_t)nextHead);

		} while (oldValue != newValue);
		nextHead->setNext((MM_HeapLinkedFreeHeader *)newValue, compressed);
	}

protected:
private:

};

#endif /* HEAPLINKEDFREEHEADER_HPP_ */

