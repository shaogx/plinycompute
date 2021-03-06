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

#ifndef HASH_PARTITION_TRANSFORMATION_SINK_H
#define HASH_PARTITION_TRANSFORMATION_SINK_H


#include "ComputeSink.h"
#include "TupleSetMachine.h"
#include "TupleSet.h"
#include "DataTypes.h"
#include <vector>

namespace pdb {

// runs hashes all of the tuples, and stores all tuples to a container that is partitioned
// by node partitions.
template <class ValueType>
class HashPartitionTransformationSink : public ComputeSink {


public:

    /**
     * constructor.
     * @param numPartitions: number of partitions in the cluster
     * @param inputSchema: the schema of input tuple set
     * @param attToOperateOn: the column that we want to partition and keep
     */
    HashPartitionTransformationSink(int numPartitions, int numNodes, TupleSpec& inputSchema, TupleSpec& attToOperateOn) {

        // to setup the output tuple set
        TupleSpec empty;
        TupleSetSetupMachine myMachine(inputSchema, empty);

        // this is the input attribute that we will process
        std::vector<int> matches = myMachine.match(attToOperateOn);
        whichAttToHash = matches[0];
        this->numPartitions = numPartitions;
        this->numNodes = numNodes;
    }

    /**
     * create container for output
     * @return: a new container for partitioned output
     */
    Handle<Object> createNewOutputContainer() override {

        // we create a node-partitioned vector to store the output
        // we create a node-partitioned vector to store the output
        Handle<Vector<Handle<Vector<Handle<Vector<ValueType>>>>>> returnVal =
            makeObject<Vector<Handle<Vector<Handle<Vector<ValueType>>>>>>(numNodes);
        int i, j;
        for (i = 0; i < numNodes; i++) {
            Handle<Vector<Handle<Vector<ValueType>>>> curNodeVec
                = makeObject<Vector<Handle<Vector<ValueType>>>>(numPartitions/numNodes);
            for (j = 0; j < numPartitions/numNodes; j++) {
                Handle<Vector<ValueType>> curVec = makeObject<Vector<ValueType>>();
                curNodeVec->push_back(curVec);
            }
            returnVal->push_back(curNodeVec);
        }
        return returnVal;

    }


    /**
     * partition the input tuple set, and store the partitioned result to the output container
     * @param input: the input tuple set
     * @param writeToMe: the output container
     */

    void writeOut(TupleSetPtr input, Handle<Object>& writeToMe) override {

        // get the partitioned vector we are adding to
        Handle<Vector<Handle<Vector<Handle<Vector<ValueType>>>>>> writeMe =
            unsafeCast<Vector<Handle<Vector<Handle<Vector<ValueType>>>>>>(writeToMe);
        size_t hashVal;


        // get the input columns
        std::vector<ValueType>& valueColumn = input->getColumn<ValueType>(whichAttToHash);

        // and allocate everyone to a partition
        size_t length = valueColumn.size();
        for (size_t i = 0; i < length; i++) {

            hashVal = Hasher<ValueType>::hash(valueColumn[i]);
            int nodeId = (hashVal % (numPartitions/numNodes))/(numPartitions/numNodes);
            int partitionId = (hashVal % (numPartitions/numNodes)) % (numPartitions/numNodes);
            Vector<ValueType>& myVec = *((*((*writeMe)[nodeId]))[partitionId]);

            try {
                //to add the value to the partition
                myVec.push_back(valueColumn[i]);

            } catch (NotEnoughSpace & n) {

                /* if we got here then we run out of space and we need delete the already-processed
                 *  data, throw an exception so that new space can be allocated by handling the exception,
                 *  and try to process the remaining unprocessed data again */
                valueColumn.erase(valueColumn.begin(), valueColumn.begin() + i);
                throw n;

            }
        }
    }

    ~HashPartitionTransformationSink() {}


private:
    // the attributes to operate on
    int whichAttToHash;

    // number of partitions in the cluster
    int numPartitions;

    // number of nodes in the cluster
    int numNodes;

};
}

#endif
