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
#ifndef TEST_83_H
#define TEST_83_H

//by Jia, May 2017

#include "Handle.h"
#include "Lambda.h"
#include "QueryClient.h"
#include "DistributedStorageManagerClient.h"
#include "DispatcherClient.h"
#include "LambdaCreationFunctions.h"
#include "UseTemporaryAllocationBlock.h"
#include "Pipeline.h"
#include "DoubleVectorAggregation.h"
#include "DoubleVector.h"
#include "DoubleVectorResult.h"
#include "VectorSink.h"
#include "ScanDoubleVectorSet.h"
#include "HashSink.h"
#include "MapTupleSetIterator.h"
#include "VectorTupleSetIterator.h"
#include "ComputePlan.h"
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <chrono>
#include <fcntl.h>


// to run the aggregate, the system first passes each through the hash operation...
// then the system 
using namespace pdb;

int main (int argc, char * argv[]) {


       bool printResult = true;
       bool clusterMode = false;
       std :: cout << "Usage: #printResult[Y/N] #clusterMode[Y/N] #dataSize[MB] #masterIp #addData[Y/N]" << std :: endl;
       if (argc > 1) {
           if (strcmp(argv[1],"N") == 0) {
               printResult = false;
               std :: cout << "You successfully disabled printing result." << std::endl;
           } else {
               printResult = true;
               std :: cout << "Will print result." << std :: endl;
           }

       } else {
           std :: cout << "Will print result. If you don't want to print result, you can add N as the first parameter to disable result printing." << std :: endl;
       }

       if (argc > 2) {
           if (strcmp(argv[2],"Y") == 0) {
               clusterMode = true;
               std :: cout << "You successfully set the test to run on cluster." << std :: endl;
           } else {
               clusterMode = false;
           }
       } else {
           std :: cout << "Will run on local node. If you want to run on cluster, you can add any character as the second parameter to run on the cluster configured by $PDB_HOME/conf/serverlist." << std :: endl;
       }

       int numOfMb = 1024; //by default we add 1024MB data
       if (argc > 3) {
           numOfMb = atoi(argv[3]);
       }
       std :: cout << "To add data with size: " << numOfMb << "MB" << std :: endl;

       std :: string masterIp = "localhost";
       if (argc > 4) {
           masterIp = argv[4];
       }
       std :: cout << "Master IP Address is " << masterIp << std :: endl;

       bool whetherToAddData = true;
       if (argc > 5) {
           if (strcmp(argv[5],"N") == 0) {
              whetherToAddData = false;
           }
       }

       PDBLoggerPtr clientLogger = make_shared<PDBLogger>("clientLog");

       DistributedStorageManagerClient temp (8108, masterIp, clientLogger);

       CatalogClient catalogClient (8108, masterIp, clientLogger);

       string errMsg;

       if (whetherToAddData == true) {


            // now, create a new database
            if (!temp.createDatabase ("test83_db", errMsg)) {
                cout << "Not able to create database: " + errMsg;
                exit (-1);
            } else {
                cout << "Created database.\n";
            }

            // now, create a new set in that database
            if (!temp.createSet<DoubleVector> ("test83_db", "test83_set", errMsg)) {
                cout << "Not able to create set: " + errMsg;
                exit (-1);
            } else {
                cout << "Created set.\n";
            }


            //Step 2. Add data
            DispatcherClient dispatcherClient = DispatcherClient(8108, masterIp, clientLogger);

            double total = 0;
            if (numOfMb > 0) {
                int numIterations = numOfMb/64;
                int remainder = numOfMb - 64*numIterations;
                if (remainder > 0) { numIterations = numIterations + 1; }
                for (int num = 0; num < numIterations; num++) {
                    int blockSize = 64;
                    if ((num == numIterations - 1) && (remainder > 0)){
                        blockSize = remainder;
                    }
                    makeObjectAllocatorBlock(blockSize * 1024 * 1024, true);
                    Handle<Vector<Handle<DoubleVector>>> storeMe =
                        makeObject<Vector<Handle<DoubleVector>>> ();
                    try {
                        for (int i = 0; true ; i++) {
                             
                            Handle <DoubleVector> myData = makeObject <DoubleVector> (10);
                            for (int j = 0; j < 10; j++) {
                                (*(myData->data))[j] = 1;
                            }

                            storeMe->push_back(myData);
                            total = total + 1;
                       }
                         
                    } catch (pdb :: NotEnoughSpace &n) {
                        for (int i = 0; i < storeMe->size(); i++) {
                            if (i % 10000 == 0) {
                                std :: cout << i << ": ";
                                 ((*storeMe)[i])->print();
                            }            
                        }
                        if (!dispatcherClient.sendData<DoubleVector>(std::pair<std::string, std::string>("test83_set", "test83_db"), storeMe, errMsg)) {
                            std :: cout << "Failed to send data to dispatcher server" << std :: endl;
                            return -1;
                        }
                   }
                   PDB_COUT << blockSize << "MB data sent to dispatcher server~~" << std :: endl;
                }

                std :: cout << "total=" << total << std :: endl;

                //to write back all buffered records        
                temp.flushData( errMsg );
          }
        }
        // now, create a new set in that database to store output data
        PDB_COUT << "to create a new set for storing output data" << std :: endl;
        if (!temp.createSet<DoubleVector> ("test83_db", "output_set1", errMsg)) {
                cout << "Not able to create set: " + errMsg;
                exit (-1);
        } else {
                cout << "Created set.\n";
        }

        QueryClient myClient (8108, "localhost", clientLogger, true);

	// this is the object allocation block where all of this stuff will reside
        const UseTemporaryAllocationBlock tempBlock {1024 * 1024 * 128};
        // register this query class
        catalogClient.registerType ("libraries/libDoubleVectorAggregation.so", errMsg);
	
	// create all of the computation objects
	Handle <Computation> myScanSet = makeObject <ScanDoubleVectorSet> ("test83_db", "test83_set");
	Handle <Computation> myAgg = makeObject <DoubleVectorAggregation> ("test83_db", "output_set1");
	myAgg->setInput(myScanSet);

        auto begin = std :: chrono :: high_resolution_clock :: now();

        if (!myClient.executeComputations(errMsg, myAgg)) {
            std :: cout << "Query failed. Message was: " << errMsg << "\n";
            return 1;
        }
        std :: cout << std :: endl;
       
        auto end = std::chrono::high_resolution_clock::now();
        //std::cout << "Time Duration: " <<
          //      std::chrono::duration_cast<std::chrono::nanoseconds>(end-begin).count() << " ns." << std::endl;

        std::cout << std::endl;
        if (printResult == true) {
            std :: cout << "to print result..." << std :: endl;
            SetIterator <DoubleVector> result = myClient.getSetIterator <DoubleVector> ("test83_db", "test83_set");

            std :: cout << "Query results: ";
            int count = 0;
            for (auto a : result)
            {
                     count ++;
                     if (count%10000==0) {
                         std :: cout << count << ":";
                         a->print();
                     }
            }
            std :: cout << "input count:" << count << "\n";
        }


        // print the resuts
        if (printResult == true) {
            std :: cout << "to print result..." << std :: endl;
            SetIterator <DoubleVectorResult> result = myClient.getSetIterator <DoubleVectorResult> ("test83_db", "output_set1");

            std :: cout << "Query results: ";
            int count = 0;
            for (auto a : result)
            {
                     count ++;
                     std :: cout << count << ":";
                     a->print();
            }
            std :: cout << "aggregation output count:" << count << "\n";
        }

        if (clusterMode == false) {
            // and delete the sets
            myClient.deleteSet ("test83_db", "output_set1");
        } else {
            if (!temp.removeSet ("test83_db", "output_set1", errMsg)) {
                cout << "Not able to remove set: " + errMsg;
                exit (-1);
            } else {
                cout << "Removed set.\n";
            }
        }
        int code = system ("scripts/cleanupSoFiles.sh");
        if (code < 0) {
            std :: cout << "Can't cleanup so files" << std :: endl;
        }
        std::cout << "Time Duration: " <<
                std::chrono::duration_cast<std::chrono::duration<float>>(end-begin).count() << " secs." << std::endl;
}


#endif