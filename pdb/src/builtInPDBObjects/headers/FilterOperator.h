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
#ifndef FILTER_OPERATOR_H
#define FILTER_OPERATOR_H

// by Jia, Sept 2016

#include "ExecutionOperator.h"
#include "QueryBase.h"
#include "Selection.h"
#include "BlockQueryProcessor.h"

// PRELOAD %FilterOperator%

namespace pdb {

/*
 * this class encapsulates a FilterOperator that runs in backend.
 */

class FilterOperator : public ExecutionOperator {

public:
    ENABLE_DEEP_COPY

    FilterOperator() {
        selection = nullptr;
    }

    FilterOperator(Handle<QueryBase>& selection) {
        this->selection = selection;
    }

    std::string getName() override {
        return "FilterOperator";
    }

    BlockQueryProcessorPtr getProcessor() override {
        if (selection == nullptr) {
            return nullptr;
        }
        return (unsafeCast<Selection<Object, Object>>(selection))->getFilterBlockProcessor();
    }

    Handle<QueryBase>& getSelection() override {
        return this->selection;
    }

private:
    Handle<QueryBase> selection;
};
}

#endif