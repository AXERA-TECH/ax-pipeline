#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <chip>" >&2
  exit 1
fi

CHIP="$1"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOST_ARCH="$(uname -m)"

mkdir -p "${ROOT_DIR}/.ci/downloads" "${ROOT_DIR}/.ci/toolchains"

download_if_missing() {
  local url="$1"
  local out="$2"
  if [[ -f "${out}" ]]; then
    return 0
  fi
  echo "download: ${url} -> ${out}" >&2
  wget -q -O "${out}" "${url}"
}

case "${CHIP}" in
  ax650)
    MSP_ZIP_NAME="msp_50_3.10.2.zip"
    MSP_URL_DEFAULT="https://github.com/ZHEQIUSHUI/assets/releases/download/ax_3.6.2/msp_50_3.10.2.zip"
    MSP_EXTRACT_DIR="${ROOT_DIR}/.ci/msp/ax650"
    TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/aarch64-none-linux-gnu.toolchain.cmake"
    TOOLCHAIN_ARCHIVE_NAME="gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz"
    TOOLCHAIN_URL_DEFAULT="https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/${TOOLCHAIN_ARCHIVE_NAME}"
    TOOLCHAIN_DIR="${ROOT_DIR}/.ci/toolchains/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu"
    TOOLCHAIN_BIN="${TOOLCHAIN_DIR}/bin"
    COMPILER_CHECK="aarch64-none-linux-gnu-g++"
    ;;
  ax630c)
    MSP_ZIP_NAME="msp_20e_3.0.0.zip"
    MSP_URL_DEFAULT="https://github.com/ZHEQIUSHUI/assets/releases/download/ax_3.6.2/msp_20e_3.0.0.zip"
    MSP_EXTRACT_DIR="${ROOT_DIR}/.ci/msp/ax620e"
    TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/aarch64-none-linux-gnu.toolchain.cmake"
    TOOLCHAIN_ARCHIVE_NAME="gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz"
    TOOLCHAIN_URL_DEFAULT="https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/${TOOLCHAIN_ARCHIVE_NAME}"
    TOOLCHAIN_DIR="${ROOT_DIR}/.ci/toolchains/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu"
    TOOLCHAIN_BIN="${TOOLCHAIN_DIR}/bin"
    COMPILER_CHECK="aarch64-none-linux-gnu-g++"
    ;;
  ax620q)
    MSP_ZIP_NAME="msp_20e_3.0.0.zip"
    MSP_URL_DEFAULT="https://github.com/ZHEQIUSHUI/assets/releases/download/ax_3.6.2/msp_20e_3.0.0.zip"
    MSP_EXTRACT_DIR="${ROOT_DIR}/.ci/msp/ax620e"
    TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/arm-AX620E-linux-uclibcgnueabihf.toolchain.cmake"
    TOOLCHAIN_ARCHIVE_NAME="arm-AX620E-linux-uclibcgnueabihf_V3_20240320.tgz"
    TOOLCHAIN_URL_DEFAULT="https://github.com/AXERA-TECH/ax620q_bsp_sdk/releases/download/v2.0.0/${TOOLCHAIN_ARCHIVE_NAME}"
    TOOLCHAIN_DIR="${ROOT_DIR}/.ci/toolchains/arm-AX620E-linux-uclibcgnueabihf"
    TOOLCHAIN_BIN="${TOOLCHAIN_DIR}/bin"
    COMPILER_CHECK="arm-AX620E-linux-uclibcgnueabihf-g++"
    ;;
  ax620qp)
    MSP_ZIP_NAME="msp_20e_3.0.0.zip"
    MSP_URL_DEFAULT="https://github.com/ZHEQIUSHUI/assets/releases/download/ax_3.6.2/msp_20e_3.0.0.zip"
    MSP_EXTRACT_DIR="${ROOT_DIR}/.ci/msp/ax620e"
    TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/arm-linux-gnueabihf.toolchain.cmake"
    TOOLCHAIN_ARCHIVE_NAME="gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf.tar.xz"
    TOOLCHAIN_URL_DEFAULT="https://releases.linaro.org/components/toolchain/binaries/7.5-2019.12/arm-linux-gnueabihf/${TOOLCHAIN_ARCHIVE_NAME}"
    TOOLCHAIN_DIR="${ROOT_DIR}/.ci/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf"
    TOOLCHAIN_BIN="${TOOLCHAIN_DIR}/bin"
    COMPILER_CHECK="arm-linux-gnueabihf-g++"
    ;;
  axcl-x86_64)
    MSP_ZIP_NAME="axcl_linux_3.10.2.zip"
    MSP_URL_DEFAULT="https://github.com/ZHEQIUSHUI/assets/releases/download/ax_3.6.2/axcl_linux_3.10.2.zip"
    MSP_EXTRACT_DIR="${ROOT_DIR}/.ci/axcl/axcl_linux_3.10.2_x86_64"
    TOOLCHAIN_FILE=""
    TOOLCHAIN_BIN=""
    COMPILER_CHECK="g++"
    AXCL_SUBDIR_NAME="axcl_linux_x86"
    ;;
  axcl-aarch64)
    MSP_ZIP_NAME="axcl_linux_3.10.2.zip"
    MSP_URL_DEFAULT="https://github.com/ZHEQIUSHUI/assets/releases/download/ax_3.6.2/axcl_linux_3.10.2.zip"
    MSP_EXTRACT_DIR="${ROOT_DIR}/.ci/axcl/axcl_linux_3.10.2_aarch64"
    TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/aarch64-none-linux-gnu.toolchain.cmake"
    TOOLCHAIN_ARCHIVE_NAME="gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz"
    TOOLCHAIN_URL_DEFAULT="https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/${TOOLCHAIN_ARCHIVE_NAME}"
    TOOLCHAIN_DIR="${ROOT_DIR}/.ci/toolchains/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu"
    TOOLCHAIN_BIN="${TOOLCHAIN_DIR}/bin"
    COMPILER_CHECK="aarch64-none-linux-gnu-g++"
    AXCL_SUBDIR_NAME="axcl_linux_arm64"

    # Native build on an aarch64 host (e.g. Raspberry Pi):
    # do NOT download/use the x86_64 cross toolchain archive.
    if [[ "${HOST_ARCH}" == "aarch64" || "${HOST_ARCH}" == "arm64" ]]; then
      TOOLCHAIN_FILE=""
      TOOLCHAIN_BIN=""
      COMPILER_CHECK="g++"
    fi
    ;;
  *)
    echo "unsupported chip: ${CHIP}" >&2
    exit 1
    ;;
