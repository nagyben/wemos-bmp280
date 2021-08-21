import logging
import os
import unittest.mock as mock

import firebase_admin
from firebase_admin import firestore

LOG = logging.getLogger(__name__)


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
    # project=os.getenv("FIRESTORE_PROJECT_ID"), client_options={"api_endpoint": os.getenv("FIRESTORE_EMULATOR_HOST")}

    LOG.debug("Inserting shtuff")
    db.collection("weather").document("test").set({"key": "value"})

    return f"OK"


def _get_firestore_client():
    LOG.info("Getting firestore client...")
    return firestore.Client()
