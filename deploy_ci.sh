#!/usr/bin/env bash
#
# expects the following parameters to be passed in this specific order:
#  - artifact type                                e.g. firmware/software
#  - project root folder                          e.g. BITBUCKET_CLONE_DIR
#  - user credentials in form username:password   e.g. BB_UPLOADER_AUTH_STRING
#  - username of the repository owner             e.g. BITBUCKET_REPO_OWNER
#  - repository name                              e.g. BITBUCKET_REPO_SLUG

artifact_type="$1"
repository_root_dir="$2"
auth="$3"
repo_owner="$4"
repo_slug="$5"

artifacts_dir="${repository_root_dir}/artifacts"

function upload_to_bitbucket ()
{
  curl -s -u "${auth}" -X POST "https://api.bitbucket.org/2.0/repositories/${repo_owner}/${repo_slug}/downloads" -F "files=@$1"
}

function deploy_firmware ()
{
  marvie_version=$(cat "${repository_root_dir}/version")

  local target_name="marvie.firmware.v${marvie_version}.zip"
  zip -j -r "${target_name}" "${artifacts_dir}/firmware"
  upload_to_bitbucket "${target_name}" &

  local target_name="marvie.bootloader.zip"
  zip -j -r "${target_name}" "${artifacts_dir}/bootloader"
  upload_to_bitbucket "${target_name}" &

  wait
}

function deploy_software ()
{
  # TODO: rewrite
  local target_name="${repository_root_dir}"/Software/MarvieControl-x86_64.AppImage
  upload_to_bitbucket "${target_name}"
}

if [[ "${artifact_type}" == 'firmware' ]]; then
  deploy_firmware
elif [[ "${artifact_type}" == 'software' ]]; then
  deploy_software
fi