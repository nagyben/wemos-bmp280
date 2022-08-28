import datetime
import math
import unittest.mock as mock

import flask
import numpy
import pandas

import viz

app = flask.Flask(__name__)

DAYS = 10
MOCK_DATA = pandas.DataFrame(
    [
        {
            "key": numpy.nan,
            "timestamp": (
                datetime.datetime.today()
                - datetime.timedelta(days=DAYS)
                + datetime.timedelta(minutes=i)
            ).isoformat(),
            "humidity_%": 62.50879 * math.sin(i / 180 * math.pi),
            "wifiConnecTime_ms": numpy.nan,
            "pressure_Pa": 98756.16 * math.sin(i / 180 * math.pi),
            "Vcc": 981.0 * math.sin(i / 180 * math.pi),
            "temp_C": 16.16 * math.sin(i / 180 * math.pi),
            "git_rev": "v0.1-12-g4b3b3b2",
            "start_ms": 42.0,
            "preConnectTime_ms": 48.0,
            "postGcpToken_ms": 13631.0,
            "postConnectTime_ms": 6397.0,
        }
        for i in range(DAYS * 24 * 60)
    ]
)


@app.route("/")
def page():
    return viz.render()


@app.route("/<path:path>")
def file(path):
    return flask.send_from_directory("", path)


app.run("0.0.0.0", port=8000, debug=True)
