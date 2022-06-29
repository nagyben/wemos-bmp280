import os

import requests

URL = os.getenv("URL")


def test_viz():
    print(f"URL: {URL}")
    response = requests.get(URL)
    assert response.status_code == 200
