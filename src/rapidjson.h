// Copyright (c) 2018 Baidu, Inc. All Rights Reserved.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef  DMKIT_RAPIDJSON_H
#define  DMKIT_RAPIDJSON_H

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-local-typedefs"

#endif

#include "thirdparty/rapidjson/allocators.h"
#include "thirdparty/rapidjson/document.h"
#include "thirdparty/rapidjson/encodedstream.h"
#include "thirdparty/rapidjson/encodings.h"
#include "thirdparty/rapidjson/filereadstream.h"
#include "thirdparty/rapidjson/filewritestream.h"
#include "thirdparty/rapidjson/fwd.h"
#include "thirdparty/rapidjson/istreamwrapper.h"
#include "thirdparty/rapidjson/memorybuffer.h"
#include "thirdparty/rapidjson/memorystream.h"
#include "thirdparty/rapidjson/ostreamwrapper.h"
#include "thirdparty/rapidjson/pointer.h"
#include "thirdparty/rapidjson/prettywriter.h"
#include "thirdparty/rapidjson/rapidjson.h"
#include "thirdparty/rapidjson/reader.h"
#include "thirdparty/rapidjson/schema.h"
#include "thirdparty/rapidjson/stream.h"
#include "thirdparty/rapidjson/stringbuffer.h"
#include "thirdparty/rapidjson/writer.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#pragma GCC diagnostic pop
#endif

#endif  //DMKIT_RAPIDJSON_H
