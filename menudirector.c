#include "menudirector.h"

#include <vdr/device.h>
#include <vdr/epg.h>
#include <vdr/skins.h>
#include <vdr/tools.h>

cMenuDirectorData::cMenuDirectorData()
{
  mHidePluginEntryInMainMenu = 0;
  mPortalmode = 1;
  mSwapKeys = 0;
  mSwapKeysInMenu = 0;
  mDisplayChannelInfo = 1;
  mSwitchChannelOnItemSelect = 1;
  mHidePluginAfterChannelSwitch = 0;
  mDisplayShortcutNumbers = 1;
  mDisplayEpgInfo = 1;
  mHideButton = kOk;
  mAskOnStart = 0;
  mAvoidStartOnSingleChannels = 0;

  mCalledByChannelSwitch = 0;
};

cMenuDirectorData::~cMenuDirectorData()
{
}

cMenuDirectorItem::cMenuDirectorItem(const char* text, cChannel* channel) : cOsdItem(text)
{
  mChannel = channel;
}

cMenuDirectorItem::~cMenuDirectorItem()
{
}

cChannel* cMenuDirectorItem::Channel()
{
  return mChannel;
}

cChannel* cMenuDirector::mReferenceChannel = NULL;
cMenuDirectorData* cMenuDirector::mData = NULL;

cMenuDirector::cMenuDirector(const char* title, cMenuDirectorData* data) : cOsdMenu(title)
{
  mData = data;
  mCurrentChannel = NULL;



  SetHasHotkeys();

  DetermineReferenceChannel();
  SetLinkChannels(mCurrentChannel);

  Display();
}

cMenuDirector::~cMenuDirector()
{
}

cOsdMenu* cMenuDirector::Create(const char* title,cOsdProxyData* data)
{
  if ((((cMenuDirectorData*)data)->mCalledByChannelSwitch) && (((cMenuDirectorData*)data)->mAskOnStart))
  {
    eKeys wPressed = Skins.Message(mtInfo, tr("Link(ed) channel - start arghDirector?"), 5);
    if (wPressed != kOk)
      return NULL;
  }
  if ((!((cMenuDirectorData*)data)->mCalledByChannelSwitch) && (((cMenuDirectorData*)data)->mAvoidStartOnSingleChannels))
  {
    cChannel* currentChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());
    if (!currentChannel || (!IsLinkChannel(currentChannel) && !IsLinkedChannel(currentChannel)))
    {
      Skins.Message(mtWarning, tr("Not a link(ed) channel!"), 3);
      return NULL;
    }
  }
  return new cMenuDirector(title, (cMenuDirectorData*)data);
}

void cMenuDirector::Release()
{
  mReferenceChannel = NULL;
  mData = NULL;
}

void cMenuDirector::ChannelSwitchNonOsd(cChannel* channel)
{
  cDevice::PrimaryDevice()->SwitchChannel(channel, true);
  if ((mData) && (mData->mDisplayChannelInfo == 1))
    Skins.Message(mtInfo,channel->Name());
}

void cMenuDirector::ChannelSwitchNonOsd(int direction)
{
  if (!mReferenceChannel)
    return;

  if ((mData) && (mData->mSwapKeys == 1))
    direction *= -1;

  if(GetLinkChannelsOutOfChannel(mReferenceChannel))
  {
    cChannel* currentChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());
    cLinkChannels* linkChannels = GetLinkChannelsOutOfChannel(mReferenceChannel);

    if ((currentChannel == mReferenceChannel) && (direction == -1))
    {
      ChannelSwitchNonOsd(linkChannels->Last()->Channel());
      return;
    }
    if ((currentChannel == mReferenceChannel) && (direction == +1))
    {
      ChannelSwitchNonOsd(linkChannels->First()->Channel());
      return;
    }
    
    cChannel* prevChannel = mReferenceChannel;
    for (cLinkChannel* linkChannel = linkChannels->First(); linkChannel != NULL; linkChannel = linkChannels->Next(linkChannel))
    {
      if (linkChannel->Channel() == currentChannel)
      {
        if (direction == -1)
        {
          ChannelSwitchNonOsd(prevChannel);
          return;
        }
        else
        {
          if (linkChannels->Next(linkChannel) == NULL)
            ChannelSwitchNonOsd(mReferenceChannel);
          else
            ChannelSwitchNonOsd(linkChannels->Next(linkChannel)->Channel());
          return;
        }
      }

      prevChannel = linkChannel->Channel();
    }
  }
}

eOSState cMenuDirector::KeyHandlerFunction(eKeys Key)
{
  if (Key == kUp)
  {
    ChannelSwitchNonOsd(-1);
  }
  else if (Key == kDown)
  {
    ChannelSwitchNonOsd(+1);
  }

  return osContinue;
}

