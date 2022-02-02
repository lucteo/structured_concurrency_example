#!/bin/bash

realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
}
CURDIR=$(realpath $(dirname "$0"))
source ${CURDIR}/_test_common.sh

set -e

${CURDIR}/test-clang-concepts.sh
${CURDIR}/test-clang-format.sh

echo
echo -e "${LIGHTGREEN}All tests passed successfully.${CLEAR}"
echo
