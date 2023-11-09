#include "AsyncMgr.h"
#include <windows.h>


AsyncMgr* AsyncMgr::Instance()
{
  static AsyncMgr instance;
  return &instance;
}

uintptr_t AsyncMgr::Spawn(void* func, void* param)
{
  uintptr_t taskId = 0;

  AsyncEntry entry;
  entry.kill_flag = false;
  entry.handle = CreateThread(0, 
                              0, 
                              reinterpret_cast<LPTHREAD_START_ROUTINE>(func), 
                              param, 0, 
                              reinterpret_cast<DWORD*>(&taskId));
  if (INVALID_HANDLE_VALUE == entry.handle)
  {
    return 0;
  }

  AsyncMap[taskId] = entry;

  return taskId;
}

bool AsyncMgr::Kill(uintptr_t taskId, bool force)
{
  bool res = false;
  if (AsyncMap.end() != AsyncMap.find(taskId))
  {
    AsyncMap[taskId].kill_flag = true;
    res = true;

    if (true == force)
    {
      TerminateThread(AsyncMap[taskId].handle, 0);
    }
  }

  return res;
}

bool AsyncMgr::KeepAlive(uintptr_t taskId)
{
  if (AsyncMap.end() != AsyncMap.find(taskId))
  {
    return !AsyncMap[taskId].kill_flag;
  }

  return false;
}

AsyncMgr::AsyncMgr()
{

}

AsyncMgr::~AsyncMgr()
{

}