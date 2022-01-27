import flask
import json

app = flask.Flask(__name__)

@app.route("/", methods=["GET", "POST"])
def main():
    print(flask.request.data)
    print(flask.request.headers)
    return "OK"


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, ssl_context='adhoc')