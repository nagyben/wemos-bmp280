# list of all targets
.PHONY: help
help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  install           to install dependencies"
	@echo "  format            to format code"
	@echo "  lint              to lint code"
	@echo "  test              to run tests"
	@echo "  test-s            to run tests with stdout"
	@echo "  test-ui           to run ui tests"
	@echo "  run               to run the server"

.PHONY: test
test:
	poetry run python -m pytest tests/

.PHONY: test-s
test-s:
	poetry run python -m pytest tests/ -s

.PHONY: test-ui
test-ui:
	poetry run python -m pytest ui_tests/

.PHONY: install
install:
	poetry install

.PHONY: format
format:
	poetry run black . && poetry run isort .

.PHONY: lint
lint:
	poetry run mypy --strict . && poetry run black --check . && poetry run isort --check .

.PHONY: run
run:
	poetry run python test_server.py