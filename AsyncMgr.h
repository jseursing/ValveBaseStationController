#pragma once
#include <future>
#include <map>

class AsyncMgr
{
public:

  static AsyncMgr* Instance();
  uintptr_t Spawn(void* func, void* param);
  bool Kill(uintptr_t taskId, bool force = false);
  bool KeepAlive(uintptr_t taskId);

private:

  AsyncMgr();
  ~AsyncMgr();

  struct AsyncEntry
  {
    bool  kill_flag;
    void* handle;
  };

  std::map<uintptr_t, AsyncEntry> AsyncMap;
};