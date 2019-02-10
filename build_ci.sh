#!/usr/bin/env bash
#
# expects the following parameters to be passed in this specific order:
#  - project root folder, e.g. BITBUCKET_CLONE_DIR

repository_root_dir="$1"
artifacts_dir="${repository_root_dir}/artifacts"

# these settings can be changed safely
build_type="RelWithDebInfo"

function configure_cmake ()
{
  mkdir -p "$1"/build && cd "$1"/build

  cmake -GNinja \
  -DCMAKE_BUILD_TYPE="${build_type}" \
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
  echo "Built on: $(date)" > "$1"/BUILD_INFO
  echo "Built with: $(/usr/bin/arm-none-eabi-gcc --version | head -1)" >> "$1"/BUILD_INFO
}

function build_firmware ()
{
  project_dir="${repository_root_dir}/Firmware"
  project_build_dir="${project_dir}/build"

  # build H version of firmware
  configure_cmake "${project_dir}" '-DMODEM_PWRKEY_LEVEL=H'
  marvie_base_version=$(cmake -LA "${project_dir}" | grep MARVIE_FIRMWARE_VERSION_BASE | cut -d'=' -f2)
  cmake --build "${project_build_dir}" --config "${build_type}" --target marvie_firmware.elf.bin

  new_filename="${project_build_dir}/marvie.firmware.${marvie_base_version}H.bin"
  mv "${project_build_dir}/marvie_firmware.elf.bin" "${new_filename}"
  sha1sum "${new_filename}" > "${new_filename}.sha1"

  # build L version of firmware
  configure_cmake "${project_dir}" '-DMODEM_PWRKEY_LEVEL=L'
  cmake --build "${project_build_dir}" --config "${build_type}" --target marvie_firmware.elf.bin

  new_filename="${project_build_dir}/marvie.firmware.${marvie_base_version}L.bin"
  mv "${project_build_dir}/marvie_firmware.elf.bin" "${new_filename}"
  sha1sum "${new_filename}" > "${new_filename}.sha1"

  generate_build_info "${artifacts_dir}/firmware"

  # copy artifacts
  cp "${project_build_dir}"/marvie.firmware*.{bin,sha1} "${artifacts_dir}/firmware"
}

function build_bootloader ()
{
  project_dir="${repository_root_dir}/Firmware/Bootloader"
  project_build_dir="${project_dir}/build"

  configure_cmake "${project_dir}"
  cmake --build "${project_build_dir}" --config "${build_type}" --target bootloader.elf.bin

  new_filename="${project_build_dir}/marvie.bootloader.bin"
  mv "${project_build_dir}/bootloader.elf.bin" "${new_filename}"
  sha1sum "${new_filename}" > "${new_filename}.sha1"

  generate_build_info "${artifacts_dir}/bootloader"

  # copy artifacts
  cp "${project_build_dir}"/marvie.bootloader*.{bin,sha1} "${artifacts_dir}/bootloader"
}


mkdir -p "${artifacts_dir}"

build_firmware
build_bootloader

echo $marvie_base_version > "${repository_root_dir}"/version
