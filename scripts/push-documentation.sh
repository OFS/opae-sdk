#!/bin/bash


setup_git() {
	git config user.name "OPAE Bot"
	git config user.email opae_robot@intel.com
}

commit_website_files() {

  mkdir upload_docs
  cd upload_docs
  git clone https://OPAE:$GIT_TOKEN@github.com/OPAE/opae.github.io.git
  cd opae.github.io


  if [ "$1" = "latest" ]
  then
    temp_dir=`ls ../../sphinx/html/`
    cp -r ../../sphinx/html/$temp_dir/* latest/
  else
    cp -r ../../sphinx/html/* .
  fi

  python ../../../scripts/index_generator.py

  git add -A
  git commit --message "Travis build: Update Documentation under travis build : $TRAVIS_BUILD_NUMBER"
}

upload_files() {
  git push --quiet --set-upstream origin master
  echo "Latest documentation uploaded to opae.github.io"
}

setup_git
commit_website_files "$1"
upload_files
