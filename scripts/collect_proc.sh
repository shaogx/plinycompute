#!/usr/bin/env bash
#  Copyright 2018 Rice University
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#  ========================================================================    

set -o errexit

usage() {
    cat <<EOM

    Description: This script collects CPU and memory information about the machines in a cluster
    of PlinyCompute.

    Usage: scripts/$(basename $0) param1 param2

           param1: <pem_file> (e.g. conf/pdb-key.pem)
EOM
   exit -1;
}

[ -z $1 ] && { usage; }

PEM_FILE=$1
USERNAME=ubuntu

#remember to set environment variable: PDB_HOME first
if [ -z ${PDB_HOME} ]; 
    then echo "[ERROR] Please set PDB_HOME first";
else 
    echo "Your current PDB_HOME is: '$PDB_HOME'";
fi

# remember to provide your pem file as parameter
# if no pem file is provided, uses conf/pdb-key.pem as default
if [ -z ${PEM_FILE} ];
    then PEM_FILE=$PDB_HOME/conf/pdb-key.pem;
    chmod -R 600 $PDB_HOME/conf/pdb-key.pem
else
    echo "Your pem file is: '$PEM_FILE'";
    chmod -R 600 $PEM_FILE
fi

# if pem_file doesn't exist, exit!
if [ ! -f ${PEM_FILE} ]; then
    echo "Pem file '$PEM_FILE' not found, make sure the path and file name are correct!"
    exit -1;
fi

if [ ! -d "$PDB_HOME/conf" ]
         then mkdir $PDB_HOME/conf
fi

if [ ! -f "$PDB_HOME/conf/serverlist" ]
   then cp serverlist $PDB_HOME/conf/serverlist
fi

#distribute the collection scripts
$PDB_HOME/scripts/proc/distribute.sh $PEM_FILE $USERNAME

#collect the information of the distributed cluster
$PDB_HOME/scripts/proc/collect.sh $PEM_FILE $USERNAME

#cleanup the collection scripts
$PDB_HOME/scripts/proc/cleanupLocal.sh $PEM_FILE $USERNAME
