/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#pragma once

#include <typeindex>
#include <string>

namespace RdkWindowManager
{
    union RdkWindowManagerDataInfo
    {
        bool        booleanData;
        int8_t      integer8Data;
        int32_t     integer32Data;
        int64_t     integer64Data;
        uint8_t     unsignedInteger8Data;
        uint32_t    unsignedInteger32Data;
        uint64_t    unsignedInteger64Data;
        float       floatData;
        double      doubleData;
        std::string* stringData;
        void*       pointerData;
        RdkWindowManagerDataInfo() {}
        ~RdkWindowManagerDataInfo() {}
    };

    class RdkWindowManagerData
    {
        public:
            RdkWindowManagerData();
            ~RdkWindowManagerData();
            RdkWindowManagerData(bool data);
            RdkWindowManagerData(int8_t data);
            RdkWindowManagerData(int32_t data);
            RdkWindowManagerData(int64_t data);
            RdkWindowManagerData(uint8_t data);
            RdkWindowManagerData(uint32_t data);
            RdkWindowManagerData(uint64_t data);
            RdkWindowManagerData(float data);
            RdkWindowManagerData(double data);
            RdkWindowManagerData(std::string data);
            RdkWindowManagerData(void* data);
            
            bool toBoolean() const;
            int8_t toInteger8() const;
            int32_t toInteger32() const;
            int64_t toInteger64() const;
            uint8_t toUnsignedInteger8() const;
            uint32_t toUnsignedInteger32() const;
            uint64_t toUnsignedInteger64() const;
            float toFloat() const;
            double toDouble() const;
            std::string toString() const;
            void* toVoidPointer() const;

            RdkWindowManagerData& operator=(bool value);
            RdkWindowManagerData& operator=(int8_t value);
            RdkWindowManagerData& operator=(int32_t value);
            RdkWindowManagerData& operator=(int64_t value);
            RdkWindowManagerData& operator=(uint8_t value);
            RdkWindowManagerData& operator=(uint32_t value);
            RdkWindowManagerData& operator=(uint64_t value);
            RdkWindowManagerData& operator=(float value);
            RdkWindowManagerData& operator=(double value);
            RdkWindowManagerData& operator=(const char* value);
            RdkWindowManagerData& operator=(const std::string& value);
            RdkWindowManagerData& operator=(void* value);
            RdkWindowManagerData& operator=(const RdkWindowManagerData& value);

            std::type_index dataTypeIndex();

        private:
            std::type_index mDataTypeIndex;
            RdkWindowManagerDataInfo mData;

            void setData(std::type_index typeIndex, void* data);
    };
}
