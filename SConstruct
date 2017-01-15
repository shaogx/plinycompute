# SConstuct
# for PDB
# created on: 12/04/2015

import os
import re
import platform
import multiprocessing

common_env = Environment(CXX = 'clang++')
common_env.Append(YACCFLAGS='-d')
common_env.Append(CFLAGS='-std=c11')

# the following variables are used for output coloring to see errors and warnings better.
common_env = Environment(ENV = {'PATH' : os.environ['PATH'],
                         'TERM' : os.environ['TERM'],
                         'HOME' : os.environ['HOME']})
SRC_ROOT = os.path.join(Dir('.').abspath, "src") # assume the root source dir is in the working dir named as "src"

# OSX settings
if common_env['PLATFORM'] == 'darwin':
    print 'Compiling on OSX'
    common_env.Append(CXXFLAGS = '-std=c++1y -Wall -O0 -g')
    common_env.Replace(CXX = "clang++")

# Linux settings
elif  common_env['PLATFORM'] == 'posix':
    print 'Compiling on Linux'
    common_env.Append(LIBS = ['libdl.so', 'uuid'])
    common_env.Append(CXXFLAGS = '-std=c++14 -g  -O0 -ldl')
    common_env.Append(LINKFLAGS = '-pthread')
    common_env.Replace(CXX = "clang++")

common_env.Append(CCFLAGS='-DINITIALIZE_ALLOCATOR_BLOCK')
#common_env.Append(CCFLAGS='-DPDB_DEBUG')
# Make the build multithreaded
num_cpu = int(multiprocessing.cpu_count())
SetOption('num_jobs', num_cpu)

# two code files that will be included by the VTableMap to pre-load all of the
# built-in object types into the map
objectTargetDir = os.path.join(SRC_ROOT, 'objectModel', 'headers')
def extractCode (common_env, targets, sources):
    #
    print objectTargetDir + 'BuiltinPDBObjects.h'
    #
    # this is the file where the produced list of includes goes
    includeFile = open(os.path.join(objectTargetDir, 'BuiltinPDBObjects.h'), 'w+')
    includeFile.write ("// Auto-generated by code in SConstruct\n")
    #
    # this is the file where the produced code goes
    codeFile = open(os.path.join(objectTargetDir,'BuiltinPDBObjects.cc'), 'w+')
    codeFile.write ("// Auto-generated by code in SConstruct\n\n")
    codeFile.write ("// first, record all of the type codes\n")
    #
    # this is the file where all of the built-in type codes goes
    typeCodesFile = open(os.path.join(objectTargetDir, 'BuiltInObjectTypeIDs.h'), 'w+')
    typeCodesFile.write ("// Auto-generated by code in SConstruct\n")
    typeCodesFile.write ('#define NoMsg_TYPEID 0\n')
    #
    # loop through all of the files in the directory
    counter = 1;
    allClassNames = []
    for fileName in sources:
        datafile = file(fileName)
        #
        # seaeach foe a line like:
        # // PRELOAD %ObjectTwo%
        p = re.compile('//\s*PRELOAD\s*%[\w\s\<\>]*%')
        for line in datafile:
            #
            # if we found the line
            if p.search(line):
                #
                # add this .h file to the list of files to include
                includeFile.write ('#include "' + fileName + '"\n')
                #
                # extract the text between the two '%' symbols
                m = p.search(line)
                instance = m.group ()
                p = re.compile('%[\w\s\<\>]*%')
                m = p.search(instance)
                classname = (m.group ())[1:-1]
                #
                codeFile.write ('objectTypeNamesList [getTypeName <' + classname + '> ()] = ' + str(counter) + ';\n')
                allClassNames.append (classname)
                #
                # and here we write out the built-in type codes
                pattern = re.compile('\<[\w\s\<\>]*\>')
                if pattern.search (classname):
                    templateArg = pattern.search (classname)
                    classname = classname.replace (templateArg.group (), "").strip ()
                    #
                typeCodesFile.write('#define ' + classname + '_TYPEID ' + str(counter) + '\n')
                counter = counter + 1
                #
    counter = 1
    codeFile.write ('\n// now, record all of the vTables\n')
    for classname in allClassNames:
        #
        codeFile.write ('{\n\tconst UseTemporaryAllocationBlock tempBlock{1024 * 24};');
        codeFile.write ('\n\ttry {\n\t\t')
        codeFile.write (classname + ' tempObject;\n')
        codeFile.write ('\t\tallVTables [' + str(counter) + '] = tempObject.getVTablePtr ();\n')
        codeFile.write ('\t} catch (NotEnoughSpace &e) {\n\t\t')
        codeFile.write ('std :: cout << "Not enough memory to allocate ' + classname + ' to extract the vTable.\\n";\n\t}\n}\n\n');
        counter = counter + 1
    #

