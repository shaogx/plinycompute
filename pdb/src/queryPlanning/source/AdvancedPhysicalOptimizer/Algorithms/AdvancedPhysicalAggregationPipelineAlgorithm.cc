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

#include <JobStageBuilders/TupleSetJobStageBuilder.h>
#include <AdvancedPhysicalOptimizer/Algorithms/AdvancedPhysicalAggregationPipelineAlgorithm.h>
#include <JobStageBuilders/AggregationJobStageBuilder.h>

namespace pdb {

AdvancedPhysicalAggregationPipelineAlgorithm::AdvancedPhysicalAggregationPipelineAlgorithm(const AdvancedPhysicalPipelineNodePtr &handle,
                                                                           const std::string &jobID,
                                                                           bool isProbing,
                                                                           bool isOutput,
                                                                           const Handle<SetIdentifier> &source,
                                                                           const Handle<ComputePlan> &computePlan,
                                                                           const LogicalPlanPtr &logicalPlan,
                                                                           const ConfigurationPtr &conf)
                                                                           : AdvancedPhysicalAbstractAlgorithm(handle,
                                                                                                               jobID,
                                                                                                               isProbing,
                                                                                                               isOutput,
                                                                                                               source,
                                                                                                               computePlan,
                                                                                                               logicalPlan,
                                                                                                               conf) {}
PhysicalOptimizerResultPtr AdvancedPhysicalAggregationPipelineAlgorithm::generate(int nextStageID,
                                                                                  const StatisticsPtr &stats) {

  // extract the atomic computations from the pipes for this algorithm
  extractAtomicComputations();

  // extract the hash sets we might want to probe
  extractHashSetsToProbe();

  // get the final atomic computation
  auto finalAtomicComputation = this->pipelineComputations.back();

  // get the source atomic computation
  auto sourceAtomicComputation = this->pipelineComputations.front();

  // create a analyzer result
  PhysicalOptimizerResultPtr result = make_shared<PhysicalOptimizerResult>();

  // grab the output of the current node
  std::string outputName = finalAtomicComputation->getOutputName();

  // the computation specifier of this aggregation
  std::string computationSpecifier = finalAtomicComputation->getComputationName();

  // grab the computation associated with this node
  Handle<Computation> curComp = logicalPlan->getNode(computationSpecifier).getComputationHandle();

  // I am a pipeline breaker because I have more than one consumers
  Handle<SetIdentifier> sink = nullptr;

  // in the case that the current computation does not require materialization by default
  // we have to set an output to it, we it gets materialized
  if (!curComp->needsMaterializeOutput() && pipeline.back()->getNumConsumers() == 1) {

    sink = makeObject<SetIdentifier>(jobID,
                                     finalAtomicComputation->getOutputName() + "_aggregationResult",
                                     PartitionedHashSetType,
                                     true);
  }
  else if(!curComp->needsMaterializeOutput()){

    // set the output
    curComp->setOutput(jobID, finalAtomicComputation->getOutputName());

    // create the sink and set the page size
    sink = makeObject<SetIdentifier>(jobID, finalAtomicComputation->getOutputName());
    sink->setPageSize(conf->getPageSize());

    // add this set to the list of intermediate sets
    result->interGlobalSets.push_back(sink);

  } else {
    // this computation needs materialization either way so just create the sink set identifier
    sink = makeObject<SetIdentifier>(curComp->getDatabaseName(), curComp->getSetName());
  }

  // create the set identifier where we store the data to be aggregated after the TupleSetJobStage
  Handle<SetIdentifier> aggregator = makeObject<SetIdentifier>(jobID, finalAtomicComputation->getOutputName() + "_aggregationData");
  aggregator->setPageSize(conf->getShufflePageSize());

  // are we using a combiner (the thing that combines the records by key before sending them to the right node)
  Handle<SetIdentifier> combiner = nullptr;
  if (curComp->isUsingCombiner()) {
    // create a set identifier for the combiner
    combiner = makeObject<SetIdentifier>(jobID, finalAtomicComputation->getOutputName() + "_combinerData");
  }

  // create a tuple set job stage builder
  TupleSetJobStageBuilderPtr tupleStageBuilder = make_shared<TupleSetJobStageBuilder>();

  // the input to the pipeline is the output set of the source node
  tupleStageBuilder->setSourceTupleSetName(sourceAtomicComputation->getOutputName());

  // set the source set identifier
  tupleStageBuilder->setSourceContext(source);

  // is this source a result of an aggregation
  tupleStageBuilder->setInputAggHashOut(source->isAggregationResult());

  // set the job id
  tupleStageBuilder->setJobId(jobID);

  // are we probing a hash set in this pipeline
  tupleStageBuilder->setProbing(isProbing);

  // set the compute plan
  tupleStageBuilder->setComputePlan(computePlan);

  // copy the computation names
  for(auto it = this->pipelineComputations.begin(); it != this->pipelineComputations.end(); ++it) {

    // grab the computation
    auto &comp = *it;

    // we don't need the output set name... (that is just the way the pipeline building works)
    // the aggregation is not applied here it is applied in the next job stage
    if(comp->getAtomicComputationTypeID() == WriteSetTypeID ||
      (it != this->pipelineComputations.begin() && comp->getAtomicComputationTypeID() == ApplyAggTypeID)) {
      continue;
    }

    // add the set name of the atomic computation to the pipeline
    tupleStageBuilder->addTupleSetToBuildPipeline(comp->getOutputName());
  }

  // create the tuple set job stage to run the pipeline with a shuffle sink
  // here is what we are doing :
  // the input to the stage is either the output of the join or the source node we started)
  // the repartitioning flag is set to true, so that we run a pipeline with a shuffle sink
  // the pipeline until the combiner will apply all the computations to the source set
  // and put them on a page partitioned into multiple maps the combiner will then read the maps that belong to
  // the partitions of a certain node and combine them by key. The output pages of the combiner will then be sent
  tupleStageBuilder->setJobStageId(nextStageID++);
  tupleStageBuilder->setTargetTupleSetName(finalAtomicComputation->getInputName());
  tupleStageBuilder->setTargetComputationName(computationSpecifier);
  tupleStageBuilder->setOutputTypeName("IntermediateData");
  tupleStageBuilder->setCombiner(combiner);
  tupleStageBuilder->setSinkContext(aggregator);
  tupleStageBuilder->setRepartition(true);
  tupleStageBuilder->setAllocatorPolicy(curComp->getAllocatorPolicy());

  // add all the probing hash sets
  for(auto it : probingHashSets) {
    tupleStageBuilder->addHashSetToProbe(it.first, it.second);
  }

  // add the created tuple job stage to the
  result->physicalPlanToOutput.emplace_back(tupleStageBuilder->build());

  // cast the computation to AbstractAggregateComp to create the consuming job stage for aggregation
  Handle<AbstractAggregateComp> agg = unsafeCast<AbstractAggregateComp, Computation>(curComp);

  // we need an aggregation stage after this to aggregate the results from the tuple stage we previously created
  // the data will be aggregated in the sink set
  AggregationJobStageBuilderPtr aggregationBuilder = make_shared<AggregationJobStageBuilder>();

  aggregationBuilder->setJobId(jobID);
  aggregationBuilder->setJobStageId(nextStageID);
  aggregationBuilder->setAggComp(agg);
  aggregationBuilder->setSourceContext(aggregator);
  aggregationBuilder->setSinkContext(sink);
  aggregationBuilder->setMaterializeOrNot(curComp->needsMaterializeOutput());

  // to push back the aggregation stage;
  result->physicalPlanToOutput.emplace_back(aggregationBuilder->build());

  // to push back the aggregator set
  result->interGlobalSets.push_back(aggregator);

  // update the source pipes to reflect the state after executing the job stages
  // if we have a consumer we have a new source pipe since we materialize this result
  if(pipeline.back()->getNumConsumers() != 0) {

    // add consumers as new sources
    for(int i = 0; i < pipeline.back()->getNumConsumers(); ++i) {
      result->createdSourceComputations.push_back(pipeline.back()->getConsumer(i));
    }
  }

  // we succeeded
  result->success = true;

  // the new source is the sink
  updateConsumers(sink, approximateResultSize(stats), stats);

  return result;
}

AdvancedPhysicalAbstractAlgorithmTypeID AdvancedPhysicalAggregationPipelineAlgorithm::getType() {
  return AGGREGATION_ALGORITHM;
}


}

