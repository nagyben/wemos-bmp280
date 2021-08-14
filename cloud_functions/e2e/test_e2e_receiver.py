import os
import requests

RECEIVER_URL = os.getenv("RECEIVER_URL")

def test_receiver():
    response = requests.post(RECEIVER_URL, json={"key": "value"})

    assert response.status_code == 200