esac

MSP_ZIP_PATH="${AXSDK_MSP_ZIP_PATH:-${ROOT_DIR}/.ci/downloads/${MSP_ZIP_NAME}}"
MSP_URL="${AXSDK_MSP_URL:-${MSP_URL_DEFAULT}}"
BUILD_DIR="${AXSDK_BUILD_DIR:-${ROOT_DIR}/build_${CHIP}_ci}"
STAGE_DIR="${AXSDK_STAGE_DIR:-${ROOT_DIR}/artifacts/${CHIP}}"

if [[ -n "${TOOLCHAIN_BIN}" ]]; then
  TOOLCHAIN_ARCHIVE_PATH="${ROOT_DIR}/.ci/downloads/${TOOLCHAIN_ARCHIVE_NAME}"
  TOOLCHAIN_URL="${AXSDK_TOOLCHAIN_URL:-${TOOLCHAIN_URL_DEFAULT}}"
  if [[ ! -d "${TOOLCHAIN_BIN}" ]]; then
    download_if_missing "${TOOLCHAIN_URL}" "${TOOLCHAIN_ARCHIVE_PATH}"
    rm -rf "${TOOLCHAIN_DIR}"
    mkdir -p "${ROOT_DIR}/.ci/toolchains"
    case "${TOOLCHAIN_ARCHIVE_PATH}" in
      *.tar.xz) tar -C "${ROOT_DIR}/.ci/toolchains" -xf "${TOOLCHAIN_ARCHIVE_PATH}" ;;
      *.tgz) tar -C "${ROOT_DIR}/.ci/toolchains" -xzf "${TOOLCHAIN_ARCHIVE_PATH}" ;;
      *) echo "unknown toolchain archive: ${TOOLCHAIN_ARCHIVE_PATH}" >&2; exit 1 ;;
    esac
  fi

  export PATH="${TOOLCHAIN_BIN}:${PATH}"
