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

#if !defined(MEMORYZEROER_HPP_)
#define MEMORYZEROER_HPP_

#include "omrcfg.h"
#include "omrcomp.h"
#include "omrthread.h"
#include "BaseVirtual.hpp"

class MM_EnvironmentBase;

/**
 * @brief Lightweight singleton class for asynchronous memory zeroing
 * 
 * This class manages a single worker thread that performs memory zeroing operations
 * asynchronously. The thread waits for work requests and signals completion via a
 * status pointer.
 * 
 * @ingroup GC_Base_Core
 */
class MM_MemoryZeroer : public MM_BaseVirtual
{
/*
 * Data members
 */
private:
	omrthread_monitor_t _monitor;          /**< Monitor for thread synchronization */
	omrthread_t _workerThread;             /**< The worker thread handle */
	
	void *_zeroStart;                      /**< Start address for zeroing operation */
	uintptr_t _zeroSize;                   /**< Size of memory to zero */
	volatile int *_statusPtr;              /**< Pointer to status variable to update on completion */
	
	volatile bool _hasWork;                /**< Flag indicating work is available */
	volatile bool _shutdownRequested;      /**< Flag to request thread shutdown */
	volatile bool _threadRunning;          /**< Flag indicating thread is running */

protected:
public:

/*
 * Function members
 */
private:
	/**
	 * Worker thread entry point
	 * @param arg Pointer to MM_MemoryZeroer instance
	 * @return Thread exit code
	 */
	static int J9THREAD_PROC workerThreadMain(void *arg);
	
	/**
	 * Main worker loop executed by the thread
	 */
	void workerLoop();

protected:
	bool initialize(MM_EnvironmentBase *env);
	void tearDown(MM_EnvironmentBase *env);

public:
	/**
	 * Create and initialize a new MemoryZeroer instance
	 * @param env The environment
	 * @return Pointer to new instance, or NULL on failure
	 */
	static MM_MemoryZeroer *newInstance(MM_EnvironmentBase *env);
	
	/**
	 * Destroy the MemoryZeroer instance
	 * @param env The environment
	 */
	virtual void kill(MM_EnvironmentBase *env);
	
	/**
	 * Request asynchronous memory zeroing
	 * @param start Start address of memory to zero
	 * @param size Size of memory to zero
	 * @param statusPtr Pointer to status variable (will be set to 1 when complete)
	 */
	void requestZeroing(void *start, uintptr_t size, volatile int *statusPtr);
	
	/**
	 * Constructor
	 */
	MM_MemoryZeroer()
		: MM_BaseVirtual()
		, _monitor(NULL)
		, _workerThread(NULL)
		, _zeroStart(NULL)
		, _zeroSize(0)
		, _statusPtr(NULL)
		, _hasWork(false)
		, _shutdownRequested(false)
		, _threadRunning(false)
	{
		_typeId = __FUNCTION__;
	}
};

#endif /* MEMORYZEROER_HPP_ */