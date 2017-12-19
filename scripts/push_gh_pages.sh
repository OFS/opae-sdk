#!/bin/bash

# Exit with nonzero exit code if anything fails
set -e

SOURCE_BRANCH="master"
TARGET_BRANCH="gh-pages"

# Pull requests and commits to other branches shouldn't try to deploy, just build to verify
if [ "$TRAVIS_PULL_REQUEST" != "false" -o "$TRAVIS_BRANCH" != "$SOURCE_BRANCH" ]; then
    echo "Skipping deploy; just doing a build."
    exit 0
fi

SHA=`git rev-parse --verify HEAD`

if [ -n "$GITHUB_API_KEY" ]; then
  cd mybuild_docs/sphinx/html
  git init
  git checkout -b gh-pages
  git add --all .
  git -c user.name='Abelardo Jara-Berrocal' -c user.email='abelardo.jara-berrocal@intel.com' commit -m init
  # Make sure to make the output quiet, or else the API token will leak!
  # API key can replace the password.
  git push -f -q https://abelardojarab:$GITHUB_API_KEY@github.com/OPAE/opae-sdk gh-pages &2>/dev/null
fi
