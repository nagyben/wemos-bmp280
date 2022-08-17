import datetime
import os
import unittest.mock as mock
from time import time

import firebase_admin
import freezegun
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


@pytest.fixture
def firestore_data():
    DAYS = 60
    return pandas.DataFrame(
        [
            {
                "key": numpy.nan,
                "timestamp": (
                    datetime.datetime.today()
                    - datetime.timedelta(days=DAYS)
                    + datetime.timedelta(days=i)
                ).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": 98756.16,
                "Vcc": 981.0,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postMqttConnectTime_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for i in range(DAYS)
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


@freezegun.freeze_time(datetime.datetime(2020, 6, 1))
def test_load_data_loads_last_30_days_by_default(db):
    delete_documents(db)
    for key in ["20200601", "20200502", "20200501"]:
        db.collection(FIREBASE_COLLECTION).document(key).set(
            {
                "date": "2020-06-01",
                "data": [{"timestamp": key}],
            }
        )
    expected = pandas.DataFrame([{"timestamp": "20200502"}, {"timestamp": "20200601"}])
    actual = viz.load_data()
    pandas.testing.assert_frame_equal(expected, actual, check_like=True)


@freezegun.freeze_time(time_to_freeze=datetime.datetime(2021, 1, 2))
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
    actual = viz.load_data()
    print(expected)
    print("=====")
    print(actual)
    pandas.testing.assert_frame_equal(expected, actual, check_like=True)


@freezegun.freeze_time(time_to_freeze=datetime.datetime(2020, 6, 1))
def test_load_data_loads_last_n_days(db):

    # == populate DB
    delete_documents(db)
    daterange = pandas.date_range(
        start=datetime.datetime.today() - datetime.timedelta(days=10),
        end=datetime.datetime.today(),
    )
    for date in daterange:
        datestring = date.strftime("%Y%m%d")
        db.collection(FIREBASE_COLLECTION).document(datestring).set(
            {
                "date": date.strftime("%Y-%m-%d"),
                "data": [{"timestamp": datestring}],
            }
        )

    # == define expected output
    expected_date_range = pandas.date_range(
        start=datetime.datetime.today() - datetime.timedelta(days=5),
        end=datetime.datetime.today(),
    )
    expected = pandas.DataFrame(
        [{"timestamp": date.strftime("%Y%m%d")} for date in expected_date_range]
    )

    # == get actual output
    actual = viz.load_data(days=5)

    print(expected)
    print("======")
    print(actual)
    pandas.testing.assert_frame_equal(expected, actual, check_like=True)


def test_upload_with_cache_control_metadata_set(monkeypatch):
    m = mock.MagicMock()
    blob = mock.MagicMock()
    m.return_value.bucket.return_value.blob.return_value = blob
    monkeypatch.setenv("STATIC_SITE_BUCKET", "")

    with mock.patch("viz.gcs_client", new=m):
        viz.upload("<html>")

    assert blob.cache_control == "private, max-age=300"
    blob.patch.assert_called_once()


@pytest.mark.parametrize("adc,volts", [(946, 4.11), (823, 3.56)])
def test_convert_voltage(adc, volts):
    numpy.testing.assert_almost_equal(viz.convert_voltage(adc), volts)


def test_filter_out_Vcc_lt_500():
    df_in = pandas.DataFrame(
        [
            {
                "timestamp": datetime.datetime(2020, 1, 1).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": 98756.16,
                "Vcc": vcc,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postMqttConnectTime_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for vcc in [499, 500, 501]
        ]
    )

    expected = pandas.DataFrame(
        [
            {
                "timestamp": datetime.datetime(2020, 1, 1).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": 98756.16,
                "Vcc": vcc,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postMqttConnectTime_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for vcc in [500, 501]  # filter out everything below Vcc=500
        ]
    )

    actual = viz._filter_out_Vcc_lt_500(df_in)

    print(expected)
    print("====")
    print(actual)
    pandas.testing.assert_frame_equal(actual, expected)


def test_filter_out_lt_90k_pressure():
    df_in = pandas.DataFrame(
        [
            {
                "timestamp": datetime.datetime(2020, 1, 1).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": pa,
                "Vcc": 786,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postMqttConnectTime_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for pa in [89_999, 90_000, 90_001]
        ]
    )

    expected = pandas.DataFrame(
        [
            {
                "timestamp": datetime.datetime(2020, 1, 1).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": pa,
                "Vcc": 786,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postMqttConnectTime_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for pa in [90_000, 90_001]  # filter out everything below Pressure=90k
        ]
    )

    actual = viz._filter_out_lt_90k_pressure(df_in)

    print(expected)
    print("====")
    print(actual)
    pandas.testing.assert_frame_equal(actual, expected)


def test_filter_out_gt_110k_pressure():
    df_in = pandas.DataFrame(
        [
            {
                "timestamp": datetime.datetime(2020, 1, 1).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": pa,
                "Vcc": 786,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postGcpToken_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for pa in [109_999, 110_000, 110_001]
        ]
    )

    expected = pandas.DataFrame(
        [
            {
                "timestamp": datetime.datetime(2020, 1, 1).isoformat(),
                "humidity_%": 62.50879,
                "wifiConnecTime_ms": numpy.nan,
                "pressure_Pa": pa,
                "Vcc": 786,
                "temp_C": 16.16,
                "git_rev": "v0.1-12-g4b3b3b2",
                "start_ms": 42.0,
                "preConnectTime_ms": 48.0,
                "postGcpToken_ms": 13631.0,
                "postConnectTime_ms": 6397.0,
            }
            for pa in [109_999, 110_000]  # filter out everything above Pressure=110k
        ]
    )

    actual = viz._filter_out_gt_110k_pressure(df_in)

    print(expected)
    print("====")
    print(actual)
    pandas.testing.assert_frame_equal(actual, expected)
