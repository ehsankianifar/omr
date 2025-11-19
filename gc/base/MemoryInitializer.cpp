	
    
#include "ehsanLogger.h"
#include <mutex>
#include "omrcomp.h"
#include "HeapLinkedFreeHeader.hpp"
#include "omrutil.h"
#include <thread>

static volatile uintptr_t _initBase;
static volatile uintptr_t _initCurrent;
static volatile uintptr_t _initTop;
static int _initMode;
static int _blockSize;
static bool _isReady = false;
//std::mutex _mtx;

enum Mode {
    disabled = 0,
    inlined  = 1,
    attached = 2,
    detached = 3,
    lastItem = detached
};
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
		// zero header metadata
		if(deleteHeader)
			memset((void*)addrBase, 0, sizeof(MM_HeapLinkedFreeHeader));
	}
	ehsanLog("Checking base %lx top %lx initBase %lx initTop %lx initCurrent %lx redult %d whileCount %d", addrBase, addrTop, _initBase, _initTop, _initCurrent, result, whileCount);
	return result;
}

static void startZeroing()
{
    size_t size = _initTop - _initCurrent;
    int blocks = size/_blockSize;
    //start initialization
    ehsanLog("Started init from %lx to %lx size %lu blocks %d", _initBase, _initTop, size, blocks);
    for(int i = 0; i < blocks; i++){
        OMRZeroMemory((void*)_initCurrent, _blockSize);
        _initCurrent += _blockSize;
    }
    if(_initCurrent < _initTop){
        OMRZeroMemory((void*)_initCurrent, (_initTop - _initCurrent));
        _initCurrent = _initTop;
    }
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
		ehsanLog("Skip init base %lx top %lx initBase %lx initTop %lx initCurrent %lx", addrBase, addrTop, _initBase, _initTop, _initCurrent);
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
        ehsanLog("_initMode: %d, _blockSize %d", _initMode, _blockSize);
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