# here we get a list of all of the .h files in the 'headers' directory
from os import listdir
from os.path import isfile, isdir, join, abspath
objectheaders = os.path.join(SRC_ROOT, 'builtInPDBObjects', 'headers')
onlyfiles = [abspath(join(objectheaders, f)) for f in listdir(objectheaders) if isfile(join(objectheaders, f)) and f[-2:] == '.h']

# tell scons that the two files 'BuiltinPDBObjects.h' and 'BuiltinPDBObjects.cc' depend on everything in
# the 'headers' directory
common_env.Depends (objectTargetDir + 'BuiltinPDBObjects.h', onlyfiles)
common_env.Depends (objectTargetDir + 'BuiltinPDBObjects.cc', onlyfiles)
common_env.Depends (objectTargetDir + 'BuiltInObjectTypeIDs.h', onlyfiles)

# tell scons that the way to build 'BuiltinPDBObjects.h' and 'BuiltinPDBObjects.cc' is to run extractCode
builtInObjectBuilder = Builder (action = extractCode)
common_env.Append (BUILDERS = {'ExtactCode' : extractCode})
common_env.ExtactCode ([objectTargetDir + 'BuiltinPDBObjects.h', objectTargetDir + 'BuiltinPDBObjects.cc', objectTargetDir + 'BuiltInObjectTypeIDs.h'], onlyfiles)

# Construct a dictionary where each key is the directory basename of a PDB system component folder and each value
# is a list of .cc files used to implement that component.
#
# Expects the path structure of .cc files to be: SRC_ROOT / [component name] / source / [ComponentFile].cc
#
# For example, the structure:
#
#   src/                 <--- assume SRC_ROOT is here
#       compA/
#           headers/
#           source/
#               file1.cc
#               file2.cc
#               readme.txt
#       compB/
#           headers/
#           source/
#               file3.cc
#               file4.cc
#
#
# would result in component_dir_basename_to_cc_file_paths being populated as:
#
#   {'compA':[SRC_ROOT + "/compA/source/file1.cc", SRC_ROOT + "/compA/source/file2.cc"],
#    'compB':[SRC_ROOT + "/compB/source/file3.cc", SRC_ROOT + "/compB/source/file3.cc"]}
#
# on a Linux system.
component_dir_basename_to_lexer_c_file_paths = dict ()
component_dir_basename_to_cc_file_paths = dict ()
component_dir_basename_to_lexer_file_paths = dict ()
src_root_subdir_paths = [path for path in  map(lambda s: join(SRC_ROOT, s), listdir(SRC_ROOT)) if isdir(path)]
for src_subdir_path in src_root_subdir_paths:

    source_folder = join(src_subdir_path, 'source/')
    if(not isdir(source_folder)): # if no source folder lives under the subdir_path, skip this folder
        continue

    src_subdir_basename = os.path.basename(src_subdir_path)

    # first, map build output folders (on the left) to source folders (on the right)
    if src_subdir_basename == 'logicalPlan':
        # maps .y and .l source files used by flex and bison
        lexerSources = [abspath(join(join (source_folder),f2)) for f2 in listdir(source_folder) if isfile(join(source_folder, f2)) and (f2[-2:] == '.y' or f2[-2:] == '.l')]
        component_dir_basename_to_lexer_file_paths [src_subdir_basename] = lexerSources
        
        # maps .cc source files
        common_env.VariantDir(join('build/', src_subdir_basename), [source_folder], duplicate = 0)        
        ccSources = [abspath(join(join ('build/', src_subdir_basename),f2)) for f2 in listdir(source_folder) if isfile(join(source_folder, f2)) and (f2[-3:] == '.cc')]        

        component_dir_basename_to_cc_file_paths [src_subdir_basename] = ccSources

        # maps .c files        
        cSources = [(abspath(join(join ('build/', src_subdir_basename),'Parser.c'))), (abspath(join(join ('build/', src_subdir_basename),'Lexer.c')))]
        
        #component_dir_basename_to_lexer_c_file_paths [src_subdir_basename] = cSources
            
    else:
        common_env.VariantDir(join('build/', src_subdir_basename), [source_folder], duplicate = 0)

        # next, add all of the sources in
        allSources = [abspath(join(join ('build/', src_subdir_basename),f2)) for f2 in listdir(source_folder) if isfile(join(source_folder, f2)) and (f2[-3:] == '.cc' or f2[-2:] == '.y' or f2[-2:] == '.l')]
        component_dir_basename_to_cc_file_paths [src_subdir_basename] = allSources

