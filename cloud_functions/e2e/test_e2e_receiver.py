import os
import requests
from google.cloud import firestore

RECEIVER_URL = os.getenv("RECEIVER_URL")
GOOGLE_PROJECT = os.getenv("GOOGLE_PROJECT")

COLLECTION = "weather"

def delete_documents(db: firestore.Client):
    docs = db.collection(COLLECTION).stream()

    for doc in docs:
        doc.reference.delete()

def test_receiver():
    db = firestore.Client(project=GOOGLE_PROJECT)
    delete_documents(db)

    response = requests.post(RECEIVER_URL, json={"key": "value"})

    print(response.content)
    assert response.status_code == 200

    docs = db.collection(COLLECTION).stream()


    assert len(docs) == 0