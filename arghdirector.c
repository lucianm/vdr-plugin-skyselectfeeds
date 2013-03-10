/*
 * director.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/menuitems.h>
#include <vdr/plugin.h>
#include <vdr/status.h>
#include <vdr/remote.h>
#include <vdr/tools.h>

#include "menudirector.h"
#include "osdproxy.h"

static const char *VERSION        = "0.2.8_pre20130310";
static const char *DESCRIPTION    = "skin-aware plugin to use the Sky multifeed option";
static const char *MAINMENUENTRY  = "arghDirector";

const char* allowedButtonNames[] = {
  "Ok",
  "Red",
  "Green",
  "Yellow",
  "Blue",
  "User1",
  "User2",
  "User3",
  "User4",
  "User5",
  "User6",
  "User7",
  "User8",
  "User9",
};

eKeys allowedButtons[] =
{
  kOk,
  kRed,
  kGreen,
  kYellow,
  kBlue,
  kUser1,
  kUser2,
  kUser3,
  kUser4,
  kUser5,
  kUser6,
  kUser7,
  kUser8,
  kUser9
};

#define NOOFALLOWEDBUTTONS 14

int hide = 0;
int portalmode = 1;
int swapKeys = 0;
int swapKeysInMenu = 0;
int displayChannelInfo = 1;
int switchChannelOnItemSelect = 1;
int hidePluginAfterChannelSwitch = 0;
int displayShortcutNumbers = 1;
int displayEpgInfo = 1;
int showPluginButton = 0;
int autoStart = 1;
int autoStartRef = 1;
int autoStartAsk = 1;
int avoidStartOnNoLinkChannels = 1;

int calledByChannelSwitch = 0;

class cDirectorStatus : public cStatus
{
private:
  cPlugin* parent;
protected:
  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
public:
  cDirectorStatus(cPlugin* plugin);
};

class cPluginDirector : public cPlugin
{
private:	
  // Add any member variables or functions you may need here.
  cDirectorStatus *directorStatus;  

public:
  cPluginDirector(void);
  virtual ~cPluginDirector();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Start(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return (hide ? NULL : tr(MAINMENUENTRY)); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
};


cDirectorStatus::cDirectorStatus(cPlugin* plugin)
{
  parent = plugin;
}
	
void cDirectorStatus::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
  // pre-channel-switch - we are not interested
  if (ChannelNumber == 0)
    return;
  // channelno not no of primdevice - puuh no thx
  if (cDevice::CurrentChannel() != ChannelNumber)
    return;
  // all autostarts turned off - go away dude!
  if ((!autoStart) && (!autoStartRef))
    return;
 
  const cChannel* Channel = Channels.GetByNumber(ChannelNumber);//Device->CurrentChannel());
  if (!Channel)
    return;

  bool wAutoStart = false;
  if (autoStart)
  {
    if ((Channel->LinkChannels() != NULL) && (Channel->LinkChannels()->Count() > 1) && parent)
      wAutoStart = true;
  }
  if (autoStartRef)
  {
    if ((Channel->RefChannel() != NULL) && (Channel->RefChannel()->LinkChannels() != NULL) && (Channel->RefChannel()->LinkChannels()->Count() >= 1) && parent)
      wAutoStart = true;
  }

  if (wAutoStart)
  {
    calledByChannelSwitch = 1;
    cRemote::CallPlugin(parent->Name());
  }
}

cPluginDirector::cPluginDirector(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  directorStatus = NULL;
}

cPluginDirector::~cPluginDirector()
{
  // Clean up after yourself!
  if (directorStatus)
    delete directorStatus;
}

const char *cPluginDirector::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginDirector::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginDirector::Start(void)
{
  // Start any background activities the plugin shall perform.

  //if(autoStart == 1)
  directorStatus = new cDirectorStatus(this);

  // Default values for setup
  return true;
}

void cPluginDirector::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

cOsdObject *cPluginDirector::MainMenuAction(void)
{
  cMenuDirectorData* config = new cMenuDirectorData();
  config->mHidePluginEntryInMainMenu = hide;
  config->mPortalmode = portalmode;
  config->mSwapKeys = swapKeys;
  config->mDisplayChannelInfo = displayChannelInfo;
  config->mSwitchChannelOnItemSelect = switchChannelOnItemSelect;
  config->mHidePluginAfterChannelSwitch = hidePluginAfterChannelSwitch;
  config->mDisplayShortcutNumbers = displayShortcutNumbers;
  config->mDisplayEpgInfo = displayEpgInfo;
  config->mSwapKeysInMenu = swapKeysInMenu;
  config->mHideButton = allowedButtons[showPluginButton];
  config->mAskOnStart = autoStartAsk;
  config->mAvoidStartOnSingleChannels = avoidStartOnNoLinkChannels ;
  config->mCalledByChannelSwitch = calledByChannelSwitch;

  // directly reset
  calledByChannelSwitch = 0;

  return new cOsdProxy(cMenuDirector::Create,cMenuDirector::KeyHandlerFunction,cMenuDirector::Release,"Director",config, allowedButtons[showPluginButton],true);
}

bool cPluginDirector::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  if (!strcasecmp(Name, "HideMenu"))
    hide = atoi(Value);
  else if (!strcasecmp(Name, "SwapKeys"))
    swapKeys = atoi(Value);
  else if (!strcasecmp(Name, "SwapKeysInMenu"))
    swapKeysInMenu = atoi(Value);
  else if (!strcasecmp(Name, "PortalMode"))
    portalmode = atoi(Value);
  else if (!strcasecmp(Name, "DisplayChannelInfo"))
    displayChannelInfo = atoi(Value);
  else if (!strcasecmp(Name, "SwitchChannelOnItemSelect"))
    switchChannelOnItemSelect = atoi(Value);
  else if (!strcasecmp(Name, "HidePluginAfterChannelSwitch"))
    hidePluginAfterChannelSwitch = atoi(Value);
  else if (!strcasecmp(Name, "DisplayShortcutNumbers"))
    displayShortcutNumbers= atoi(Value);
  else if (!strcasecmp(Name, "DisplayEpgInfo"))
    displayEpgInfo= atoi(Value);
  else if (!strcasecmp(Name, "ShowPluginButton"))
    showPluginButton = atoi(Value);
  else if (!strcasecmp(Name, "AutoStart"))
    autoStart = atoi(Value);
  else if (!strcasecmp(Name, "AutoStartRef"))
    autoStartRef = atoi(Value);
  else if (!strcasecmp(Name, "AutoStartAsk"))
    autoStartAsk = atoi(Value);
  else if (!strcasecmp(Name, "AvoidStartOnNoLinkChannels"))
    avoidStartOnNoLinkChannels = atoi(Value);

  return true;
}

//the setup part
class cMenuSetupDirector : public cMenuSetupPage
{
public:
  cMenuSetupDirector();

protected:
  virtual void Store(void);

private:
  int new_hide;
};

cMenuSetupDirector::cMenuSetupDirector()
{
  new_hide = hide;    
  Add(new cMenuEditBoolItem(tr("Hide main menu entry"), &hide, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Swap up and down"), &swapKeys, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Swap up and down in menu"), &swapKeysInMenu, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Portal Mode"), &portalmode, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Display info on channel change"), &displayChannelInfo, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Switch channel on linkchannel selection"), &switchChannelOnItemSelect, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Hide plugin after channel switch"), &hidePluginAfterChannelSwitch, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Display channel shortcut numbers"), &displayShortcutNumbers, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Display EPG information"), &displayEpgInfo, tr("no"), tr("yes")));
  Add(new cMenuEditStraItem(tr("Show plugin again on button"), &showPluginButton, NOOFALLOWEDBUTTONS, allowedButtonNames));
  Add(new cMenuEditBoolItem(tr("Autostart plugin on reference-channels"), &autoStart, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Autostart plugin on referenced channels"), &autoStartRef, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Ask before autostart plugin"), &autoStartAsk, tr("no"), tr("yes")));
  Add(new cMenuEditBoolItem(tr("Avoid start on nonlink channels"), &avoidStartOnNoLinkChannels, tr("no"), tr("yes")));
}

void cMenuSetupDirector::Store(void)
{
  SetupStore("HideMenu", new_hide = hide);
  SetupStore("SwapKeys", swapKeys);
  SetupStore("SwapKeysInMenu", swapKeysInMenu);
  SetupStore("PortalMode", portalmode);
  SetupStore("DisplayChannelInfo", displayChannelInfo);
  SetupStore("SwitchChannelOnItemSelect", switchChannelOnItemSelect);
  SetupStore("DisplayShortcutNumbers", hidePluginAfterChannelSwitch);
  SetupStore("HidePluginAfterChannelSwitch", displayShortcutNumbers);
  SetupStore("DisplayEpgInfo", displayEpgInfo);
  SetupStore("ShowPluginButton", showPluginButton);
  SetupStore("AutoStart", autoStart);
  SetupStore("AutoStartRef", autoStartRef);
  SetupStore("AutoStartAsk", autoStartAsk);
  SetupStore("AvoidStartOnNoLinkChannels", avoidStartOnNoLinkChannels);
}

cMenuSetupPage *cPluginDirector::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new cMenuSetupDirector();
}

VDRPLUGINCREATOR(cPluginDirector); // Don't touch this!
