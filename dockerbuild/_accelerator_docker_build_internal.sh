#!/bin/bash

set -euxo pipefail


bootstrapEnv()
{
        # for squashing git whining when people run with docker instead of rootless podman
        git config --global --add safe.directory /
        git config --global --add safe.directory "*"

        # for preventing any submodule tomfuckery
        git submodule update --init --recursive -f

        # whack build dir it's literally faster and easier to do this than to cache it
        rm -rfv ./build

        # clean up patch cruft in breakpad dir so we start from a clean slate no matter what
        pushd third_party/breakpad
                git reset --hard
                git clean -x -f -d
                git clean -X -f -d
        popd
}


bootstrapPkgs()
{
    # we really need to slim this shit down lol
    dpkg --add-architecture i386 &&     \
        apt-get update -y &&            \
        apt-get install -y              \
        --no-install-recommends         \
        git                             \
        clang                           \
        make                            \
        python3-httplib2 python3-pip    \
        lib32stdc++-10-dev lib32z1-dev libc6-dev-i386 linux-libc-dev:i386 \
        libzstd-dev libzstd-dev:i386 zlib1g-dev zlib1g-dev:i386

	# force clang to be our compiler no matter what, hopefully
        update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100
        update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100

	# this is just for logging
        cc --version || true
        c++ --version || true
}


smBranch="master"
amTempLocation="_am_temp"
succCloneLocation="/accelerator/${amTempLocation}/successful_clone"
bootstrapAM()
{
    # need to install ambuild if we already cloned, otherwise checkout-deps will do it 4 us
    if test -f "${succCloneLocation}"; then
        pip install /accelerator/"${amTempLocation}"/ambuild
        return 255;
    fi

    rm -rf /accelerator/"${amTempLocation}"/    || true
    mkdir -p ${amTempLocation}                  || exit 1
    pushd ${amTempLocation}                     || exit 1
        git clone -b ${smBranch} --recursive --depth 1 https://github.com/alliedmodders/sourcemod sourcemod             || exit 1
        git clone                --recursive --depth 1 https://github.com/alliedmodders/ambuild ambuild                 || exit 1
        pip install ./ambuild

        # make a blank file so that we don't reclone everything if we don't need to
        true > "${succCloneLocation}" || exit 1
    popd
}


buildIt()
{
    if test ! -d build; then
        mkdir -p build
    fi

    pushd build
        CC=clang CXX=clang++ python3 ../configure.py --sm-path=/accelerator/${amTempLocation}/sourcemod/
        ambuild
    popd
}

###############################

cd /accelerator

bootstrapPkgs
bootstrapEnv
bootstrapAM || true

buildIt
