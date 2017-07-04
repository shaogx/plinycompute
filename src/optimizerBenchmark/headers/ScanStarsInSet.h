/*****************************************************************************
 *                                                                           *
 *  Copyright 2018 Rice University                                           *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *      http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 *****************************************************************************/
#ifndef SCAN_STARSIN_SET_H
#define SCAN_STARSIN_SET_H

#include "ScanUserSet.h"
#include "StarsIn.h"

using namespace pdb;
class ScanStarsInSet : public ScanUserSet <StarsIn> {

public:
	ENABLE_DEEP_COPY

	 ScanStarsInSet () {
        }

	ScanStarsInSet (std :: string dbName, std :: string setName) {
            setDatabaseName( dbName );
            setSetName ( setName );
        }
};

#endif