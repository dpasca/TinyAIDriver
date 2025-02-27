#!/bin/bash

BASHSCRIPTDIR=$(cd "$(dirname "$0")" || exit; pwd)
EXTDIR="${BASHSCRIPTDIR}/_externals"
#GLMVER="0.9.9.8"
GLMVER="efec5db081e3aad807d0731e172ac597f6a39447"
SDLVER="release-2.30.x"
IMGUIVER="5b7feebfd8f3e58392d955b4c8ae01f77eae25ed"
FMTVER="7f46cb75b8c0b1e69edeaf728cddbd0cb72736ba"

checkout_revision(){
    REMOTE_REPO=$1
    REPO=$2
    REVISION=$3
    if [[ -d "${REPO}" ]] ; then
        cd "${REPO}" || exit
        git checkout -
        git pull
    else
        git clone "${REMOTE_REPO}"
        cd "${REPO}" || exit
    fi
    git checkout "${REVISION}"
    cd .. || exit
}

mkdir -p "${EXTDIR}" || exit 1
pushd "${EXTDIR}" || exit 1
checkout_revision "https://github.com/g-truc/glm"     "glm"   "${GLMVER}"
checkout_revision "https://github.com/libsdl-org/SDL" "SDL"   "${SDLVER}"
checkout_revision "https://github.com/ocornut/imgui"  "imgui" "${IMGUIVER}"
checkout_revision "https://github.com/fmtlib/fmt.git" "fmt"   "${FMTVER}"
popd

