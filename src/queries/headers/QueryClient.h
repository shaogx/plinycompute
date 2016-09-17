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

#ifndef QUERY_CLIENT
#define QUERY_CLIENT

#include "Set.h"
#include "Handle.h"
#include "PDBLogger.h"
#include "PDBVector.h"
#include "CatalogClient.h"
#include "ExecuteQuery.h"
#include "LocalQueryOutput.h"

namespace pdb {

class QueryClient {

public:

	// connect to the database
	QueryClient (int portIn, std :: string addressIn, PDBLoggerPtr myLoggerIn) : myHelper (portIn, addressIn, myLoggerIn) {
		port = portIn;
		address = addressIn;
		myLogger = myLoggerIn;
		runUs = makeObject <Vector <Handle <QueryBase>>> ();
	}

	// access a set in the database
	template <class Type>
	Handle <Set <Type>> getSet (std :: string databaseName, std :: string setName) {
	
		// verify that the database and set work 
		std :: string errMsg;
		std :: string typeName = myHelper.getObjectType (databaseName, setName, errMsg);
		
		if (typeName == "") {
			myLogger->error ("query client: not able to verify type: " + errMsg);
			Handle <Set <Type>> returnVal = makeObject <Set <Type>> (false);
			return returnVal;
		}

		if (typeName != getTypeName <Type> ()) {
			std :: cout << "Wrong type for database set " << setName << "\n";
			Handle <Set <Type>> returnVal = makeObject <Set <Type>> (false);
			return returnVal;
		}

		Handle <Set <Type>> returnVal = makeObject <Set <Type>> (databaseName, setName);
		return returnVal;
	}

	template <class ...Types>
	bool execute (std :: string &errMsg, Handle <QueryBase> firstParam, Handle <Types>... args) {
		if (firstParam->wasError ()) {
			std :: cout << "There was an error constructing this query.  Can't run it.\n";
			exit (1);
		}

		runUs->push_back (firstParam);
		return execute (errMsg, args...);	
	}

	bool execute (std :: string &errMsg) {

		// this is the request
		const UseTemporaryAllocationBlock myBlock {1024};
		Handle <ExecuteQuery> executeQuery = makeObject <ExecuteQuery> ();

		// this call asks the database to execute the query, and then it inserts the result set name
		// within each of the results, as well as the database connection information
		return simpleDoubleRequest <ExecuteQuery, Vector <Handle <QueryBase>>, Vector <String>, bool> (myLogger, port, 
		address, false, 124 * 1024, 
                [&] (Handle <Vector <String>> result) {
                        if (result != nullptr) {

				// make sure we got the correct number of results
				if (result->size () != runUs->size ()) {
                                        errMsg = "Got a strange result size from execute";
                                        myLogger->error ("QueryErr: " + errMsg);
                                        return false;
                                }

				// now, annotate each of the results with information on the set where
				// they are stored, as well as the info on how to connect to the server
				for (int i = 0; i < result->size (); i++) {

					if ((*runUs)[i]->getQueryType () != "localoutput")
						std :: cout << "This is bad... there was an output that was not a local query output.\n";

					(*runUs)[i]->setSetName ((*result)[i]);
					Handle <LocalQueryOutput <Object>> temp = unsafeCast <LocalQueryOutput <Object>> ((*runUs)[i]);
					temp->setServer (port, address, myLogger);
				}
                                return true;
                        }
                        errMsg = "Error getting type name: got nothing back from catalog";
                        return false;}, executeQuery, runUs);
	}

private:

	// how we connect to the catalog
	CatalogClient myHelper;

	// this is the query graph we'll execute
	Handle <Vector <Handle <QueryBase>>> runUs;

	// connection info
	int port;
	std :: string address;

	// for logging
	PDBLoggerPtr myLogger;
};

}

#endif

