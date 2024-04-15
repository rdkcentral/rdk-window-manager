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

#include "rdkwindowmanagerdata.h"


namespace RdkWindowManager
{
    RdkWindowManagerData::RdkWindowManagerData() : mDataTypeIndex(typeid(void*))
    {
        mData.pointerData = nullptr;
        mData.stringData = nullptr;
    }

    RdkWindowManagerData::RdkWindowManagerData(bool data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(bool), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(int8_t data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(int8_t), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(int32_t data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(int32_t), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(int64_t data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(int64_t), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(uint8_t data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(uint8_t), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(uint32_t data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(uint32_t), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(uint64_t data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(uint64_t), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(float data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(float), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(double data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(double), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(std::string data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(std::string), &data);
    }

    RdkWindowManagerData::RdkWindowManagerData(void* data) : mDataTypeIndex(typeid(void*))
    {
        setData(typeid(void*), &data);
    }
    
    RdkWindowManagerData::~RdkWindowManagerData()
    {
        if (mDataTypeIndex == typeid(std::string))
        {
            if (nullptr != mData.stringData)
            {
              delete mData.stringData;
              mData.stringData = nullptr;
            }
        }
    }

    bool RdkWindowManagerData::toBoolean() const
    {
        if (mDataTypeIndex == typeid(bool))
        {
            return mData.booleanData;
        }
        return false;
    }

    int8_t RdkWindowManagerData::toInteger8() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (int8_t)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (int8_t)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (int8_t)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (int8_t)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (int8_t)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (int8_t)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (int8_t)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (int8_t)(mData.doubleData);
        }
        return 0;
    }

    int32_t RdkWindowManagerData::toInteger32() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (int32_t)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (int32_t)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (int32_t)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (int32_t)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (int32_t)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (int32_t)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (int32_t)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (int32_t)(mData.doubleData);
        }
        return 0;
    }

    int64_t RdkWindowManagerData::toInteger64() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (int64_t)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (int64_t)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (int64_t)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (int64_t)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (int64_t)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (int64_t)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (int64_t)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (int64_t)(mData.doubleData);
        }
        return 0;
    }
    
    uint8_t RdkWindowManagerData::toUnsignedInteger8() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (uint8_t)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (uint8_t)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (uint8_t)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (uint8_t)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (uint8_t)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (uint8_t)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (uint8_t)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (uint8_t)(mData.doubleData);
        }
        return 0;
    }

