import os

import requests

URL = os.getenv("URL")


def test_viz():
    response = requests.get(URL)
    assert response.status_code == 200
