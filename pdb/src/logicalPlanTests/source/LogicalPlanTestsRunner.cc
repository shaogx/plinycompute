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
#include "LogicalPlanTestsRunner.h"

#include "BuildLogicalPlanTests.h"

namespace pdb_tests {
void runLogicalPlanTests(UnitTest& qunit) {
    testBuildLogicalPlanFromLoad(qunit);
    testBuildLogicalPlanFromApplyFunction(qunit);
    testBuildLogicalPlanFromApplyMethod(qunit);
    testBuildLogicalPlanFromHoist(qunit);
    testBuildLogicalPlanFromGreaterThan(qunit);
    testBuildLogicalPlanFromFilter(qunit);
    testBuildLogicalPlanFromStore(qunit);
    testBuildLogicalPlanFromStringTest47(qunit);
}
}