# second, map build output folders (on the left) to source folders (on the right) for .so libraries
common_env.VariantDir('build/libraries/', 'src/sharedLibraries/source/', duplicate = 0)

# List of folders with headers
headerpaths = [abspath(join(join(SRC_ROOT, f), 'headers/')) for f in listdir(SRC_ROOT) if os.path.isdir (join(join(SRC_ROOT, f), 'headers/'))]

#boost has its own folder structure, which is difficult to be converted to our headers/source structure --Jia
# set BOOST_ROOT and BOOST_SRC_ROOT
BOOST_ROOT = os.path.join(Dir('.').abspath, "src/boost")
BOOST_SRC_ROOT = os.path.join(Dir('.').abspath, "src/boost/libs")
# map all boost source files to a list
boost_component_dir_basename_to_cc_file_paths = dict ()
boost_src_root_subdir_paths = [path for path in  map(lambda s: join(BOOST_SRC_ROOT, s), listdir(BOOST_SRC_ROOT)) if isdir(path)]
for boost_src_subdir_path in boost_src_root_subdir_paths:
        boost_source_folder = join(boost_src_subdir_path, 'src/')
        if(not isdir(boost_source_folder)): # if no source folder lives under the subdir_path, skip this folder
                continue

        boost_src_subdir_basename = os.path.basename(boost_src_subdir_path)

        # first, map build output folders (on the left) to source folders (on the right)
        common_env.VariantDir(join('build/', boost_src_subdir_basename), [boost_source_folder], duplicate = 0)

        # next, add all of the sources in
        allBoostSources = [abspath(join(join ('build/', boost_src_subdir_basename),f2)) for f2 in listdir(boost_source_folder) if isfile(join(boost_source_folder, f2)) and f2[-4:] == '.cpp']
        boost_component_dir_basename_to_cc_file_paths [boost_src_subdir_basename] = allBoostSources



# append boost to headerpaths
headerpaths.append(BOOST_ROOT)



# Adds header folders and required libraries
common_env.Append(CPPPATH = headerpaths)

print 'Platform: ' + platform.platform()
print 'System: ' + platform.system()
print 'Release: ' + platform.release()
print 'Version: ' + platform.version()

all = ['build/sqlite/sqlite3.c',
       component_dir_basename_to_cc_file_paths['serverFunctionalities'],
       component_dir_basename_to_cc_file_paths['bufferMgr'],
       component_dir_basename_to_cc_file_paths['communication'],
       component_dir_basename_to_cc_file_paths['catalog'],
       component_dir_basename_to_cc_file_paths['dispatcher'],
       component_dir_basename_to_cc_file_paths['pdbServer'],
       component_dir_basename_to_cc_file_paths['objectModel'],
       component_dir_basename_to_cc_file_paths['queryExecution'],
       component_dir_basename_to_cc_file_paths['queryIntermediaryRep'], 
       component_dir_basename_to_cc_file_paths['work'],
       component_dir_basename_to_cc_file_paths['memory'],
       component_dir_basename_to_cc_file_paths['storage'],
       component_dir_basename_to_cc_file_paths['distributionManager'],
       component_dir_basename_to_cc_file_paths['tcapLexer'],
       component_dir_basename_to_cc_file_paths['tcapParser'],
       component_dir_basename_to_cc_file_paths['tcapIntermediaryRep'],
       boost_component_dir_basename_to_cc_file_paths['filesystem'],
       boost_component_dir_basename_to_cc_file_paths['program_options'],
       boost_component_dir_basename_to_cc_file_paths['smart_ptr'],
       boost_component_dir_basename_to_cc_file_paths['system'],
      ]

common_env.SharedLibrary('libraries/libSharedEmployee.so', ['build/libraries/SharedEmployee.cc'] + all)
common_env.SharedLibrary('libraries/libChrisSelection.so', ['build/libraries/ChrisSelection.cc'] + all)
common_env.SharedLibrary('libraries/libStringSelection.so', ['build/libraries/StringSelection.cc'] + all)
common_env.SharedLibrary('libraries/libLeoQuery.so', ['build/libraries/LeoQuery.cc'] + all)
common_env.SharedLibrary('libraries/libKMeansQuery.so', ['build/libraries/KMeansQuery.cc'] + all)
common_env.SharedLibrary('libraries/libPartialResult.so', ['build/libraries/PartialResult.cc'] + all)

