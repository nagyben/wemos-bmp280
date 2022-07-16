import datetime
import tempfile
from pty import slave_open
from unittest import mock

import numpy
import pandas
import pytest
from playwright.sync_api import expect, sync_playwright

import viz


@pytest.fixture(scope="session")
def monkeysession():
    mp = pytest.MonkeyPatch()
    yield mp
    mp.undo()


@pytest.fixture(scope="session")
def firestore_data():
    return pandas.DataFrame(
        [
            {
                "key": numpy.nan,
                "timestamp": datetime.datetime(2020, 1, 1, 12, i * 5).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": 98756.16,
                "Vcc": 981.0,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postGcpToken_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for i in range(10)
        ]
    )


@pytest.fixture(scope="session")
def mock_load_data(monkeysession, firestore_data):
    m = mock.MagicMock()
    m.return_value = firestore_data
    monkeysession.setattr(viz, "load_data", m)


@pytest.fixture(scope="session")
def index_page(mock_load_data, firestore_data):
    with tempfile.NamedTemporaryFile(suffix=".html") as f:
        f.write(viz.render().encode("utf-8"))

        with sync_playwright() as p:
            browser = p.chromium.launch()
            page = browser.new_page()
            page.goto(f"file://{f.name}")
            yield page
            browser.close()


def test_renders_temperature(index_page, firestore_data):
    expect(index_page.locator("#temp_C h1")).to_have_text(
        f'{firestore_data.iloc[-1].loc["temp_C"]:.1f} \xb0C', timeout=500
    )


def test_renders_humidity(index_page, firestore_data):
    expect(index_page.locator("#humidity h1")).to_have_text(
        f'{firestore_data.iloc[-1].loc["humidity_%"]:.0f}%', timeout=500
    )

def test_renders_pressure(index_page, firestore_data):
    expect(index_page.locator("#pressure h1")).to_have_text(
        f'{firestore_data.iloc[-1].loc["pressure_Pa"]/100:.0f} mbar', timeout=500
    )


def test_title(index_page):
    assert index_page.title() == "Weather | 7 Villebon Way"
