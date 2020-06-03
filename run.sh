#!/bin/bash
set -x
coverage run -m pytest tests/test_verifier.py -k test_verifier "$@"
coverage html --include=pacsign/* 
