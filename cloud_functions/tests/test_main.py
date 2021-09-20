import datetime
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
def logging(monkeypatch):
    monkeypatch.setattr(receiver, "cloudlogging", mock.MagicMock())


@pytest.fixture(scope="module")
def app():
    return flask.Flask(__name__)


@pytest.fixture
def db(monkeypatch):
    cred = mock.MagicMock(spec=google.auth.credentials.Credentials)
    firebase_admin.initialize_app()
    client = firestore.Client(
        project=os.getenv("FIRESTORE_PROJECT_ID"), credentials=cred
    )
    monkeypatch.setattr(receiver, "_get_firestore_client", lambda: client)
    return client


@pytest.fixture
def empty_db(db):
    for doc in db.collection(u"weather").stream():
        doc.reference.delete()


# https://github.com/GoogleCloudPlatform/python-docs-samples/blob/master/functions/helloworld/main_test.py
@freezegun.freeze_time("2020-01-01")
def test_adds_entry_for_date(app, db, empty_db):
    data = {"key": "value"}
    expected_doc = {
        "20200101": [{"timestamp": datetime.datetime.now().isoformat(), **data}]
    }
    with app.test_request_context(json=data):
        assert main.receiver_function(flask.request) == "OK"

        docs = db.collection(u"weather").stream()

        actual_doc = next(docs).to_dict()
        print(actual_doc)
        assert actual_doc == expected_doc
