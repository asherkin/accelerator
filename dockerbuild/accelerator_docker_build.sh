#!/bin/bash
set -euxo pipefail

dockerimage="debian:11-slim"


# we do this so that we can be agnostic about where we're invoked from
# meaning you can exec this script anywhere and it should work the same
thisiswhereiam=${BASH_SOURCE[0]}
# this should be /whatever/directory/structure/Open-Fortress-Source
script_folder=$( cd -- "$( dirname -- "${thisiswhereiam}" )" &> /dev/null && pwd )


# this should be /whatever/directory/structure/[accelerator_root]/cicd
build_dir="dockerbuild"

pushd "${script_folder}" &> /dev/null || exit 99

    # this is relative to our source dir/build
    internalscript="_accelerator_docker_build_internal.sh"

    # this should always be our accelerator root dir
    pushd ../ &> /dev/null
        dev_srcdir=$(pwd)
        container_rootdir="accelerator"

        # add -it flags automatically if in null tty
        itflag=""
        if [ -t 0 ] ; then
            itflag="-it"
        else
            itflag=""
        fi

        podman run ${itflag}                            \
        -v "${dev_srcdir}":/"${container_rootdir}"        \
        -w /${container_rootdir}                        \
        ${dockerimage}                                  \
        bash ./${build_dir}/${internalscript} "$@"

        ecodereal=$?
        echo "real exit code ${ecodereal}"

    popd &> /dev/null || exit

popd &> /dev/null || exit

exit ${ecodereal}

