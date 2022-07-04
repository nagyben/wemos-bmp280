import datetime
import os
import unittest.mock as mock

import firebase_admin
import google.auth.credentials
import numpy
import pandas
import pandas.testing
import pytest
from google.auth.credentials import AnonymousCredentials
from google.cloud import firestore, storage

import viz

FIREBASE_COLLECTION = "weather-test"
STATIC_SITE_BUCKET = "test.weather.com"


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


def delete_documents(db: firestore.Client):
    docs = db.collection(FIREBASE_COLLECTION).stream()

    for doc in docs:
        doc.reference.delete()


@pytest.fixture
def gcs_client(monkeypatch):
    client = storage.Client(credentials=AnonymousCredentials(), project="test")
    monkeypatch.setattr(viz, "gcs_client", lambda: client)
    monkeypatch.setenv("STATIC_SITE_BUCKET", STATIC_SITE_BUCKET)
    yield client
    for bucket in client.list_buckets():
        bucket.delete(force=True)


def test_load_data(firestore_data, db):
    delete_documents(db)
    data = firestore_data.to_dict(orient="records")
    db.collection(FIREBASE_COLLECTION).document("20210101").set(
        {
            "date": "2020-01-01",
            "data": data,
        }
    )
    df = viz.load_data()
    pandas.testing.assert_frame_equal(firestore_data, df, check_like=True)


def test_load_multiple_data(db):
    data = {
        "timestamp": datetime.datetime.now(),
        "key": numpy.nan,
    }
    data2 = {
        "timestamp": datetime.datetime.now(),
        "key": numpy.nan,
    }
    expected = pandas.DataFrame([data, data2])
    expected["timestamp"] = expected["timestamp"].dt.tz_localize("utc")
    db.collection(FIREBASE_COLLECTION).document("20210101").set(
        {
            "date": "2020-01-01",
            "data": [data],
        }
    )
    db.collection(FIREBASE_COLLECTION).document("20210102").set(
        {
            "date": "2020-01-02",
            "data": [data2],
        }
    )
    df = viz.load_data()
    print(expected)
    print("=====")
    print(df)
    pandas.testing.assert_frame_equal(expected, df, check_like=True)


def test_update_uploads_output_to_bucket(gcs_client, db, firestore_data):
    bucket = gcs_client.create_bucket(STATIC_SITE_BUCKET)

    data = firestore_data.to_dict(orient="records")
    db.collection(FIREBASE_COLLECTION).document("20210101").set(
        {
            "date": "2020-01-01",
            "data": data,
        }
    )

    viz.update()

    assert bucket.blob("index.html").exists()
