#!/bin/sh

echo "update libubloxconf submodules ..."
./submodule-update.sh

cd lib/libosporting
./submodule-commit.sh
cd -

echo "commit libubloxconf submodules"
git add lib/libosporting
git commit -m "update submodules."
git push