common_env.Program('bin/CatalogTests', ['build/tests/CatalogTests.cc'] + all)
common_env.Program('bin/CatalogServerTests', ['build/tests/CatalogServerTests.cc'] + all)
common_env.Program('bin/MasterServerTest', ['build/tests/MasterServerTest.cc'] + all)
common_env.Program('bin/test14', ['build/tests/Test14.cc'] + all)
common_env.Program('bin/test16', ['build/tests/Test16.cc'] + all)
common_env.Program('bin/test18', ['build/tests/Test18.cc'] + all)
common_env.Program('bin/test19', ['build/tests/Test19.cc'] + all)
common_env.Program('bin/test20', ['build/tests/Test20.cc'] + all)
common_env.Program('bin/test21', ['build/tests/Test21.cc'] + all)
common_env.Program('bin/test22', ['build/tests/Test22.cc'] + all)
common_env.Program('bin/test23', ['build/tests/Test23.cc'] + all)
common_env.Program('bin/test24', ['build/tests/Test24.cc'] + all)
common_env.Program('bin/test24-temp', ['build/tests/Test24-temp.cc'] + all)
common_env.Program('bin/test25', ['build/tests/Test25.cc'] + all)
common_env.Program('bin/test26', ['build/tests/Test26.cc'] + all)
common_env.Program('bin/test27', ['build/tests/Test27.cc'] + all)
common_env.Program('bin/test28', ['build/tests/Test28.cc'] + all)
common_env.Program('bin/test29', ['build/tests/Test29.cc'] + all)
common_env.Program('bin/test30', ['build/tests/Test30.cc'] + all)
common_env.Program('bin/test31', ['build/tests/Test31.cc'] + all)
common_env.Program('bin/test32', ['build/tests/Test32.cc'] + all)
common_env.Program('bin/test33', ['build/tests/Test33.cc'] + all)
common_env.Program('bin/test34', ['build/tests/Test34.cc'] + all)
common_env.Program('bin/test35', ['build/tests/Test35.cc'] + all)
common_env.Program('bin/test36', ['build/tests/Test36.cc'] + all)
common_env.Program('bin/test37', ['build/tests/Test37.cc'] + all)
common_env.Program('bin/test38', ['build/tests/Test38.cc'] + all)
common_env.Program('bin/test39', ['build/tests/Test39.cc'] + all)
common_env.Program('bin/test40', ['build/tests/Test40.cc'] + all)
common_env.Program('bin/test42', ['build/tests/Test42.cc'] + all)
common_env.Program('bin/test43', ['build/tests/Test43.cc'] + all)
common_env.Program('bin/test44', ['build/tests/Test44.cc'] + all)
common_env.Program('bin/test45', ['build/tests/Test45.cc'] + all)
common_env.Program('bin/test46', ['build/tests/Test46.cc'] + all)
common_env.Program('bin/test47', ['build/tests/Test47.cc'] + all +  component_dir_basename_to_lexer_file_paths['logicalPlan'] + component_dir_basename_to_cc_file_paths['logicalPlan'] + component_dir_basename_to_cc_file_paths['lambdas'])
common_env.Program('bin/test48', ['build/tests/Test48.cc'] + all)
common_env.Program('bin/test49', ['build/tests/Test49.cc'] + all)
common_env.Program('bin/test50', ['build/tests/Test50.cc'] + all)
common_env.Program('bin/test51', ['build/tests/Test51.cc'] + all)
common_env.Program('bin/test52', ['build/tests/Test52.cc'] + all)
common_env.Program('bin/test53', ['build/tests/Test53.cc'] + all)
common_env.Program('bin/test54', ['build/tests/Test54.cc'] + all)
common_env.Program('bin/test56', ['build/tests/Test56.cc'] + all)
common_env.Program('bin/test57', ['build/tests/Test57.cc'] + all)
common_env.Program('bin/test58', ['build/tests/Test58.cc'] + all)
common_env.Program('bin/test1', ['build/tests/Test1.cc'] + all)
common_env.Program('bin/test2', ['build/tests/Test2.cc'] + all)
common_env.Program('bin/test3', ['build/tests/Test3.cc'] + all)
common_env.Program('bin/test4', ['build/tests/Test4.cc'] + all)
common_env.Program('bin/test5', ['build/tests/Test5.cc'] + all)
common_env.Program('bin/test6', ['build/tests/Test6.cc'] + all)
common_env.Program('bin/test7', ['build/tests/Test7.cc'] + all)
common_env.Program('bin/test8', ['build/tests/Test8.cc'] + all)
common_env.Program('bin/test9', ['build/tests/Test9.cc'] + all)
common_env.Program('bin/test10', ['build/tests/Test10.cc'] + all)
common_env.Program('bin/test11', ['build/tests/Test11.cc'] + all)
common_env.Program('bin/test12', ['build/tests/Test12.cc'] + all)
common_env.Program('bin/test13', ['build/tests/Test13.cc'] + all)
common_env.Program('bin/test100', ['build/tests/Test100.cc'] + all)
common_env.Program('bin/test400', ['build/tests/Test400.cc'] + all)
common_env.Program('bin/test401', ['build/tests/Test401.cc'] + all)
common_env.Program('bin/test402', ['build/tests/Test402.cc'] + all)
common_env.Program('bin/test403', ['build/tests/Test403.cc'] + all)
common_env.Program('bin/test404', ['build/tests/Test404.cc'] + all)
common_env.Program('bin/test405', ['build/tests/Test405.cc'] + all)
common_env.Program('bin/test600', ['build/tests/Test600.cc'] + all)
common_env.Program('bin/test601', ['build/tests/Test601.cc'] + all)
common_env.Program('bin/test602', ['build/tests/Test602.cc'] + all)
# The dummy folder added by leo to test pipeline stuff and avoid linker issues
common_env.Program('bin/test603', ['build/tests/Test603.cc'] + all + component_dir_basename_to_cc_file_paths['dummyFolder'] +  component_dir_basename_to_lexer_file_paths['logicalPlan'] + component_dir_basename_to_cc_file_paths['logicalPlan'] + component_dir_basename_to_cc_file_paths['lambdas'])
common_env.Program('bin/test604', ['build/tests/Test604.cc'] + all  +  component_dir_basename_to_lexer_file_paths['logicalPlan'] + component_dir_basename_to_cc_file_paths['logicalPlan'] + component_dir_basename_to_cc_file_paths['lambdas'])
common_env.Program('bin/test605', ['build/tests/Test605.cc'] + all)
common_env.Program('bin/test606', ['build/tests/Test606.cc'] + all)
common_env.Program('bin/pdbServer', ['build/mainServer/PDBMainServerInstance.cc'] + all)
common_env.Program('bin/getListNodesTest', ['build/tests/GetListNodesTest.cc'] + all)
common_env.Program('bin/objectModelTest1', ['build/tests/ObjectModelTest1.cc'] + all)
common_env.Program('bin/storeSharedEmployeeInDBTest', ['build/tests/StoreSharedEmployeeInDBTest.cc'] + all)
common_env.Program('bin/registerTypeAndCreateDatabaseTest', ['build/tests/RegisterTypeAndCreateDatabaseTest.cc'] + all)

