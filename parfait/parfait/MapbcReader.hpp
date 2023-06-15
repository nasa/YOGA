
// Copyright 2016 United States Government as represented by the Administrator of the National Aeronautics and Space
// Administration. No copyright is claimed in the United States under Title 17, U.S. Code. All Other Rights Reserved.
//
// The “Parfait: A Toolbox for CFD Software Development [LAR-18839-1]” platform is licensed under the
// Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
#pragma once
#include <stdio.h>
#include <assert.h>
#include <string>
#include "FileTools.h"

inline Parfait::MapbcReader::MapbcReader(std::string filename_in) : filename(filename_in) {
    if (not FileTools::doesFileExist(filename)) {
        failedToOpenMapbc = true;
        printf("Mapbc file not found: %s\n", filename.c_str());
    } else {
        auto contents = FileTools::loadFileToString(filename);
        bcMap = MapbcParser::parse(contents);
        failedToOpenMapbc = false;
    }
}

inline int Parfait::MapbcReader::boundaryCondition(int tag) {
    if (failedToOpenMapbc) return tag;
    return bcMap.find(tag)->second.first;
}

inline void Parfait::MapbcReader::print() {
    if (!failedToOpenMapbc) {
        printf("Mapbc contents:\n");
        for (auto& item : bcMap) {
            printf("Tag: %i Boundary Condition: %i\n", item.first, item.second.first);
        }
    }
}
