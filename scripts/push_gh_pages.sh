#!/bin/bash

# Exit with nonzero exit code if anything fails
set -e

SOURCE_BRANCH="master"
TARGET_BRANCH="gh-pages"

# Pull requests and commits to other branches shouldn't try to deploy, just build to verify
if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "$SOURCE_BRANCH" ]; then
    echo "Skipping deploy; just doing a build."
    # exit 0
fi

# Required variables
SHA=`git rev-parse --verify HEAD`
cd mybuild_docs
INTEL_FPGA_API_VER_MAJOR=`cmake .. -LH | grep INTEL_FPGA_API_VER_MAJOR | awk -F "=" '{print $2}'`
INTEL_FPGA_API_VER_MINOR=`cmake .. -LH | grep INTEL_FPGA_API_VER_MINOR | awk -F "=" '{print $2}'`
INTEL_FPGA_API_VER_REV=`cmake .. -LH | grep INTEL_FPGA_API_VER_REV | awk -F "=" '{print $2}'`
cd ..

if [ -n "$GITHUB_API_KEY" ]; then
  cd mybuild_docs/sphinx/html/$INTEL_FPGA_API_VER_MAJOR.$INTEL_FPGA_API_VER_MINOR.$INTEL_FPGA_API_VER_REV
  git init
  git checkout -b gh-pages
  git add --all .
  git -c user.name='Abelardo Jara-Berrocal' -c user.email='abelardo.jara-berrocal@intel.com' commit -m init
  # Make sure to make the output quiet, or else the API token will leak!
  # API key can replace the password.
  git push -f -q https://abelardojarab:$GITHUB_API_KEY@github.com/abelardojarab/opae-doc-test gh-pages &2>/dev/null
fi
