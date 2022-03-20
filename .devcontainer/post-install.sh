#!/usr/bin/env bash

set -eux -o pipefail

# install poetry
curl -sSL https://raw.githubusercontent.com/python-poetry/poetry/master/get-poetry.py | python -
echo source /home/vscode/.poetry/env >> /home/vscode/.zshrc
echo source /home/vscode/.poetry/env >> /home/vscode/.bashrc
source $HOME/.poetry/env
# poetry config virtualenvs.create false