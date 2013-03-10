#include <vdr/tools.h>

#include "osdproxy.h"

cOsdProxy::cOsdProxy(OSDCREATEFUNC osdMenuCreateFunction, KEYHANDLERFUNC keyHandler, RELEASEFUNC releaseFunc,const char* title, cOsdProxyData* data,eKeys osdKey, bool directShow)
{
  mReturnOsEnd = false;

  mOsdMenuCreateFunc = osdMenuCreateFunction;
  mData = data;
  mTitle = strdup(title);
  mOsdMenu = NULL;
  mOsdKey = osdKey;
  mKeyHandlerFunc = keyHandler;
  mReleaseFunc = releaseFunc;

  if (directShow)
    ShowOsd(true);
}

cOsdProxy::~cOsdProxy()
{
  if (mOsdMenu)
    ShowOsd(false);
  if (mTitle)
    free(mTitle);

  if (mReleaseFunc)
    mReleaseFunc();

  DELETENULL(mData);  
}

void cOsdProxy::Show(void)
{
  if (mOsdMenu)
    mOsdMenu->Display();
}

void cOsdProxy::ShowOsd(bool show)
{
  if ((!show) && (mOsdMenu))
    DELETENULL(mOsdMenu);
  else if ((show) && (!mOsdMenu))
  {
    mOsdMenu = mOsdMenuCreateFunc(mTitle,mData);
    // if the menu creation func delivers NULL, we can kill ourselves ;)
    if (!mOsdMenu)
      mReturnOsEnd = true;
  }
}

eOSState cOsdProxy::ProcessKey(eKeys Key)
{
  // if flag is set to immediately return to VDR ...
  if (mReturnOsEnd)
    return osEnd;

  if (mOsdMenu)
  {
    eOSState retState = mOsdMenu->ProcessKey(Key);
    if (retState == osEnd)
    {
      ShowOsd(false);
      return osContinue;
    }
    return retState;
  }
  else if (Key == mOsdKey)
  {
    ShowOsd(true);
    return osContinue;
  }
  else if (Key == kBack)
    return osEnd;
  else if (mKeyHandlerFunc)
    return mKeyHandlerFunc(Key);
  else
    return osContinue;
}
