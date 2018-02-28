#!/bin/sh

setup_git() {
  git config --global user.email "nakul.korde@intel.com"
  git config --global user.name "Nakul Korde"
}

commit_website_files() {
  mkdir upload_docs
  cd upload_docs
  git clone https://github.com/OPAE/opae.github.io.git
  cd doc-testing
  cp -r ../../mybuild_docs/sphinx/html/* .
  git add -A
  git commit --message "Travis build: Update Documentation with traceback: second test commit $TRAVIS_BUILD_NUMBER"
}

upload_files() {
  git push --quiet --set-upstream origin master 
  cd ../../
}

setup_git 
commit_website_files
upload_files

echo "Latest documentation uploaded to opae.github.io"