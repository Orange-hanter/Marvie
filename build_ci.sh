#!/usr/bin/env bash
#
# expects the following parameters to be passed in this specific order:
#  - project root folder, e.g. BITBUCKET_CLONE_DIR
set -e

repository_root_dir="$1"
artifacts_dir="${repository_root_dir}/artifacts"

# these settings can be changed safely
firmware_build_type="RelWithDebInfo"

function configure_cmake ()
{
  mkdir -p "$1"/build && cd "$1"/build

  cmake -GNinja \
  -DCMAKE_BUILD_TYPE="${firmware_build_type}" \
  -DSTM32_CHIP=STM32F407VE \
  -DCMAKE_MODULE_PATH="${repository_root_dir}/Firmware/CMake/" \
  -DCMAKE_TOOLCHAIN_FILE="${repository_root_dir}/Firmware/CMake/gcc_stm32.cmake" \
  -DCHIBIOS_ROOT="${repository_root_dir}/Firmware/ChibiOS" \
  -DCHIBIOS_PROCESS_STACK_SIZE=0x800 \
  -DCHIBIOS_MAIN_STACK_SIZE=0x800 \
  "$2" \
  ..
}

function generate_build_info ()
{
  mkdir -p "$1"
  echo "Build type: ${firmware_build_type}" > "$1/BUILD_INFO"
  echo "" >> "$1/BUILD_INFO"
  echo "Built on: $(date)" >> "$1/BUILD_INFO"
  echo "Built with: $(/usr/bin/arm-none-eabi-gcc --version | head -1)" >> "$1/BUILD_INFO"
}

function calc_sha256 ()
{
  local filename="$1"
  (cd $(dirname "${filename}"); sha256sum $(basename "${filename}") > "${filename}.sha256")
}

function build_firmware ()
{
  local firmware_type="$1"
  local project_dir="${repository_root_dir}/Firmware"
  local project_build_dir="${project_dir}/build"

  # build
  configure_cmake "${project_dir}" "-DMODEM_PWRKEY_LEVEL=${firmware_type}"
  marvie_base_version=$(cmake -LA "${project_dir}" | grep MARVIE_FIRMWARE_VERSION_BASE | cut -d'=' -f2)
  cmake --build "${project_build_dir}" --config "${firmware_build_type}" --target marvie_firmware.elf.bin

  # set proper name of *.elf binary and get its sha256
  local new_filename_mask="${project_build_dir}/marvie.firmware.${marvie_base_version}${firmware_type}"
  mv "${project_build_dir}/marvie_firmware.elf" "${new_filename_mask}.elf"
  calc_sha256 "${new_filename_mask}.elf"

  # set proper name of *.bin binary and get its sha256
  mv "${project_build_dir}/marvie_firmware.elf.bin" "${new_filename_mask}.bin"
  calc_sha256 "${new_filename_mask}.bin"

  mkdir -p "${artifacts_dir}/firmware/"
  for file in "${project_build_dir}"/marvie.firmware*.{bin,elf,sha256}; do
    mv "${file}" "${artifacts_dir}/firmware/"
  done

  generate_build_info "${artifacts_dir}/firmware"

  export marvie_base_version
}

function build_bootloader ()
{
  local project_dir="${repository_root_dir}/Firmware/Bootloader"
  local project_build_dir="${project_dir}/build"

  # build
  configure_cmake "${project_dir}"
  cmake --build "${project_build_dir}" --config "${firmware_build_type}" --target bootloader.elf.bin

  # set proper name of *.bin binary and get its sha256
  local new_filename_mask="${project_build_dir}/marvie.bootloader"
  mv "${project_build_dir}/bootloader.elf.bin" "${new_filename_mask}.bin"
  calc_sha256 "${new_filename_mask}.bin"

  # set proper name of *.bin binary and get its sha256
  mv "${project_build_dir}/bootloader.elf" "${new_filename_mask}.elf"
  calc_sha256 "${new_filename_mask}.elf"

  mkdir -p "${artifacts_dir}/bootloader/"
  for file in "${project_build_dir}"/marvie.bootloader*.{bin,elf,sha256}; do
    mv "${file}" "${artifacts_dir}/bootloader/"
  done

  generate_build_info "${artifacts_dir}/bootloader"
}

build_firmware 'L'
build_firmware 'H'
build_bootloader

echo $marvie_base_version > "${repository_root_dir}"/version
