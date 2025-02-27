#!/bin/bash

unameOut="$(uname -s)"
case "${unameOut}" in
    Linux*)     MACHINE=linux;;
    Darwin*)    MACHINE=macos;;
    CYGWIN*)    MACHINE=win;;
    MINGW*)     MACHINE=win;;
    *)          echo "unsupported architecture"; exit 1
esac

BASHSCRIPTDIR="$(cd "$(dirname "$0")" || exit; pwd)"
ROOTDIR=${BASHSCRIPTDIR}
BUILDDIR=${ROOTDIR}/_build/${MACHINE}
SOURCEDIR=${ROOTDIR}/TinyFreeway
NBFILES=${BUILDDIR}/.nbfiles

displayusage() {
    echo " ================================================================================= "
    echo "|    Usage:                                                                       |"
    echo "| build.sh OPTS                                                                   |"
    echo "|    available options [OPTS]:                                                    |"
    echo "| -b) --build)          automatically updates the build when necessary            |"
    echo "| -c) --clean)          removes build dirs                                        |"
    echo "| -d) --dry-run)        creates the make file without building                    |"
    echo "| -f) --force)          forces an update of the build                             |"
    echo "| -h) --help)           print this help                                           |"
    echo "| -m) --make)           performs make                                             |"
	echo "| -n) --nproc)          sets the number of parallel processing (default nproc -1) |"
    echo "| -t) --build-type)     specifies a different cmake build type (e.g. \"-t -Debug\") |"
    echo "| -w) --cmake-params)   specifies cmake options in quoted (e.g. \"-DVAR=value\")    |"
	echo "| [no arguments]        automatically updates the build when necessary            |"
    echo " ================================================================================= "
}

update_makefiles(){
    mkdir -p "${BUILDDIR}"
    cd "${BUILDDIR}" || exit

	if [ "${MACHINE}" == "macos" ]; then
        cmake "${ROOTDIR}" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" ${CMAKEOPTS:+$CMAKEOPTS}
    elif [ "${MACHINE}" == "linux" ]; then
        cmake "${ROOTDIR}" -DCMAKE_BUILD_TYPE="${BUILDTYPE:-Release}" ${CMAKEOPTS:+$CMAKEOPTS}
    elif [ "${MACHINE}" == "win" ]; then
        cmake -A x64 "${ROOTDIR}" ${CMAKEOPTS:+$CMAKEOPTS}
    fi
    CMAKE_RET=$?
	if [ $CMAKE_RET -ne 0 ] ; then exit ${CMAKE_RET}; fi
    cd - || exit
}

build(){
    echo "Check source tree with ${SOURCEDIR} (${SOURCEDIR}/src)..."
    if [ ! -d "${SOURCEDIR}/src" ]; then
        echo "Could not find source directory ${SOURCEDIR}/src"
        exit 1
    fi

    update_makefiles

    cd "${BUILDDIR}" || exit
    if [ "${MACHINE}" == "win" ]; then
        cmake --build . --config "${BUILDTYPE:-Release}" -- -maxcpucount:${NPARPROC}
    else
        make -j${NPARPROC}
    fi
    cd - || exit
}

emake(){
    cd "${BUILDDIR}" || exit
	if [ "${MACHINE}" == "win" ]; then
        cmake --build . --config "${BUILDTYPE:-Release}" -- -maxcpucount:${NPARPROC}
    else
        make -j${NPARPROC}
    fi
    cd - || exit
}

for arg in "$@"; do
	shift
	case "$arg" in
		"--build")          set -- "$@" "-b" ;;
		"--clean")          set -- "$@" "-c" ;;
		"--dry-run")        set -- "$@" "-d" ;;
		"--force")          set -- "$@" "-f" ;;
		"--help")           set -- "$@" "-h" ;;
		"--make")           set -- "$@" "-m" ;;
		"--nproc")          set -- "$@" "-n" ;;
		"--build-type")     set -- "$@" "-t" ;;
		"--cmake-params")   set -- "$@" "-w" ;;
		*)                  set -- "$@" "$arg";;
	esac
done

# Parse short options
OPTIND=1
while getopts "bcdfhilmn:pqrt:w:?" opt
do
	case "$opt" in
		"b") build; exit 0;;
		"c") CLEANBUILD="TRUE";;
		"d") UPDATEMAKEFILES="TRUE"; DRY_RUN="TRUE" ;;
		"f") UPDATEMAKEFILES="TRUE";;
		"h") displayusage; exit 0;;
		"m") emake; exit 0;;
		"n") NPARPROC=${OPTARG};;
		"t") BUILDTYPE=${OPTARG}; UPDATEMAKEFILES="TRUE";;
		"w") CMAKEOPTS+="${OPTARG} "; UPDATEMAKEFILES="TRUE";;
        "?") displayusage; exit 0;;
    esac
done

shift "$((OPTIND-1))"

if [[ -z "${NPARPROC}" ]]; then
	if [ "${MACHINE}" == "win" ]; then
		(( NPARPROC = 3 ))
	else
		(( NPARPROC = $(nproc) - 1 ))
	fi
fi


if [[ "${CLEANBUILD}" == "TRUE" ]] ; then rm -rf _bin _build; fi

if [[ "${UPDATEMAKEFILES}" == "TRUE" ]] ; then update_makefiles; fi

if [[ "${DRY_RUN}" == "TRUE" ]] ; then  exit 0; fi

build