    uint32_t RdkWindowManagerData::toUnsignedInteger32() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (uint32_t)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (uint32_t)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (uint32_t)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (uint32_t)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (uint32_t)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (uint32_t)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (uint32_t)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (uint32_t)(mData.doubleData);
        }
        return 0;
    }

    uint64_t RdkWindowManagerData::toUnsignedInteger64() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (uint64_t)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (uint64_t)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (uint64_t)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (uint64_t)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (uint64_t)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (uint64_t)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (uint64_t)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (uint64_t)(mData.doubleData);
        }
        return 0;
    }

    float RdkWindowManagerData::toFloat() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (float)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (float)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (float)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (float)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (float)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (float)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (float)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (float)(mData.doubleData);
        }
        return 0;
    }

    double RdkWindowManagerData::toDouble() const
    {
        if (mDataTypeIndex == typeid(int8_t))
        {
            return (double)(mData.integer8Data);
        }
        else if (mDataTypeIndex == typeid(int32_t))
        {
            return (double)(mData.integer32Data);
        }
        else if (mDataTypeIndex == typeid(int64_t))
        {
            return (double)(mData.integer64Data);
        }
        else if (mDataTypeIndex == typeid(uint8_t))
        {
            return (double)(mData.unsignedInteger8Data);
        }
        else if (mDataTypeIndex == typeid(uint32_t))
        {
            return (double)(mData.unsignedInteger32Data);
        }
        else if (mDataTypeIndex == typeid(uint64_t))
        {
            return (double)(mData.unsignedInteger64Data);
        }
        else if (mDataTypeIndex == typeid(float))
        {
            return (double)(mData.floatData);
        }
        else if (mDataTypeIndex == typeid(double))
        {
            return (double)(mData.doubleData);
        }
        return 0;
    }
    std::string RdkWindowManagerData::toString() const
    {
        if (mDataTypeIndex == typeid(std::string) && (nullptr != mData.stringData))
        {
            return *(mData.stringData);
        }
        return "";
    }

    void* RdkWindowManagerData::toVoidPointer() const
    {
        if (mDataTypeIndex == typeid(void*))
        {
            return mData.pointerData;
        }
        return nullptr;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(bool value)
    {
        setData(typeid(bool), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(int8_t value)
    {
        setData(typeid(int8_t), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(int32_t value)
    {
        setData(typeid(int32_t), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(int64_t value)
    {
        setData(typeid(int64_t), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(uint8_t value)
    {
        setData(typeid(uint8_t), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(uint32_t value)
    {
        setData(typeid(uint32_t), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(uint64_t value)
    {
        setData(typeid(uint64_t), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(float value)
    {
        setData(typeid(float), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(double value)
    {
        setData(typeid(double), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(const char* value)
    {
        std::string tempString = value;
        setData(typeid(std::string), &tempString);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(const std::string& value)
    {
        std::string tempString = value;
        setData(typeid(std::string), &tempString);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(void* value)
    {
        setData(typeid(void*), &value);
        return *this;
    }

    RdkWindowManagerData& RdkWindowManagerData::operator=(const RdkWindowManagerData& value)
    {
        std::type_index typeIndex = value.mDataTypeIndex;
        
        if (typeIndex == typeid(bool))
        {
            mData.booleanData = value.mData.booleanData;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(int8_t))
        {
            mData.integer8Data = value.mData.integer8Data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(int32_t))
        {
            mData.integer32Data = value.mData.integer32Data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(int64_t))
        {
            mData.integer64Data = value.mData.integer64Data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(uint8_t))
        {
            mData.unsignedInteger8Data = value.mData.unsignedInteger8Data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(uint32_t))
        {
            mData.unsignedInteger32Data = value.mData.unsignedInteger32Data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(uint64_t))
        {
            mData.unsignedInteger64Data = value.mData.unsignedInteger64Data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(float))
        {
            mData.floatData = value.mData.floatData;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(double))
        {
            mData.doubleData = value.mData.doubleData;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(std::string))
        {
            if (nullptr == mData.stringData)
            {
              mData.stringData = new std::string("");
            }
            *(mData.stringData) = (nullptr != value.mData.stringData)?(*(value.mData.stringData)):"";
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(void*))
        {
            mData.pointerData = value.mData.pointerData;
            mDataTypeIndex = typeIndex;
        }
        return *this;
    }


    std::type_index RdkWindowManagerData::dataTypeIndex()
    {
        return mDataTypeIndex;
    }

    void RdkWindowManagerData::setData(std::type_index typeIndex, void* data)
    {
        if (typeIndex == typeid(bool))
        {
            mData.booleanData = *((bool*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(int8_t))
        {
            mData.integer8Data = *((int8_t*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(int32_t))
        {
            mData.integer32Data = *((int32_t*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(int64_t))
        {
            mData.integer64Data = *((int64_t*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(uint8_t))
        {
            mData.unsignedInteger8Data = *((uint8_t*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(uint32_t))
        {
            mData.unsignedInteger32Data = *((uint32_t*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(uint64_t))
        {
            mData.unsignedInteger64Data = *((uint64_t*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(float))
        {
            mData.floatData = *((float*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(double))
        {
            mData.doubleData = *((double*)data);
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(std::string))
        {
            if (nullptr == mData.stringData)
            {
              mData.stringData = new std::string("");
            }
            *(mData.stringData) = *(std::string*)data;
            mDataTypeIndex = typeIndex;
        }
        else if (typeIndex == typeid(void*))
        {
            mData.pointerData = data;
            mDataTypeIndex = typeIndex;
        }
    }
}
