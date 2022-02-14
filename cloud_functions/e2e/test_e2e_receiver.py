import json
import os

import google.auth.transport.requests
import google.oauth2.id_token
import pytest
import requests
from google.cloud import firestore, pubsub_v1

RECEIVER_URL = os.getenv("RECEIVER_URL")
GOOGLE_PROJECT = os.getenv("GOOGLE_PROJECT")

COLLECTION = "test_weather"


@pytest.fixture(scope="session")
def token():
    auth_req = google.auth.transport.requests.Request()
    return google.oauth2.id_token.fetch_id_token(auth_req, RECEIVER_URL)


@pytest.fixture(scope="session")
def db():
    return firestore.Client(project=GOOGLE_PROJECT)


def delete_documents(db: firestore.Client):
    docs = db.collection(COLLECTION).stream()

    for doc in docs:
        doc.reference.delete()


def test_receiver(token, db):
    delete_documents(db)

    new_doc = {"key": "value"}

    response = requests.post(
        RECEIVER_URL, json=new_doc, headers={"Authorization": f"Bearer {token}"}
    )

    print(response.content)
    assert response.status_code == 200

    docs = db.collection(COLLECTION).stream()

    try:
        actual = next(docs).to_dict()
        print(actual)
        assert actual == new_doc
    except StopIteration:
        raise LookupError(f"Could not find any records in {COLLECTION}")


def test_mqtt(db):
    # Instantiates a Pub/Sub client
    publisher = pubsub_v1.PublisherClient()
    PROJECT_ID = os.getenv("GOOGLE_PROJECT")
    TOPIC_NAME = os.getenv("TOPIC_NAME")

    topic_path = publisher.topic_path(PROJECT_ID, TOPIC_NAME)

    new_doc = {"data": "test-e2e"}
    message_json = json.dumps(new_doc)
    message_bytes = message_json.encode("utf-8")

    # delete test db contents
    delete_documents(db)

    # Publishes a message
    publish_future = publisher.publish(topic_path, data=message_bytes)
    publish_future.result()  # Verify the publish succeeded

    docs = db.collection(COLLECTION).stream()

    try:
        actual = next(docs).to_dict()
        print(actual)
        assert actual == new_doc
    except StopIteration:
        raise LookupError(f"Could not find any records in {COLLECTION}")
