#!/bin/bash

pushd $INCLUDEOS_SRC

git pull --tags

echo ">>> Connecting to GitHub API"
echo "Github username: "

read git_user

tag=`git describe --abbrev=0`
JSON=`curl -u $git_user https://api.github.com/repos/hioa-cs/IncludeOS/releases/tags/$tag`

ASSET=`echo $JSON | ./etc/get_latest_binary_bundle_asset.py`

echo ">>> Fetching asset $ASSET"

filename_tag=`echo $tag | tr . -`

filename="IncludeOS_install_"$filename_tag".tar.gz"

URL="https://github.com/hioa-cs/IncludeOS/releases/download/$tag/$filename"

curl -u $git_user -H "Content-Type: application/octet-stream"  https://api.github.com/repos/hioa-cs/IncludeOS/releases/assets/$ASSET

echo "    Latest tag: $tag"
echo "    Fetching: $URL ..."

#curl $URL

popd
