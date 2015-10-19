/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _DISPLAY_PARAMETERS_H__
#define _DISPLAY_PARAMETERS_H__

#include <utils/Log.h>
#include <string.h>
#include <stdlib.h>
#include <utils/String8.h>
#include <utils/KeyedVector.h>

namespace android {

class DisplayParameters{
    public:    
        DisplayParameters();    
        DisplayParameters(const String8 &params) 
        {   
            unflatten(params); 
        }    
        ~DisplayParameters();    
        String8 flatten() const;    
        void unflatten(const char *str);    
        void set(const char *key, const char *value);    
        void setInt(const char *key, int value);    
        void setFloat(const char *key, float value);    

        const char *get(const char *key) const;    
        int getInt(const char *key) const;    
        float getFloat(const char *key) const;   
        
        void remove(const char *key); 
        void dump() const;
        status_t dump(int fd, const Vector<String16>& args) const;
        
        // Parameter keys to communicate between jni and java
        static const char  KEY_FORMAT[] ;
        static const char  KEY_AUDIO_CHAN[];
        static const char  KEY_VIDEO_SCALE[] ;
        static const char  KEY_COLORKEY[] ;
        static const char  KEY_COLOR[];
        static const char  KEY_ALPHA[];
        static const char  KEY_HDMI_RES_WIDTH[];
        static const char  KEY_HDMI_RES_HEIGHT[];
        static const char  KEY_HDMI_RES_HZ[];
        static const char  KEY_HDMI_RES_PG[];
        static const char  KEY_HDMI_RES_ASPECT[];
        static const char  KEY_SCALE_X[];
        static const char  KEY_SCALE_Y[]; 
        static const char  KEY_YPBPR_RES_WIDTH[];
        static const char  KEY_YPBPR_RES_HEIGHT[];
        static const char  KEY_YPBPR_RES_HZ[];
        static const char  KEY_YPBPR_RES_PG[];
        static const char  KEY_YPBPR_RES_ASPECT[];


        // Paramerter keys about Display Device
        static const char KEY_ID[];
        static const char KEY_WIDTH[];
        static const char KEY_HEIGHT[];
        static const char KEY_NAME[];
        static const char KEY_DES[] ;
   private:    
    DefaultKeyedVector<String8,String8>    mMap;
    

   };
};
#endif
