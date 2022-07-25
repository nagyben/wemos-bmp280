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

.PHONY: show-output
show-output:
	python -m webbrowser -t "test.html"