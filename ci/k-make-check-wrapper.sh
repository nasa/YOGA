#!/usr/bin/env bash

set -x # echo commands
set -e # exit on first error
set -u # Treat unset variables as error

build_directory_root=/hpnobackup2/fun3d/yoga-ci

build_machine=k4
ssh -o StrictHostKeyChecking=no fun3d@${build_machine} true

BUILD_TAG="${CI_JOB_NAME}-${CI_JOB_ID}"
set +u
if [[ -z "${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME}" || -z "{CI_MERGE_REQUEST_TARGET_BRANCH_NAME}" ]]; then
    checkout_cmd="git checkout ${CI_COMMIT_SHA}"
else
    checkout_cmd="git checkout ${CI_MERGE_REQUEST_SOURCE_BRANCH_NAME} && git merge ${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}"
fi
set -u

ssh -o LogLevel=error fun3d@${build_machine} <<EOF
whoami && \
uname -n && \
mkdir -p ${build_directory_root} && \
cd ${build_directory_root} && \
  mkdir -p ${BUILD_TAG} && \
  cd ${BUILD_TAG} && \
    pwd && \
    git clone ${CI_REPOSITORY_URL} && \
    cd yoga && \
      pwd && \
      ${checkout_cmd} && \
      ./update_submodules.sh && \
      ./ci/k-make-check-qsub.sh ${CHECK_QUEUE}
EOF

scp fun3d@${build_machine}:${build_directory_root}/${BUILD_TAG}/log-\* .

cat log-make-check.txt

trap "cat cleanup.log" EXIT
ssh -o LogLevel=error fun3d@${build_machine} > cleanup.log 2>&1 <<EOF
whoami && \
uname -n && \
cd ${build_directory_root}/${BUILD_TAG}/yoga && \
 ./ci/remove_old_builds.sh \
  "${build_directory_root}"
EOF
trap - EXIT

