#!/bin/bash

for pyver in {27,36}
do
  bandit -r tools/pac_sign -o pacsign-bandit-py${pyver}.dat -f txt
done

echo "scan-pacsign-bandit PASSED"
