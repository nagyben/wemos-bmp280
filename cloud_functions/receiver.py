import logging
import os
import unittest.mock as mock

from firebase_admin import firestore


def receiver(request):
    """Responds to any HTTP request.
    Args:
        request (flask.Request): HTTP request object.
    Returns:
        The response text or any set of values that can be turned into a
        Response object using
        `make_response <https://flask.palletsprojects.com/en/1.1.x/api/#flask.Flask.make_response>`.
    """
    db = _get_firestore_client()
    print(request.json)
    print("Inserting shtuff")
    db.collection("weather").document("test").set({"key": "value"})

    return f"OK"


def _get_firestore_client():
    LOG.info("Getting firestore client...")
    return firestore.Client()
