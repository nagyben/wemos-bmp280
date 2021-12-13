import os

import google.auth.transport.requests
import google.oauth2.id_token
import pytest
import requests
from google.cloud import firestore

RECEIVER_URL = os.getenv("RECEIVER_URL")
GOOGLE_PROJECT = os.getenv("GOOGLE_PROJECT")

COLLECTION = "test_weather"


@pytest.fixture(scope="session")
def token():
    auth_req = google.auth.transport.requests.Request()
    return google.oauth2.id_token.fetch_id_token(auth_req, RECEIVER_URL)


def delete_documents(db: firestore.Client):
    docs = db.collection(COLLECTION).stream()

    for doc in docs:
        doc.reference.delete()


def test_receiver(token):
    db = firestore.Client(project=GOOGLE_PROJECT)
    delete_documents(db)

    new_doc = {"key": "value"}

    response = requests.post(
        RECEIVER_URL, json=new_doc, headers={"Authorization": f"Bearer {token}"}
    )

    print(response.content)
    assert response.status_code == 200

    docs = db.collection(COLLECTION).stream()

    actual = next(docs).to_dict()
    print(actual)
    assert actual == new_doc
