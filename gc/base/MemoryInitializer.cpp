	
    
#include "ehsanLogger.h"
#include <mutex>
#include "omrcomp.h"
#include "HeapLinkedFreeHeader.hpp"
#include "omrutil.h"
#include "omrthread.h"

static volatile uintptr_t _initBase;
static volatile uintptr_t _initCurrent;
static volatile uintptr_t _initTop;
static size_t _minimumSize;
static int _initMode;
static int _blockSize;
static bool _zeroIfTLH;
static bool _isReady = false;
static omrthread_monitor_t _monitor;
static omrthread_t _thread;
//std::mutex _mtx;

enum Mode {
    disabled = 0,
    inlined  = 1,
    omrThread = 2,
    lastItem = omrThread
};

static void startZeroing()
{
    size_t size = _initTop - _initCurrent;
    int blocks = size/_blockSize;
    //start initialization
    //ehsanLog("Started init from %lx to %lx size %lu blocks %d", _initBase, _initTop, size, blocks);
    for(int i = 0; i < blocks; i++){
        OMRZeroMemory((void*)_initCurrent, _blockSize);
        _initCurrent += _blockSize;
    }
    if(_initCurrent < _initTop){
        OMRZeroMemory((void*)_initCurrent, (_initTop - _initCurrent));
        _initCurrent = _initTop;
    }
}

int omrThreadZeroing(void *arg)
{
    while (1) {
        omrthread_monitor_enter(_monitor);
        while (_initCurrent == _initTop) {
            omrthread_monitor_wait(_monitor);
        }
        omrthread_monitor_exit(_monitor);

        startZeroing();
    }
    return 0;
}

