#!/bin/bash

if [ -n "$GITHUB_API_KEY" ]; then
  cd mybuild_docs/sphinx/html
  git init
  git checkout -b gh-pages
  git add --all .
  git -c user.name='Abelardo Jara-Berrocal' -c user.email='abelardo.jara-berrocal@intel.com' commit -m init
  # Make sure to make the output quiet, or else the API token will leak!
  # API key can replace the password.
  git push -f -q https://abelardojarab:$GITHUB_API_KEY@opae.github.io gh-pages &2>/dev/null
fi
