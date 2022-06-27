import json
import os
import time

import google.auth.transport.requests
import google.oauth2.id_token
import pytest
import requests
from google.cloud import firestore, pubsub_v1

GOOGLE_PROJECT = os.getenv("GOOGLE_PROJECT")

COLLECTION = "weather-test"


@pytest.fixture(scope="session")
def db():
    return firestore.Client(project=GOOGLE_PROJECT)


def delete_documents(db: firestore.Client):
    docs = db.collection(COLLECTION).stream()

    for doc in docs:
        doc.reference.delete()


def test_mqtt(db):
    # Instantiates a Pub/Sub client
    publisher = pubsub_v1.PublisherClient()
    PROJECT_ID = os.getenv("GOOGLE_PROJECT")
    TOPIC_NAME = os.getenv("TOPIC_NAME")

    topic_path = publisher.topic_path(PROJECT_ID, TOPIC_NAME)

    print(f"Topic path: {topic_path}")

    new_doc = {"data": "test-e2e"}
    message_json = json.dumps(new_doc)
    message_bytes = message_json.encode("utf-8")

    # delete test db contents
    delete_documents(db)

    # Publishes a message
    publish_future = publisher.publish(topic_path, data=message_bytes)
    publish_future.result()  # Verify the publish succeeded

    time.sleep(5)  # wait for the cloud function to do its thing

    docs = db.collection(COLLECTION).stream()

    try:
        actual = next(docs).to_dict()
        print(actual)
        assert "data" in actual
        assert "data" in actual["data"][0]
    except StopIteration:
        raise LookupError(f"Could not find any records in {COLLECTION}")
