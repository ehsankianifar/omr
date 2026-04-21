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

#include "omrcfg.h"
#include "omrport.h"
#include "ModronAssertions.h"

#include "MemoryZeroer.hpp"
#include "EnvironmentBase.hpp"
#include "GCExtensionsBase.hpp"

MM_MemoryZeroer *
MM_MemoryZeroer::newInstance(MM_EnvironmentBase *env)
{
	MM_MemoryZeroer *zeroer = (MM_MemoryZeroer *)env->getForge()->allocate(
		sizeof(MM_MemoryZeroer), 
		OMR::GC::AllocationCategory::FIXED, 
		OMR_GET_CALLSITE()
	);
	
	if (NULL != zeroer) {
		new(zeroer) MM_MemoryZeroer();
		if (!zeroer->initialize(env)) {
			zeroer->kill(env);
			zeroer = NULL;
		}
	}
	
	return zeroer;
}

void
MM_MemoryZeroer::kill(MM_EnvironmentBase *env)
{
	tearDown(env);
	env->getForge()->free(this);
}

bool
MM_MemoryZeroer::initialize(MM_EnvironmentBase *env)
{
	/* Initialize the monitor for thread synchronization */
	if (0 != omrthread_monitor_init_with_name(&_monitor, 0, "MM_MemoryZeroer::monitor")) {
		return false;
	}
	
	/* Create the worker thread */
	if (0 != omrthread_create(&_workerThread, 0, J9THREAD_PRIORITY_NORMAL, 0, 
	                          workerThreadMain, this)) {
		omrthread_monitor_destroy(_monitor);
		_monitor = NULL;
		return false;
	}
	
	/* Wait for thread to start */
	omrthread_monitor_enter(_monitor);
	while (!_threadRunning) {
		omrthread_monitor_wait(_monitor);
	}
	omrthread_monitor_exit(_monitor);
	
	return true;
}

void
MM_MemoryZeroer::tearDown(MM_EnvironmentBase *env)
{
	if (NULL != _monitor) {
		/* Request shutdown and notify the worker thread */
		omrthread_monitor_enter(_monitor);
		_shutdownRequested = true;
		omrthread_monitor_notify(_monitor);
		omrthread_monitor_exit(_monitor);
		
		/* Wait for the thread to exit */
		if (NULL != _workerThread) {
			omrthread_monitor_enter(_monitor);
			while (_threadRunning) {
				omrthread_monitor_wait(_monitor);
			}
			omrthread_monitor_exit(_monitor);
			_workerThread = NULL;
		}
		
		/* Destroy the monitor */
		omrthread_monitor_destroy(_monitor);
		_monitor = NULL;
	}
}

int J9THREAD_PROC
MM_MemoryZeroer::workerThreadMain(void *arg)
{
	MM_MemoryZeroer *zeroer = (MM_MemoryZeroer *)arg;
	zeroer->workerLoop();
	return 0;
}

void
MM_MemoryZeroer::workerLoop()
{
	omrthread_monitor_enter(_monitor);
	
	/* Signal that thread has started */
	_threadRunning = true;
	omrthread_monitor_notify(_monitor);
	
	/* Main worker loop */
	while (!_shutdownRequested) {
		/* Wait for work */
		while (!_hasWork && !_shutdownRequested) {
			omrthread_monitor_wait(_monitor);
		}
		
		if (_shutdownRequested) {
			break;
		}
		
		/* We have work - copy the parameters and release the monitor */
		void *start = _zeroStart;
		uintptr_t size = _zeroSize;
		volatile int *statusPtr = _statusPtr;
		
		/* Clear work flag before releasing monitor */
		_hasWork = false;
		
		/* Release monitor while doing the actual work */
		omrthread_monitor_exit(_monitor);
		
		/* Perform the memory zeroing operation */
		OMRZeroMemory(start, size);
		
		/* Update status to indicate completion */
		if (NULL != statusPtr) {
			*statusPtr = 1;
		}
		
		/* Re-acquire monitor for next iteration */
		omrthread_monitor_enter(_monitor);
	}
	
	/* Signal that thread is exiting */
	_threadRunning = false;
	omrthread_monitor_notify(_monitor);
	omrthread_monitor_exit(_monitor);
}

void
MM_MemoryZeroer::requestZeroing(void *start, uintptr_t size, volatile int *statusPtr)
{
	Assert_MM_true(NULL != _monitor);
	
	omrthread_monitor_enter(_monitor);
	
	/* Wait if there's already work pending */
	while (_hasWork) {
		omrthread_monitor_wait(_monitor);
	}
	
	/* Set up the work parameters */
	_zeroStart = start;
	_zeroSize = size;
	_statusPtr = statusPtr;
	_hasWork = true;
	
	/* Notify the worker thread */
	omrthread_monitor_notify(_monitor);
	
	omrthread_monitor_exit(_monitor);
}
