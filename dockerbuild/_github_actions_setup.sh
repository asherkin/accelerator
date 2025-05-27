#!/bin/bash

set -euxo pipefail

pwd
ls -la

mkdir /accelerator
cp -av "$(pwd)"/. /accelerator
cd /accelerator

pwd
ls -la


bash ./dockerbuild/_accelerator_docker_build_internal.sh
