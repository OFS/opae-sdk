#!/bin/bash
set -x
coverage run -m pytest tests "$@"
coverage html --include=pacsign/*
coverage report --include=pacsign/*