fi

if ! command -v "${COMPILER_CHECK}" >/dev/null 2>&1; then
  echo "missing compiler ${COMPILER_CHECK} (PATH=${PATH})" >&2
  exit 1
fi
("${COMPILER_CHECK}" -v >/dev/null 2>&1) || {
  echo "compiler probe failed: ${COMPILER_CHECK} -v" >&2
  exit 1
}

MSP_ROOT=""
if [[ "${CHIP}" == axcl-* ]]; then
  if [[ -n "${AXSDK_AXCL_DIR:-}" ]]; then
    MSP_ROOT="${AXSDK_AXCL_DIR}"
  else
    # Prefer system-installed AXCL (fast, avoids CI zip quirks).
    if [[ -f "/usr/include/axcl/axcl.h" || -f "/usr/include/axcl.h" ]] && \
       [[ -f "/usr/lib/axcl/libaxcl_sys.so" || -f "/usr/lib/libaxcl_sys.so" || -f "/usr/lib64/libaxcl_sys.so" ]]; then
      MSP_ROOT="/usr"
    else
      download_if_missing "${MSP_URL}" "${MSP_ZIP_PATH}"
      rm -rf "${MSP_EXTRACT_DIR}"
      mkdir -p "${MSP_EXTRACT_DIR}"
      unzip -q "${MSP_ZIP_PATH}" -d "${MSP_EXTRACT_DIR}"
      DETECTED_AXCL_ROOT="$(find "${MSP_EXTRACT_DIR}" -maxdepth 5 \( -path "*/${AXCL_SUBDIR_NAME}/include/axcl.h" -o -path "*/${AXCL_SUBDIR_NAME}/include/axcl/axcl.h" \) | head -n 1 || true)"
      if [[ "${DETECTED_AXCL_ROOT}" == */include/axcl/axcl.h ]]; then
        MSP_ROOT="${DETECTED_AXCL_ROOT%/include/axcl/axcl.h}"
      elif [[ "${DETECTED_AXCL_ROOT}" == */include/axcl.h ]]; then
        MSP_ROOT="${DETECTED_AXCL_ROOT%/include/axcl.h}"
      fi
    fi
  fi

  if [[ -z "${MSP_ROOT}" || ! -d "${MSP_ROOT}" ]]; then
    echo "AXCL root not found for ${CHIP}; set AXSDK_AXCL_DIR or provide AXCL zip." >&2
    exit 1
  fi

  # Some AXCL zips ship 0-byte libspdlog placeholders. Fix them up so runtime linking works.
  fix_axcl_spdlog() {
    local dir="$1"
    [[ -d "${dir}" ]] || return 0

    local good=""
    if [[ -f "${dir}/libspdlog.so.1.14.1" ]] && [[ "$(stat -c%s "${dir}/libspdlog.so.1.14.1" 2>/dev/null || echo 0)" -gt 0 ]]; then
      good="libspdlog.so.1.14.1"
    else
      for cand in "${dir}"/libspdlog.so.*; do
        [[ -f "${cand}" ]] || continue
        if [[ "$(stat -c%s "${cand}" 2>/dev/null || echo 0)" -gt 0 ]]; then
          good="$(basename "${cand}")"
          break
        fi
      done
    fi

    [[ -n "${good}" ]] || return 0

    for bad in libspdlog.so libspdlog.so.1.14; do
      if [[ -e "${dir}/${bad}" ]] && [[ "$(stat -c%s "${dir}/${bad}" 2>/dev/null || echo 0)" -eq 0 ]]; then
        rm -f "${dir:?}/${bad}"
        ln -s "${good}" "${dir}/${bad}"
      fi
    done
  }

  # Only needed for AXCL zip packages; system-installed libs should be sane.
  if [[ "${MSP_ROOT}" != "/usr" ]]; then
    fix_axcl_spdlog "${MSP_ROOT}/lib"
    fix_axcl_spdlog "${MSP_ROOT}/lib/axcl"
    fix_axcl_spdlog "${MSP_ROOT}/lib64"
  fi
