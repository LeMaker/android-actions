/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/input.h>
#include <sys/sysinfo.h>
#include "common.h"
#include "device.h"
#include "screen_ui.h"

#include "cutils/properties.h"

//ActionsCode(author:liaotianyang, type:newcode, comment:modify menu)
static const char* HEADERS1[] = { "Volume up/down to move highlight;",
                                 "menu or power button to select.",
                                 "",
                                 NULL };
                                 
static const char* HEADERS2[] = { "Short press power button to move highlight;",
                                 "long press power button 3s to select.",
                                 "",
                                 NULL };

static const char* ITEMS[] =  {"reboot system now",
                               "apply update from ADB",
                               "apply update from sdcard", //ActionsCode(author:liaotianyang, type:changecode, comment:add menu)
                               "wipe data/factory reset",
                               "wipe cache partition",
                               //"reboot to bootloader", //ActionsCode(author:liaotianyang, type:changecode, comment:del menu)
                               //"power down",
							   "wipe media partition",  //ActionsCode(author:liaotianyang, type:changecode, comment:add menu)
                               "view recovery logs",
                               NULL };

int get_recovery_mode(void);
//END ActionsCode

//ActionsCode(author:liaotianyang, type:newcode, comment:get system memory cap)
static int get_memcap(void)
{
    struct sysinfo memf;
    int ret;
    ret = sysinfo(&memf);
    printf("ret=%d, mem=%d kb\n", ret, (int)(memf.totalram/1024));
    return memf.totalram/(1024*1024);
}

/*
 *check memsize to if allow adb update
 *ActionsCode(author:liaotianyang, type:newcode)
 */
static int is_adb_update(void)
{
	static int is_adb = -1;  
    char value[PROPERTY_VALUE_MAX+1];
	if ( is_adb >= 0 )
		return is_adb;
	
    int len = property_get("ro.debuggable", value, NULL);
    if (len >= 1 && value[0] == '0') {
		is_adb = 0;
		printf("adb disable\n");
		return is_adb;
    }
	is_adb = 0;
    if ( get_memcap() > 512 )
        is_adb = 1;
    return is_adb;
}

/*
 *check memsize to if skip adb update menu items
 *ActionsCode(author:liaotianyang, type:newcode)
 */
static const char* const* get_main_items(void)
{
    static const char* menu_item[11];
    int i;
    if ( is_adb_update() )
        return ITEMS;
    menu_item[0] = ITEMS[0];
    for ( i = 1; (i < 10)&& (ITEMS[i+1] !=NULL); i++)
        menu_item[i] = ITEMS[i+1];
    menu_item[i] = NULL;
    return menu_item;
}


class DefaultDevice : public Device {
  public:
    DefaultDevice() :
        ui(new ScreenRecoveryUI) {
    }

    RecoveryUI* GetUI() { return ui; }

    int HandleMenuKey(int key, int visible) {
        if (visible) {
            switch (key) {
              case KEY_DOWN:
              case KEY_VOLUMEDOWN:
                return kHighlightDown;

              case KEY_UP:
              case KEY_VOLUMEUP:
                return kHighlightUp;

              //ActionsCode(author:liaotianyang, type:changecode, comment:use menu&power key instead of enter key)
              case KEY_MENU:
              case KEY_POWER:
              //END ActionsCode
                return kInvokeItem;
            }
        }

        return kNoAction;
    }

    BuiltinAction InvokeMenuItem(int menu_position) {
		//ActionsCode(author:wurui, type:changecode, comment:skip adb update menu)
        if ( !is_adb_update() && menu_position > 0 )
            menu_position++; //skip adb update menu
        switch (menu_position) {
          case 0: return REBOOT;
          case 1: return APPLY_ADB_SIDELOAD;
          //ActionsCode(author:wurui, type:changecode, comment:add update from sdcard & add wipe_media)
          case 2: return APPLY_EXT;
		  case 3: return WIPE_DATA;
          case 4: return WIPE_CACHE;
          //case 4: return REBOOT_BOOTLOADER;
          //case 5: return SHUTDOWN;
		  case 5: return WIPE_MEDIA;
          case 6: return READ_RECOVERY_LASTLOG;
          default: return NO_ACTION;
        }
    }

    const char* const* GetMenuHeaders() {
        //ActionsCode(author:wurui, type:changecode, comment:modify menu)
        if(get_recovery_mode() == 1)
            return HEADERS2; 
        else
            return HEADERS1; 
    }
	//ActionsCode(author:wurui, type:changecode, comment:skip adb update menu)
    const char* const* GetMenuItems() { return get_main_items(); /*ITEMS;*/ }

  private:
    RecoveryUI* ui;
};

Device* make_device() {
    return new DefaultDevice();
}
