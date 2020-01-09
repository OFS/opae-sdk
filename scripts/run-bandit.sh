#!/bin/bash

python -m  virtualenv bandit-scan
. bandit-scan/bin/activate
pip install requests[security]
pip install bandit

bandit -r ./tools -o bandit_report.html -f html

ec=$?

if [ $ec == 0 ]; then
  echo "Bandit scan completed without issue."
else
  echo "Bandit scan issue(s) found."
fi

deactivate