else
  download_if_missing "${MSP_URL}" "${MSP_ZIP_PATH}"
  rm -rf "${MSP_EXTRACT_DIR}"
  mkdir -p "${MSP_EXTRACT_DIR}"
  unzip -q "${MSP_ZIP_PATH}" -d "${MSP_EXTRACT_DIR}"
  DETECTED_MSP_ROOT="$(find "${MSP_EXTRACT_DIR}" -maxdepth 3 -type d -name msp | head -n 1 || true)"
  if [[ -z "${DETECTED_MSP_ROOT}" ]]; then
    echo "extracted MSP root not found under ${MSP_EXTRACT_DIR}" >&2
    exit 1
  fi
  MSP_ROOT="${DETECTED_MSP_ROOT}"
fi

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

CMAKE_ARGS=(
  -S "${ROOT_DIR}"
  -B "${BUILD_DIR}"
  -DAXSDK_BUILD_SHARED=ON
  -DAXSDK_BUILD_TOOLS=OFF
  -DAXSDK_BUILD_SMOKE_TESTS=OFF
)

if [[ "${CHIP}" == axcl-* ]]; then
  CMAKE_ARGS+=(
    -DAXSDK_CHIP_TYPE="axcl"
    -DAXSDK_AXCL_DIR="${MSP_ROOT}"
  )
  if [[ -n "${TOOLCHAIN_FILE}" ]]; then
    CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}")
  fi
else
  CMAKE_ARGS+=(
    -DAXSDK_CHIP_TYPE="${CHIP}"
    -DAXSDK_MSP_DIR="${MSP_ROOT}"
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}"
  )
fi

cmake "${CMAKE_ARGS[@]}"
cmake --build "${BUILD_DIR}" -j"$(getconf _NPROCESSORS_ONLN)" --target ax_pipeline_app ax_mp4_dump_annexb

mkdir -p "${STAGE_DIR}"
PACKAGE_BASENAME="ax_pipeline_${CHIP}"
PACKAGE_DIR="${STAGE_DIR}/${PACKAGE_BASENAME}"
rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}/bin" "${PACKAGE_DIR}/lib"

cp -a "${BUILD_DIR}/ax_pipeline_app" "${PACKAGE_DIR}/bin/"
cp -a "${BUILD_DIR}/ax_mp4_dump_annexb" "${PACKAGE_DIR}/bin/"

SDK_SO="$(find "${BUILD_DIR}" -maxdepth 6 -name 'libax_video_sdk.so' | head -n 1 || true)"
if [[ -z "${SDK_SO}" ]]; then
  echo "libax_video_sdk.so not found under ${BUILD_DIR}" >&2
  exit 1
fi
cp -a "${SDK_SO}" "${PACKAGE_DIR}/lib/"
cp -a "${ROOT_DIR}/configs" "${PACKAGE_DIR}/"

cat > "${PACKAGE_DIR}/BUILD_INFO.txt" <<EOF
chip=${CHIP}
build_dir=${BUILD_DIR}
msp_zip=${MSP_ZIP_PATH}
msp_root=${MSP_ROOT}
toolchain_file=${TOOLCHAIN_FILE}
compiler=$("${COMPILER_CHECK}" --version | head -n 1)
EOF

(
  cd "${STAGE_DIR}"
  tar -czf "${PACKAGE_BASENAME}.tar.gz" "${PACKAGE_BASENAME}"
)

echo "package=${STAGE_DIR}/${PACKAGE_BASENAME}.tar.gz"
