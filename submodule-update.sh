#!/bin/sh

echo "update libubloxconf submodules ..."
if [ ! -d lib/libosporting/src ]; then
  #git clone -q --depth=1 https://github.com/yhfudev/cpp-osporting.git lib/libosporting
  # git submodule add --name 'libosporting' 'https://github.com/yhfudev/cpp-osporting.git' 'lib/libosporting'
  git submodule init
fi
git submodule update

cd lib/libosporting/
git checkout master
git pull
./submodule-update.sh
cd -

echo "update libubloxconf ..."
git checkout master
git pull

git status lib/libosporting

