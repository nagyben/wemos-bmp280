import datetime
import json
import os
import time

import numpy
import pandas
import pytest
import requests
from google.cloud import firestore, pubsub_v1, storage

URL = os.getenv("URL")

GOOGLE_PROJECT = os.environ["GOOGLE_PROJECT"]
TOPIC_NAME = os.environ["TOPIC_NAME"]
STATIC_SITE_BUCKET = os.environ["STATIC_SITE_BUCKET"]
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


@pytest.fixture()
def db():
    dbclient = firestore.Client(project=GOOGLE_PROJECT)
    yield dbclient
    delete_documents(dbclient)


def delete_documents(db: firestore.Client):
    docs = db.collection(FIREBASE_COLLECTION).stream()

    for doc in docs:
        doc.reference.delete()

@pytest.fixture
def static_site_bucket():
    client = storage.Client()
    bucket = client.get_bucket(STATIC_SITE_BUCKET)
    yield bucket
    for bucket in client.list_buckets():
        bucket.delete_blobs(bucket.list_blobs())

def test_receiver_e2e(db):
    # Instantiates a Pub/Sub client
    publisher = pubsub_v1.PublisherClient()

    topic_path = publisher.topic_path(GOOGLE_PROJECT, TOPIC_NAME)

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

    docs = db.collection(FIREBASE_COLLECTION).stream()

    try:
        actual = next(docs).to_dict()
        print(actual)
        assert "data" in actual
        assert "data" in actual["data"][0]
    except StopIteration:
        raise LookupError(f"Could not find any records in {FIREBASE_COLLECTION}")


def test_viz_e2e(db, firestore_data):
    print(f"URL: {URL}")
    print(f"PROJECT_ID: {GOOGLE_PROJECT}")
    print(f"TOPIC_NAME: {TOPIC_NAME}")

    delete_documents(db)

    # trigger function by publishing mqtt message
    publisher = pubsub_v1.PublisherClient()

    topic_path = publisher.topic_path(GOOGLE_PROJECT, TOPIC_NAME)

    print(f"Topic path: {topic_path}")

    data = firestore_data.iloc[-1].to_dict()
    print(data)
    message_json = json.dumps(data)
    message_bytes = message_json.encode("utf-8")
    publish_future = publisher.publish(topic_path, data=message_bytes)
    publish_future.result()  # Verify the publish succeeded

    time.sleep(5)  # wait for the cloud function to do its thing

    # assert viz page is available
    response = requests.get(URL)
    assert response.status_code == 200


