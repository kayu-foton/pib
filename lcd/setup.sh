#!/bin/bash

set -e

VENV_DIR="lcd-env"

if [ ! -d "$VENV_DIR" ]; then
	python3 -m venv "$VENV_DIR"
fi

source "$VENV_DIR/bin/activate"

if [ -f "requirements.txt" ]; then
	pip install --upgrade pip
	pip install -r requirements.txt
fi
