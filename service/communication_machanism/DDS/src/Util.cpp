/*
 * Copyright (c) 2023 IPADS, Shanghai Jiao Tong University.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <random>
#include <sstream>
#include <iomanip>
#include "Util.h"

std::string get_random_guid()
{
    static std::random_device rd;
    static std::uniform_int_distribution<uint64_t> dist(0ULL,
                                                        0xFFFFFFFFFFFFFFFFULL);
    uint64_t ab = dist(rd);
    uint64_t cd = dist(rd);
    uint32_t a, b, c, d;
    std::stringstream ss;
    ab = (ab & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    cd = (cd & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    a = (ab >> 32U);
    b = (ab & 0xFFFFFFFFU);
    c = (cd >> 32U);
    d = (cd & 0xFFFFFFFFU);
    ss << std::hex << std::nouppercase << std::setfill('0');
    ss << std::setw(QWORD) << (a);           // << '-';
    ss << std::setw(DWORD) << (b >> 16U);    // << '-';
    ss << std::setw(DWORD) << (b & 0xFFFFU); // << '-';
    ss << std::setw(DWORD) << (c >> 16U);    // << '-';
    ss << std::setw(DWORD) << (c & 0xFFFFU);
    ss << std::setw(QWORD) << d;

    return ss.str();
}
