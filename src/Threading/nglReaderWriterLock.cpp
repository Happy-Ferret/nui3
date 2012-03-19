/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#include "nui.h"
#include "nglReaderWriterLock.h"


nglReaderWriterLock::nglReaderWriterLock()
{
  mReaders = 0;
  mWriter = 0;
}

void nglReaderWriterLock::LockRead()
{ 
return;
  bool ok = false;
  do 
  {
    {
      nglCriticalSectionGuard guard(mCS);

      if (!mWriter)
      {
        mReaders++;
        ok = true;
      }
    }

    if (mWriter)
      mWaitForRead.Wait();

  } while (!ok);
}

bool nglReaderWriterLock::TryLockRead()
{ 
return true;
  bool ok = false;
  {
    nglCriticalSectionGuard guard(mCS);
    
    if (!mWriter)
    {
      mReaders++;
      ok = true;
    }
  }

  return ok;  
}

void nglReaderWriterLock::UnlockRead()
{
return;
  nglCriticalSectionGuard guard(mCS);

  ngl_atomic_dec(mReaders);
  if (!mReaders)
    mWaitForWrite.WakeOne();
}

void nglReaderWriterLock::LockWrite()
{
return;
  do 
  {
    {
      nglCriticalSectionGuard guard(mCS);

      if (!mWriter && !mReaders)
      {
        mWriter = nglThread::GetCurThreadID();
      }
    }

    if (mWriter && mWriter != nglThread::GetCurThreadID())
      mWaitForWrite.Wait();

  } while (mWriter != nglThread::GetCurThreadID());
}

bool nglReaderWriterLock::TryLockWrite()
{
return true;
  {
    nglCriticalSectionGuard guard(mCS);
    
    if (!mWriter && !mReaders)
    {
      mWriter = nglThread::GetCurThreadID();
    }
  }
  
  return mWriter == nglThread::GetCurThreadID();
}


void nglReaderWriterLock::UnlockWrite()
{
return;
  nglCriticalSectionGuard guard(mCS);

  mWriter = NULL;
  if (!mReaders)
    mWaitForWrite.WakeOne();
  else
    mWaitForRead.WakeAll();
}



