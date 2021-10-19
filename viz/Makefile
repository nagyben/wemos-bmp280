.PHONY: test
test:
	poetry run python -m pytest tests/

.PHONY: test-s
test-s:
	poetry run python -m pytest tests/ -s

.PHONY: install
install:
	poetry install

.PHONY: format
format:
	poetry run black . && poetry run isort .

.PHONY: lint
lint:
	poetry run black --check . && poetry run isort --check .