#Testing
pdbTest=common_env.Command('test', 'scripts/integratedTests.py', 'python $SOURCE -o $TARGET')
#pdbTest=common_env.Command('test',['bin/test603', 'bin/test46', 'bin/test44','libraries/libStringSelection.so', 'libraries/libChrisSelection.so', 'libraries/libSharedEmployee.so'],'python scripts/integratedTests.py -o $TARGET')
common_env.Depends(pdbTest, ['bin/test603', 'bin/test46', 'bin/test44', 'libraries/libStringSelection.so', 'libraries/libChrisSelection.so', 'libraries/libSharedEmployee.so', 'libraries/libLeoQuery.so'])
common_env.Alias('tests', pdbTest)
main=common_env.Alias('main', ['libraries/libKMeansQuery.so',  'libraries/libPartialResult.so', 'bin/test58', 'bin/test57', 'bin/test56', 'bin/test54', 'bin/test53', 'bin/test47', 'bin/test100', 'bin/test400', 'bin/test401', 'bin/test402', 'bin/test403', 'bin/test405', 'bin/test600','bin/test601', 'bin/test602','bin/MasterServerTest', 'bin/CatalogServerTests','bin/test603', 'bin/test46', 'bin/test44', 'libraries/libStringSelection.so', 'libraries/libChrisSelection.so', 'libraries/libSharedEmployee.so', 'libraries/libLeoQuery.so', 'bin/test404', 'bin/test52', 'bin/test49', 'bin/test1', 'bin/test2', 'bin/test3', 'bin/test4', 'bin/test5', 'bin/test6', 'bin/test7', 'bin/test8', 'bin/test9', 'bin/test10', 'bin/test11', 'bin/test12', 'bin/test13', 'bin/test16', 'bin/test43', 'bin/test44', 'bin/pdbServer', 'bin/getListNodesTest', 'bin/objectModelTest1', 'bin/CatalogTests', 'bin/storeSharedEmployeeInDBTest', 'bin/registerTypeAndCreateDatabaseTest'])
Default(main)
