.PHONY: test
test:
	python -m pytest tests/

.PHONY: test-s
test-s:
	python -m pytest tests/ -s

.PHONY: install
install:
	poetry install

.PHONY: format
format:
	poetry run black . && poetry run isort .