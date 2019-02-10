#!/usr/bin/env bash
#
# expects the following parameters to be passed in this specific order:
#  - project root folder                        e.g. BITBUCKET_CLONE_DIR
#  - user credentials in form username:password e.g. BB_UPLOADER_AUTH_STRING
#  - username of the repository owner           e.g. BITBUCKET_REPO_OWNER
#  - repository name                            e.g. BITBUCKET_REPO_SLUG

repository_root_dir="$1"
auth="$2"
repo_owner="$3"
repo_slug="$4"

artifacts_dir="${repository_root_dir}"/artifacts
marvie_version=$(cat "${repository_root_dir}"/version)

function upload_to_bitbucket ()
{
  curl -s -u "${auth}" -X POST https://api.bitbucket.org/2.0/repositories/"${repo_owner}"/"${repo_slug}"/downloads -F files=@"$1"
}

target_name="marvie.firmware.v$marvie_version.zip"
zip -j -r "${target_name}" "${artifacts_dir}"/firmware
upload_to_bitbucket "${target_name}"

target_name="marvie.bootloader.zip"
zip -j -r "${target_name}" "${artifacts_dir}"/bootloader
upload_to_bitbucket "${target_name}"
