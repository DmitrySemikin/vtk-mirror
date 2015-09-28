#!/bin/bash

set -e
set -x

readonly name="jsoncpp"
readonly subtree="ThirdParty/$name/vtk$name"
readonly update="ThirdParty/$name/update.sh"
readonly repo="https://github.com/open-source-parsers/jsoncpp"
readonly tag="0.10.5"
readonly paths="
include/
src/
pkg-config/

CMakeLists.txt
version.in

LICENSE
"

readonly basehash='2d8129591539371181b5e6becb1cad4e02a2e6a4' # NEWHASH
readonly workdir="$PWD/work"
readonly upstreamdir="$workdir/upstream"
readonly extractdir="$workdir/extract"

trap "rm -rf $workdir" EXIT

git clone "$repo" "$upstreamdir"

if [ -n "$basehash" ]; then
    git worktree add "$extractdir" "$basehash"
else
    mkdir -p "$extractdir"
    git -C "$extractdir" init
fi

git -C "$upstreamdir" archive --prefix="$name-reduced/" "$tag" -- $paths | \
    tar -C "$extractdir" -x

pushd "$extractdir"
rm -rvf $paths
mv -v "$name-reduced/"* .
rmdir -v "$name-reduced/"
git add -A .
git commit -m "$name: update to $tag using update.sh"
readonly newhash="$( git rev-parse HEAD )"
popd

git fetch "$extractdir"
git merge -s ours --no-commit "$newhash"
git read-tree -u --prefix="$subtree/" "$newhash"
sed -i -e "/NEWHASH$/s/='.*'/='$newhash'/" "$update"
git add "$update"
git commit -m "$name: update to $tag"
