#!/bin/bash

set -euxo pipefail

bootstrapPkgs()
{
    dpkg --add-architecture i386 &&     \
        apt-get update -y &&            \
        apt-get install -y              \
        build-essential                 \
        git                             \
        curl                            \
        clang                           \
        python3-httplib2 python3-pip    \
        libc6-dev-i386-cross            \
        lib32stdc++-10-dev lib32z1-dev libc6-dev-i386 linux-libc-dev:i386 \
        --no-install-recommends
}


succCloneLocation="/accelerator/am_temp/successful_clone"
bootstrapAM()
{
    if test -f "${succCloneLocation}"; then
        pushd am_temp
        pip install ./ambuild
        popd
        return 255;
    fi

    rm -rf /accelerator/am_temp/    || true
    mkdir -p am_temp                || exit 1
    pushd am_temp                   || exit 1
        git clone --recursive https://github.com/alliedmodders/sourcemod || exit 1

        # skip downloading mysql we do not care about it
        bash sourcemod/tools/checkout-deps.sh -m    || exit 1

        # make a blank file so that we don't reclone everything if we don't need to
        true > "${succCloneLocation}" || exit 1
    popd
}


bootstrapBreakpad()
{
    bash breakpad.sh
}

buildIt()
{
    pushd build
        CC=clang CXX=clang++ python3 ../configure.py \
        --mms-path=/accelerator/am_temp/mmsource-1.12/ \
        --sm-path=/accelerator/am_temp/sourcemod/ \
        --hl2sdk-root=/accelerator/am_temp/ \
        -s tf2
        ambuild
    popd
}

###############################



cd /accelerator

bootstrapPkgs
if bootstrapAM; then
    bootstrapBreakpad
else
    bootstrapBreakpad
fi

buildIt
