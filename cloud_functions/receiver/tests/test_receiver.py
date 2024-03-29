import base64
import datetime
import json
import logging
import os
import unittest.mock as mock

import firebase_admin
import flask
import freezegun
import google.auth.credentials
import pytest
from google.cloud import firestore

import main
import receiver


@pytest.fixture(autouse=True)
def mock_cloudlogging(monkeypatch, caplog):
    monkeypatch.setattr(receiver, "cloudlogging", mock.MagicMock())
    caplog.set_level(logging.DEBUG)


@pytest.fixture(scope="module")
def firebase():
    firebase_admin.initialize_app()


@pytest.fixture
def db(monkeypatch):
    monkeypatch.setenv("FIRESTORE_COLLECTION_NAME", "weather-test")
    cred = mock.MagicMock(spec=google.auth.credentials.Credentials)
    client = firestore.Client(
        project=os.getenv("FIRESTORE_PROJECT_ID"), credentials=cred
    )
    monkeypatch.setattr(receiver, "_get_firestore_client", lambda: client)
    return client


@pytest.fixture
def empty_db(db):
    for doc in db.collection("weather-test").stream():
        doc.reference.delete()


# https://github.com/GoogleCloudPlatform/python-docs-samples/blob/master/functions/helloworld/main_test.py
@freezegun.freeze_time()
def test_adds_entry_for_date(firebase, db, empty_db):
    data = {"key": "value"}
    expected_key = datetime.datetime.now().strftime("%Y%m%d")
    expected_doc = {
        "date": datetime.datetime.now().strftime("%Y-%m-%d"),
        "data": [{"timestamp": datetime.datetime.now().isoformat(), **data}],
    }

    context = {
        "herp": "derp",
        "event_id": "event_id",
        "timestamp": datetime.datetime.now().isoformat(),
        "resource": {"name": "herp"},
        "data": base64.b64encode(json.dumps(data).encode()),
    }

    event = {}

    assert main.receiver_function(context, event) == "OK"

    actual_doc = db.collection("weather-test").document(expected_key).get()
    assert actual_doc.exists
    print(actual_doc)
    assert actual_doc.to_dict() == expected_doc


@freezegun.freeze_time()
def test_adds_entry_to_existing_entries_on_same_day(firebase, db, empty_db):
    data = {"key": "value"}
    expected_key = datetime.datetime.now().strftime("%Y%m%d")
    db.collection("weather-test").document(expected_key).set(
        {
            "date": datetime.datetime.now().strftime("%Y-%m-%d"),
            "data": [
                {"timestamp": datetime.datetime(2020, 1, 1, 0, 0).isoformat(), **data},
                {"timestamp": datetime.datetime(2020, 1, 1, 0, 5).isoformat(), **data},
                {"timestamp": datetime.datetime(2020, 1, 1, 0, 10).isoformat(), **data},
            ],
        }
    )

    expected_doc = {
        "date": datetime.datetime.now().strftime("%Y-%m-%d"),
        "data": [
            {"timestamp": datetime.datetime(2020, 1, 1, 0, 0).isoformat(), **data},
            {"timestamp": datetime.datetime(2020, 1, 1, 0, 5).isoformat(), **data},
            {"timestamp": datetime.datetime(2020, 1, 1, 0, 10).isoformat(), **data},
            {"timestamp": datetime.datetime.now().isoformat(), **data},
        ],
    }

    context = {
        "herp": "derp",
        "event_id": "event_id",
        "timestamp": datetime.datetime.now().isoformat(),
        "resource": {"name": "herp"},
        "data": base64.b64encode(json.dumps(data).encode()),
    }

    event = {}

    assert main.receiver_function(context, event) == "OK"

    actual_doc = db.collection("weather-test").document(expected_key).get()
    assert actual_doc.exists
    print(actual_doc.to_dict())
    print(expected_doc)
    assert actual_doc.to_dict() == expected_doc


def test_receiver_handles_event_messages(firebase, db, empty_db):
    data = {"key": "value"}
    db.collection("weather-test").document("20200101").set(
        {
            "date": "2020-01-01",
            "data": [
                {"timestamp": datetime.datetime(2020, 1, 1, 0, 0).isoformat(), **data},
                {"timestamp": datetime.datetime(2020, 1, 1, 0, 5).isoformat(), **data},
                {"timestamp": datetime.datetime(2020, 1, 1, 0, 10).isoformat(), **data},
            ],
        }
    )

    data = "garden1-connected"
    context = {
        "herp": "derp",
        "event_id": "event_id",
        "timestamp": datetime.datetime.now().isoformat(),
        "resource": {"name": "herp"},
        "data": base64.b64encode(data.encode("utf-8")),
        "attributes": {"deviceId": "herp1", "subFolder": "events"},
    }

    event = {}

    assert main.receiver_function(context, event) == "OK"
