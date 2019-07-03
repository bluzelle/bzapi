#!/usr/bin/env bash

#
# Copyright (C) 2019 Bluzelle
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# set -x

TEMP_FILE=$(mktemp)

case "$1" in
    cpd)
        find "$3" -not \( -path "$4" -prune \) -name "*.cpp" -not -name "*_test.cpp" -o -name "*.hpp" > $TEMP_FILE
        $2 cpd --minimum-tokens 115 --language cpp --filelist $TEMP_FILE
    ;;
    pmccabe)
        echo "Modified McCabe Cyclomatic Complexity"
        echo "|   Traditional McCabe Cyclomatic Complexity"
        echo "|       |    # Statements in function"
        echo "|       |        |   First line of function"
        echo "|       |        |       |   # lines in function"
        echo "|       |        |       |       |  filename(definition line number):function"
        echo "|       |        |       |       |           |"
        pmccabe $(find "$2" -not \( -path "$3" -prune \) -name "*.cpp" -not -name "*_test.cpp" -o -name "*.hpp") 2> $TEMP_FILE | sort -nr
        echo "-----------------------------------------------------------------------------"
        echo Parse errors:
        echo
        cat $TEMP_FILE
    ;;
esac

rm $TEMP_FILE
