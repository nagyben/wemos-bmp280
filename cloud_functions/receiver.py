import datetime
import logging
import os
import unittest.mock as mock

from google.cloud import logging as cloudlogging


def _setup_logging():
    lg_client = cloudlogging.Client()
    lg_client.setup_logging(log_level=logging.DEBUG)


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
    _setup_logging()
    db = _get_firestore_client()
    logging.debug(request.json)

    doc = db.collection("weather").document(datetime.datetime.now().strftime("%Y%m%d"))

    if not doc.get().exists:
        doc.set(
            {
                "date": datetime.datetime.now().strftime("%Y-%m-%d"),
                "data": [
                    {"timestamp": datetime.datetime.now().isoformat(), **request.json}
                ],
            }
        )
    else:
        data = doc.get().to_dict()["data"]
        data.append({"timestamp": datetime.datetime.now().isoformat(), **request.json})
        doc.update({"data": data})

    return f"OK"


def _get_firestore_client():
    logging.debug("Getting firestore client...")
    return firestore.Client()
