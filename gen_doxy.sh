#!/bin/bash

if [ ! -d "meta/docs/doxygen-awesome-css" ]; then
    git clone https://github.com/jothepro/doxygen-awesome-css.git meta/docs/doxygen-awesome-css >/dev/null 2>&1
    cd meta/docs/doxygen-awesome-css && git checkout v2.3.4 >/dev/null 2>&1
    cd - >/dev/null
fi

GIT_VERSION_STRING=$(git describe --tags --always --dirty --long 2>/dev/null || echo "unknown")

sed -i "s/^PROJECT_NUMBER.*/PROJECT_NUMBER = \"$GIT_VERSION_STRING\"/" meta/doxy/Doxyfile

doxygen meta/doxy/Doxyfile

mkdir -p meta/docs/html/meta/screenshots
cp meta/screenshots/* meta/docs/html/meta/screenshots/ 2>/dev/null || true