static void tryInitializeMemory(MM_HeapLinkedFreeHeader *freeEntry, uintptr_t requestedSize, bool isTLH)
{
    int debug = 0;
     // don't do anything if disabled
    if(_initMode == disabled)
    {
        return;
    }

    uintptr_t addrBase = (uintptr_t)freeEntry;
    uintptr_t addrTop = addrBase + freeEntry->getSize();
    uintptr_t requestedTop = addrBase + requestedSize;
	//check if already initialized or under init.
	if((addrBase >= _initBase) && (requestedTop <= _initTop))
    {
        _initBase = requestedTop;
        requestedTop = OMR_MIN(_initTop, requestedTop + sizeof(MM_HeapLinkedFreeHeader));
        while (requestedTop > _initCurrent)
        {
            /* nop */
            debug = 1;
        }
        if(isTLH)
        {
            OMRZeroMemory((void*)addrBase, sizeof(MM_HeapLinkedFreeHeader));
        }
        ehsanLogNoNewLine("A%d%d ", isTLH, debug);
        return;
    }
    if (!isTLH && _zeroIfTLH)
    {
        return;
    }
    if((addrTop - requestedTop) < _minimumSize)
    {
        ehsanLogNoNewLine("C%d0 ", isTLH);
        return;
    }

    ehsanLog("Initiate zeroing! addrBase 0x%lx addrTop 0x%lx requestedTop 0x%lx _initBase 0x%lx _initTop 0x%lx _initCurrent 0x%lx isTLH %d",
            addrBase, addrTop, requestedTop, _initBase, _initTop, _initCurrent, isTLH);
    if(_initTop != _initCurrent)
    {
        ehsanLog("Unexpected event! Memory is under initialization.");
        return;
    }

    if(_initMode == omrThread)
    {
        omrthread_monitor_enter(_monitor);
    }

    _initBase = requestedTop;
    _initTop = addrTop;
    requestedTop = OMR_MIN(_initTop, requestedTop + sizeof(MM_HeapLinkedFreeHeader));
    _initCurrent = isTLH ? addrBase : requestedTop;

    if(_initMode == inlined)
    {
        startZeroing();
    }
    else if(_initMode == omrThread)
    {
        omrthread_monitor_notify(_monitor);
        omrthread_monitor_exit(_monitor);
    }
    while (requestedTop > _initCurrent)
    {
        /* nop */
        debug = 2;
    }
    ehsanLogNoNewLine("B%d%d ", isTLH, debug);
    
}
/*
static bool isInitialized(uintptr_t addrBase, uintptr_t addrTop, bool deleteHeader)
{
    // don't do anything if disabled
    if(_initMode == disabled)
    {
        return false;
    }
	// false if not overlapping with the whole section under initialization;
	bool result = (addrBase >= _initBase) && (addrTop <= _initTop);
	int whileCount = 0;  
	if(result)
	{
		// add 16 to account for the next free header head.
		// If this add exceeds _initTop, there may not be a next header so limit it to _initTop
		addrTop = OMR_MIN(_initTop, addrTop + sizeof(MM_HeapLinkedFreeHeader));
		// Wait if under intialization;
		while (addrTop > _initCurrent )
		{
			//NOP
			whileCount++;
		}
        if(whileCount)
        {
            ehsanLog("some thread was waiting for memory freeing! %d", whileCount);
        }
		// zero header metadata
		if(deleteHeader)
			OMRZeroMemory((void*)addrBase, sizeof(MM_HeapLinkedFreeHeader));
	}
	//ehsanLog("Checking base %lx top %lx initBase %lx initTop %lx initCurrent %lx redult %d whileCount %d", addrBase, addrTop, _initBase, _initTop, _initCurrent, result, whileCount);
	return result;
}
static void tryInitialize(uintptr_t addrBase, uintptr_t addrTop)
{
     // don't do anything if disabled
    if(_initMode == disabled)
    {
        return;
    }
	//check if already initialized or under init.
	bool status = (addrBase >= _initBase) && (addrTop <= _initTop);
	//check if the initialization is in progress.
	status |= (_initTop != _initCurrent);
	//check size constraint. 1024 is an arbiterary number!
	//status |= (addrTop - addrBase) < 1024;

	// return if any of those conditions are true!
	if(status) {
		//ehsanLog("Skip init base %lx top %lx initBase %lx initTop %lx initCurrent %lx", addrBase, addrTop, _initBase, _initTop, _initCurrent);
		return;
	}

    _initBase = addrBase;
    _initCurrent = _initBase + sizeof(MM_HeapLinkedFreeHeader);
    _initTop = addrTop;
    if(_initMode == inlined)
    {
        startZeroing();
    }
    else
    {
        std::thread cleaner(startZeroing);
        if(_initMode == attached)
        {
            cleaner.join();
        }
        else if(_initMode == detached)
        {
            cleaner.detach();
        }
    }
}
*/
static void resetInitializer()
{
	_initBase = 0;
	_initTop = 0;
	_initCurrent = 0;
    if(!_isReady)
    {
        _isReady = true;
        const char *heapInit = getenv("TR_HeapInit");
        _initMode = heapInit ? atoi(heapInit) : 0;
        if(_initMode < 0 || _initMode > lastItem)
        {
            ehsanLog("Wrong _initMode value: %d reseting to zero!");
            _initMode = 0;
        }
        const char *blockSize = getenv("TR_MemInitBlockSize");
        _blockSize= blockSize ? atoi(blockSize) : 1024 * 8;
        const char *heapInitOnlyIfTLH = getenv("TR_HeapInitOnlyIfTLH");
        _zeroIfTLH = heapInitOnlyIfTLH != NULL;
        const char *minBlocks = getenv("TR_MemInitMinBlocks");
        _minimumSize = minBlocks ? (atoi(minBlocks) * _blockSize) : (4 * _blockSize);
        ehsanLog("_initMode: %d, _blockSize %d, _zeroIfTLH %d", _initMode, _blockSize, _zeroIfTLH);
        if(_initMode == omrThread)
        {
            //omrthread_init();
            omrthread_monitor_init_with_name(&_monitor, 0, "MemoryZeroing");
            omrthread_create(&_thread, 0, J9THREAD_PRIORITY_NORMAL, 0, omrThreadZeroing, NULL);
        }
    }
}
static void checkedResetInitializer()
{
    while (_initTop != _initCurrent){
        // if memory is under initialization, wait to finish!
    }
	_initBase = 0;
	_initTop = 0;
	_initCurrent = 0;
}