eOSState cMenuDirector::ProcessKey(eKeys Key)
{
  if ((mData->mSwapKeysInMenu == 1) && (Key == kUp))
    Key = kDown;
  else if ((mData->mSwapKeysInMenu == 1) && (Key == kDown))
    Key = kUp;
  
  /*eOSState state = */cOsdMenu::ProcessKey(Key);

  if ((Key == kBack) || ((Key == mData->mHideButton) && (mData->mHideButton != kOk)))
    return osEnd;

  cChannel* itemChannel = GetChannel(Current());
  if ((itemChannel != mCurrentChannel) && (itemChannel))
  {
    if ((mData->mSwitchChannelOnItemSelect == 1) || (Key == kOk))
    {
      SwitchToChannel(itemChannel);
      if (mData->mHidePluginAfterChannelSwitch == 1)
        return osEnd;

      return osContinue;
    }
  }
  else if ((itemChannel == mCurrentChannel) && (Key == kOk))
    return osEnd;

  return osContinue;
}

void cMenuDirector::SwitchToChannel(cChannel* channel)
{
  if (!channel)
    return;

  cDevice::PrimaryDevice()->SwitchChannel(channel, true);
  mCurrentChannel = channel;
}

void cMenuDirector::DetermineReferenceChannel()
{
  cChannel* currentChannel = Channels.GetByNumber(cDevice::PrimaryDevice()->CurrentChannel());

  if (!mReferenceChannel)
  {
    cChannel* referenceChannel = currentChannel;
	
    if(!GetLinkChannelsOutOfChannel(referenceChannel))
      referenceChannel = GetRefChannelOutOfChannel(referenceChannel);
	
    mReferenceChannel = referenceChannel;
  }

  mCurrentChannel = currentChannel;
}

void cMenuDirector::AddChannelItem(cChannel* channel)
{
  if (mData->mDisplayShortcutNumbers == 1)
  {    
    /*char* buffer;
    int channelNo = cList<cOsdItem>::Count()+1;
    if (mData->mDisplayEpgInfo == 1)
      channelNo = channelNo/2+1;
    if (mData->mPortalmode == 1)
      channelNo--;

    if ((cList<cOsdItem>::First() == NULL) && (mData->mPortalmode == 1))
      asprintf(&buffer,"\t%s", channel->Name());
    else
      asprintf(&buffer," %d\t%s", channelNo ,channel->Name());

    Add(new cMenuDirectorItem(hk(buffer),channel));*/


    if ((cList<cOsdItem>::First() == NULL) && (mData->mPortalmode == 1))
      Add(new cMenuDirectorItem(channel->Name(),channel));
    else
      Add(new cMenuDirectorItem(hk(channel->Name()),channel));
  }
  else
    Add(new cMenuDirectorItem(channel->Name(),channel));

  if (mData->mDisplayEpgInfo == 1)
  {
    const char* title = NULL;
    char* osdtitle = NULL;
    cSchedulesLock schedulesLock;
    const cSchedules* schedules = cSchedules::Schedules(schedulesLock);
    const cSchedule* schedule = schedules->GetSchedule(channel->GetChannelID());
    if(schedule)
    { 
     const cEvent* event = schedule->GetPresentEvent();
     if (event)
       title = event->Title();
    }

    if (title)
      asprintf(&osdtitle,(mData->mDisplayShortcutNumbers == 1)?"\t%s":"%s", title);
    else
      asprintf(&osdtitle,(mData->mDisplayShortcutNumbers == 1)?"\t%s":"%s", tr("No EPG information available"));

    Add(new cOsdItem(osdtitle));
    cList<cOsdItem>::Last()->SetSelectable(false);

    free(osdtitle);
  }
}

void cMenuDirector::SetLinkChannels(cChannel* selectedChannel)
{
  if (mReferenceChannel != NULL)
  {
    int noOfChannels = 1;
    if(GetLinkChannelsOutOfChannel(mReferenceChannel))
      noOfChannels += GetLinkChannelsOutOfChannel(mReferenceChannel)->Count();
    SetCols((mData->mDisplayShortcutNumbers == 1)?(numdigits(noOfChannels) + 1):0);

    AddChannelItem(mReferenceChannel);

    if(GetLinkChannelsOutOfChannel(mReferenceChannel))
    {
      cLinkChannels* linkChannels = GetLinkChannelsOutOfChannel(mReferenceChannel);
      for (cLinkChannel* linkChannel = linkChannels->First(); linkChannel != NULL; linkChannel = linkChannels->Next(linkChannel))
      {
        //Add(new cMenuDirectorItem(hk(linkChannel->Channel()->Name()),linkChannel->Channel()));
        AddChannelItem(linkChannel->Channel());
      }
    }
  }
  else if (mCurrentChannel != NULL)
  {
    SetCols((mData->mDisplayShortcutNumbers == 1)?2:0);
    AddChannelItem(mCurrentChannel); 
  }

  SelectCurrentChannel();
}

void cMenuDirector::SelectCurrentChannel()
{
  for (cMenuDirectorItem* item = (cMenuDirectorItem*)First(); item != NULL; item = (cMenuDirectorItem*)Next(item))
  {
    if (item->Channel() == mCurrentChannel)
    {
      SetCurrent(item);
      break;
    }
  }
}

cChannel* cMenuDirector::GetChannel(int Index)
{
  cMenuDirectorItem* item = (cMenuDirectorItem*)Get(Index);
  return item ? (cChannel *)item->Channel() : NULL;
}
