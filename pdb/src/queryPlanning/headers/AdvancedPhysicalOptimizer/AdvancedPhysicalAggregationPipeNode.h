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

#ifndef PDB_AGGREGATIONPIPENODE_H
#define PDB_AGGREGATIONPIPENODE_H

#include "AdvancedPhysicalNode.h"

namespace pdb {

class AdvancedPhysicalAggregationPipeNode : public AdvancedPhysicalNode {
 public:
  AdvancedPhysicalAggregationPipeNode(string &jobId,
                                      Handle<ComputePlan> &computePlan,
                                      LogicalPlanPtr &logicalPlan,
                                      ConfigurationPtr &conf,
                                      vector<AtomicComputationPtr> &pipeComputations);

};

}



#endif //PDB_AGGREGATIONPIPENODE_H
