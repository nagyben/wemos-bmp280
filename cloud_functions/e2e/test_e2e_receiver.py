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


def test_mqtt():
    # Instantiates a Pub/Sub client
    publisher = pubsub_v1.PublisherClient()
    PROJECT_ID = os.getenv("GOOGLE_CLOUD_PROJECT")
    TOPIC_NAME = os.getenv("TOPIC_NAME")

    topic_path = publisher.topic_path(PROJECT_ID, TOPIC_NAME)

    message_json = json.dumps(
        {
            "data": {"message": "test"},
        }
    )
    message_bytes = message_json.encode("utf-8")

    # Publishes a message
    try:
        publish_future = publisher.publish(topic_path, data=message_bytes)
        publish_future.result()  # Verify the publish succeeded
        return "Message published."
    except Exception as e:
        print(e)
        return (e, 500)
