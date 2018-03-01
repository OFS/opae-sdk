#!/bin/sh

setup_git() {
  git config --global user.email "nakul.korde@intel.com"
  git config --global user.name "Nakul Korde"
}

commit_website_files() {
  mkdir upload_docs
  cd upload_docs
  git clone https://github.com/OPAE/opae.github.io.git
  cd opae.github.io
  cp -r ../../mybuild_docs/sphinx/html/* .
  git add -A
  git commit --message "Travis build: Update Documentation under travis build : $TRAVIS_BUILD_NUMBER"
}

upload_files() {
  git push --quiet --set-upstream origin master 
}

setup_git 
commit_website_files
upload_files

echo "Latest documentation uploaded to opae.github.io"