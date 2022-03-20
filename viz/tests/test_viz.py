import datetime
import os
import unittest.mock as mock

import firebase_admin
import google.auth.credentials
import numpy
import pandas
import pandas.testing
import pytest
from google.cloud import firestore, storage
from google.auth.credentials import AnonymousCredentials

import viz

FIREBASE_COLLECTION = "weather-test"

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


@pytest.fixture
def mock_load_data(monkeypatch, firestore_data):
    m = mock.MagicMock()
    m.return_value = firestore_data
    monkeypatch.setattr(viz, "load_data", m)


def test_renders_html(mock_load_data, firestore_data):
    html = viz.render()
    with open("test.html", "w") as f:
        f.write(html)

    assert 'class="plotly-graph-div"' in html
    assert firestore_data["Vcc"].astype(str).iloc[-1] in html


@pytest.fixture(scope="module")
def firebase():
    firebase_admin.initialize_app()


@pytest.fixture
def db(monkeypatch):
    cred = mock.MagicMock(spec=google.auth.credentials.Credentials)
    client = firestore.Client(
        project=os.getenv("FIRESTORE_PROJECT_ID"), credentials=cred
    )
    monkeypatch.setattr(viz, "_get_firestore_client", lambda: client)
    monkeypatch.setenv("FIREBASE_COLLECTION", FIREBASE_COLLECTION)
    return client

@pytest.fixture
def gcs_client():
    return storage.Client(
        credentials=AnonymousCredentials(),
        project="test"
    )

def test_load_data(firestore_data, db):
    data = firestore_data.to_dict(orient="records")
    db.collection(FIREBASE_COLLECTION).document(u"20210101").set(
        {
            "date": "2020-01-01",
            "data": data,
        }
    )
    df = viz.load_data()
    pandas.testing.assert_frame_equal(firestore_data, df, check_like=True)


def test_uploads_output_to_bucket(gcs_client):
    bucket = gcs_client.create_bucket("bucket")

    viz.

    assert bucket.blob("index.html").exists()
