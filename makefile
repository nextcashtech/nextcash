
COMPILER=g++
COMPILE_FLAGS=-I./include -std=c++11 -Wall -pthread
LINK_FLAGS=-pthread
HEADER_FILES=$(wildcard src/*/*.hpp)
SOURCE_FILES=$(wildcard src/*/*.cpp)
SRC_SUB_DIRECTORIES=$(wildcard src/*)
OBJECT_DIRECTORY=.objects
HEADER_DIRECTORY=include/arcmist
OBJECTS=$(patsubst %.cpp,${OBJECT_DIRECTORY}/%.o,${SOURCE_FILES})
DEBUG_OBJECTS=$(patsubst %.cpp,${OBJECT_DIRECTORY}/%.o.debug,${SOURCE_FILES})
OUTPUT=arcmist

.PHONY: list clean test release debug

list:
	@echo Subdirectories : $(SRC_SUB_DIRECTORIES)
	@echo Headers : $(HEADER_FILES)
	@echo Sources : $(SOURCE_FILES)
	@echo Run Options :
	@echo "  make test    # Run unit tests"
	@echo "  make debug   # Build exe with gdb info"
	@echo "  make release # Build release exe"
	@echo "  make clean   # Remove all generated files"

headers:
	@mkdir -vp ${HEADER_DIRECTORY}
	@for DIR_NAME in ${SRC_SUB_DIRECTORIES}; do mkdir -vp ${HEADER_DIRECTORY}/$${DIR_NAME#src/}; done
	@for DIR_NAME in ${SRC_SUB_DIRECTORIES}; do cp -v $${DIR_NAME}/*.hpp ${HEADER_DIRECTORY}/$${DIR_NAME#src/}/; done

${OBJECT_DIRECTORY}:
	@echo ----------------------------------------------------------------------------------------------------
	@echo "\tCREATING OBJECT DIRECTORY"
	@echo ----------------------------------------------------------------------------------------------------
	@mkdir -v ${OBJECT_DIRECTORY}
	@for DIR_NAME in ${SRC_SUB_DIRECTORIES}; do mkdir -vp ${OBJECT_DIRECTORY}/$${DIR_NAME}; done

${OBJECT_DIRECTORY}/.headers: $(HEADER_FILES) | ${OBJECT_DIRECTORY}
	@echo ----------------------------------------------------------------------------------------------------
	@echo "\tHEADER(S) UPDATED $?"
	@echo ----------------------------------------------------------------------------------------------------
	@rm -vf ${OBJECT_DIRECTORY}/*.o
	@rm -vf ${OBJECT_DIRECTORY}/*/*.o
	@rm -vf ${OBJECT_DIRECTORY}/*/*/*.o
	@touch ${OBJECT_DIRECTORY}/.headers

${OBJECT_DIRECTORY}/.debug_headers: $(HEADER_FILES) | ${OBJECT_DIRECTORY}
	@echo ----------------------------------------------------------------------------------------------------
	@echo "\tHEADER(S) UPDATED $?"
	@echo ----------------------------------------------------------------------------------------------------
	@rm -vf ${OBJECT_DIRECTORY}/*.o.debug
	@rm -vf ${OBJECT_DIRECTORY}/*/*.o.debug
	@rm -vf ${OBJECT_DIRECTORY}/*/*/*.o.debug
	@touch ${OBJECT_DIRECTORY}/.debug_headers

${OBJECT_DIRECTORY}/%.o: %.cpp | ${OBJECT_DIRECTORY}
	@echo "\033[0;32m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;32mCOMPILING RELEASE $<\033[0m"
	@echo "\033[0;32m----------------------------------------------------------------------------------------------------\033[0m"
	${COMPILER} -c -o $@ $< ${COMPILE_FLAGS}

${OBJECT_DIRECTORY}/%.o.debug: %.cpp | ${OBJECT_DIRECTORY}
	@echo "\033[0;32m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;32mCOMPILING DEBUG $<\033[0m"
	@echo "\033[0;32m----------------------------------------------------------------------------------------------------\033[0m"
	${COMPILER} -c -ggdb -o $@ $< ${COMPILE_FLAGS}

release: headers ${OBJECT_DIRECTORY}/.headers ${OBJECTS}
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;33mBUILDING RELEASE ${OUTPUT}\033[0m"
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	ar -rvcs lib${OUTPUT}.a ${OBJECTS}
	@echo "\033[0;34m----------------------------------------------------------------------------------------------------\033[0m"

debug: headers ${OBJECT_DIRECTORY}/.debug_headers ${DEBUG_OBJECTS}
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;33mBUILDING DEBUG ${OUTPUT}\033[0m"
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	ar -rvcs lib${OUTPUT}.debug.a ${DEBUG_OBJECTS}
	@echo "\033[0;34m----------------------------------------------------------------------------------------------------\033[0m"

test: headers ${OBJECT_DIRECTORY}/.headers ${OBJECTS}
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;33mBUILDING TEST\033[0m"
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	${COMPILER} -c -o ${OBJECT_DIRECTORY}/test.o test.cpp ${COMPILE_FLAGS}
	${COMPILER} ${OBJECTS} ${OBJECT_DIRECTORY}/test.o ${LIBRARY_PATHS} ${LIBRARIES} -o test ${LINK_FLAGS}
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;33mTESTING\033[0m"
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@./test || echo "\n                                  \033[0;31m!!!!!  Tests Failed  !!!!!\033[0m"
	@echo "\033[0;34m----------------------------------------------------------------------------------------------------\033[0m"

test.debug: headers ${OBJECT_DIRECTORY}/.headers ${DEBUG_OBJECTS}
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;33mBUILDING DEBUG TEST\033[0m"
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	${COMPILER} -c -ggdb -o ${OBJECT_DIRECTORY}/test.o.debug test.cpp ${COMPILE_FLAGS}
	${COMPILER} ${DEBUG_OBJECTS} ${OBJECT_DIRECTORY}/test.o.debug ${LIBRARY_PATHS} ${DEBUG_LIBRARIES} -o test.debug ${LINK_FLAGS}
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@echo "\t\033[0;33mTESTING\033[0m"
	@echo "\033[0;33m----------------------------------------------------------------------------------------------------\033[0m"
	@./test.debug || echo "\n                                  \033[0;31m!!!!!  Tests Failed  !!!!!\033[0m"
	@echo "\033[0;34m----------------------------------------------------------------------------------------------------\033[0m"

clean:
	@echo ----------------------------------------------------------------------------------------------------
	@echo "\tCLEANING"
	@echo ----------------------------------------------------------------------------------------------------
	@rm -vfr include ${OBJECT_DIRECTORY} test test.debug ${OUTPUT} ${OUTPUT}.debug *.a
