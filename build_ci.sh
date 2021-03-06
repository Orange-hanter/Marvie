#!/usr/bin/env bash
#
# expects the following parameters to be passed in this specific order:
#  - build type,          e.g. firmware/software_linux/software_windows
#  - project root folder, e.g. BITBUCKET_CLONE_DIR

build_target="$1"
repository_root_dir="$2"
artifacts_dir="${repository_root_dir}/artifacts"

# these settings can be changed safely
firmware_build_type="RelWithDebInfo"
#software_build_type="Release"

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
  cmake --build "${project_build_dir}" --config "${firmware_build_type}" --target marvie_bootloader.elf.bin

  # set proper name of *.bin binary and get its sha256
  local new_filename_mask="${project_build_dir}/marvie.bootloader"
  mv "${project_build_dir}/marvie_bootloader.elf.bin" "${new_filename_mask}.bin"
  calc_sha256 "${new_filename_mask}.bin"

  # set proper name of *.bin binary and get its sha256
  mv "${project_build_dir}/marvie_bootloader.elf" "${new_filename_mask}.elf"
  calc_sha256 "${new_filename_mask}.elf"

  mkdir -p "${artifacts_dir}/bootloader/"
  for file in "${project_build_dir}"/marvie.bootloader*.{bin,elf,sha256}; do
    mv "${file}" "${artifacts_dir}/bootloader/"
  done

  generate_build_info "${artifacts_dir}/bootloader"
}

function build_software_linux ()
{
  local project_dir="${repository_root_dir}/Software"
  local project_appdir="${project_dir}/appdir"

  # set branch and commit info to the app title
  local branch=$(git rev-parse --abbrev-ref HEAD)
  local commit=$(git rev-parse --short HEAD)
  local built_at=$(date -u)
  sed -i -e "s/QString(\"MarvieControl\")/QString(\"MarvieControl - built: ${built_at} [${branch}: ${commit}]\")/g" "${project_dir}/main.cpp"

  # build
  cd "${project_dir}"
  qmake CONFIG+=release PREFIX=/usr
  make -j$(nproc)
  make INSTALL_ROOT=appdir -j$(nproc) install; find appdir/
  unset QTDIR; unset QT_PLUGIN_PATH; unset LD_LIBRARY_PATH

  mkdir -p "${project_appdir}"/usr/share/MarvieControl/
  cp -R "${project_dir}/Sensors" "${project_appdir}"/usr/share/MarvieControl/
  cp -R "${project_dir}/Animations" "${project_appdir}"/usr/share/MarvieControl/
  cp -R "${project_dir}/Xml" "${project_appdir}"/usr/share/MarvieControl/
  cp -R "${project_dir}/icons" "${project_appdir}"/usr/share/MarvieControl/
  cp -R "${project_dir}/fonts" "${project_appdir}"/usr/share/MarvieControl/
  cp    "${project_dir}/MarvieControl.png" "${project_appdir}"

  linuxdeployqt --appimage-extract
  ./squashfs-root/AppRun "${project_appdir}"/usr/share/applications/*.desktop -appimage

  # ugly HACK
  mv "MarvieControl-${commit}-x86_64.AppImage" 'MarvieControl-x86_64.AppImage'
}

function build_software_windows ()
{
  local project_dir="${repository_root_dir}/Software"

  cd "${project_dir}"
  qmake CONFIG+=release
  make -j$(nproc)
}

if [[ "${build_target}" == 'firmware_develop' ]]; then
  build_firmware H
  build_bootloader
elif [[ "${build_target}" == 'firmware' ]]; then
  build_firmware L
  build_firmware H
  build_bootloader
  echo $marvie_base_version > "${repository_root_dir}/version"
elif [[ "${build_target}" == 'software_linux' ]]; then
  build_software_linux
elif [[ "${build_target}" == 'software_windows' ]]; then
  build_software_windows
fi