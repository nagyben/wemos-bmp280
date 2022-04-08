import base64
import datetime
import json
import logging
import os
from typing import Any

from google.cloud import logging as cloudlogging


def _setup_logging() -> None:
    lg_client = cloudlogging.Client()  # type: ignore
    lg_client.setup_logging(log_level=logging.DEBUG)  # type: ignore


from firebase_admin import firestore


def _get_firestore_client() -> firestore.Client:
    logging.debug("Getting firestore client...")
    return firestore.Client()


def receiver(event: Any, context: Any) -> str:
    """Background Cloud Function to be triggered by Pub/Sub.
    Args:
         event (dict):  The dictionary with data specific to this type of
                        event. The `@type` field maps to
                         `type.googleapis.com/google.pubsub.v1.PubsubMessage`.
                        The `data` field maps to the PubsubMessage data
                        in a base64-encoded string. The `attributes` field maps
                        to the PubsubMessage attributes if any is present.
         context (google.cloud.functions.Context): Metadata of triggering event
                        including `event_id` which maps to the PubsubMessage
                        messageId, `timestamp` which maps to the PubsubMessage
                        publishTime, `event_type` which maps to
                        `google.pubsub.topic.publish`, and `resource` which is
                        a dictionary that describes the service API endpoint
                        pubsub.googleapis.com, the triggering topic's name, and
                        the triggering event type
                        `type.googleapis.com/google.pubsub.v1.PubsubMessage`.
    Returns:
        None. The output is written to Cloud Logging.
    """
    COLLECTION = os.getenv("FIRESTORE_COLLECTION_NAME")
    _setup_logging()
    db = _get_firestore_client()
    logging.debug(context)
    logging.debug(event)
    mqtt_message = base64.b64decode(event["data"]).decode("utf-8")
    logging.debug(mqtt_message)

    # return "OK"

    doc = db.collection(COLLECTION).document(datetime.datetime.now().strftime("%Y%m%d"))

    if not doc.get().exists:
        doc.set(
            {
                "date": datetime.datetime.now().strftime("%Y-%m-%d"),
                "data": [
                    {
                        "timestamp": datetime.datetime.now().isoformat(),
                        **json.loads(mqtt_message),
                    }
                ],
            }
        )
    else:
        data = doc.get().to_dict()["data"]
        data.append(
            {
                "timestamp": datetime.datetime.now().isoformat(),
                **json.loads(mqtt_message),
            }
        )
        doc.update({"data": data})

    return "OK"
