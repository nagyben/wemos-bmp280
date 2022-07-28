import datetime
import os
from typing import Any, Dict

import jinja2
import pandas
import plotly
import plotly.io
import plotly.subplots
from google.cloud import firestore, storage

env = jinja2.Environment(
    loader=jinja2.FileSystemLoader("templates"), autoescape=jinja2.select_autoescape()
)


def render() -> str:
    template = env.get_template("viz.html")
    df = load_data()
    df = preprocess(df)
    fig = _create_figure(df)
    fig_html = _render_plotly_html(fig)
    return template.render(
        chart_html=fig_html, **_rename_df_columns_for_template_injection(df)
    )


def preprocess(df: pandas.DataFrame) -> pandas.DataFrame:
    df["timestamp"] = pandas.to_datetime(df["timestamp"], utc=True)
    df["pressure_mbar"] = df["pressure_Pa"] / 100
    df["voltage"] = df["Vcc"].apply(convert_voltage)
    df = df.loc[
        df["timestamp"]
        > datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(days=30)
    ]
    return df


def _rename_df_columns_for_template_injection(df: pandas.DataFrame) -> Dict[str, Any]:
    df = df.rename(columns={"humidity_%": "humidity"})
    return df.iloc[-1].to_dict()  # type: ignore


def load_data() -> pandas.DataFrame:
    collection_name = os.getenv("FIREBASE_COLLECTION")
    db = _get_firestore_client()
    docs = db.collection(collection_name).stream()

    df = pandas.DataFrame(next(docs).to_dict()["data"])

    for doc in docs:
        df = pandas.concat([df, pandas.DataFrame(doc.to_dict()["data"])])

    # df["timestamp"] = pandas.to_datetime(df["timestamp"], utc=True)#.dt.tz_localize("utc")
    df = df.reset_index(drop=True)
    return df


def _get_firestore_client() -> firestore.Client:
    return firestore.Client()


def _create_figure(df: pandas.DataFrame) -> plotly.graph_objects.Figure:
    traces = [
        plotly.graph_objects.Scatter(
            x=df["timestamp"],
            y=df["temp_C"],
            name="temp_C",
            hovertemplate="<b>%{x}</b><br>%{y:.1f}",
        ),
        plotly.graph_objects.Scatter(
            x=df["timestamp"],
            y=df["pressure_Pa"] / 100,
            name="pressure_mbar",
            yaxis="y2",
            hovertemplate="<b>%{x}</b><br>%{y:.0f}",
        ),
        plotly.graph_objects.Scatter(
            x=df["timestamp"],
            y=df["humidity_%"],
            name="humidity_%",
            yaxis="y3",
            hovertemplate="<b>%{x}</b><br>%{y:.1f}",
        ),
        plotly.graph_objects.Scatter(
            x=df["timestamp"],
            y=df["Vcc"],
            name="Vcc",
            yaxis="y4",
            hovertemplate="<b>%{x}</b><br>%{y:.0f}",
        ),
        plotly.graph_objects.Scatter(
            x=df["timestamp"],
            y=df["postGcpToken_ms"] - df["postConnectTime_ms"],
            name="GcpTokenTime_ms",
            yaxis="y5",
            hovertemplate="<b>%{x}</b><br>%{y:.0f}",
        ),
        plotly.graph_objects.Scatter(
            x=df["timestamp"],
            y=df["postConnectTime_ms"],
            name="postConnectTime_ms",
            yaxis="y6",
            hovertemplate="<b>%{x}</b><br>%{y:.0f}",
        ),
    ]

    fig = plotly.subplots.make_subplots(
        rows=len(traces),
        cols=1,
        shared_xaxes=True,
        vertical_spacing=0.01,
        start_cell="bottom-left",
    )

    fig.update_yaxes(visible=False)
    fig.update_xaxes(showgrid=False)

    for trace in traces:
        fig.add_trace(trace)

    fig.update_layout(
        template="none",
        hovermode="x",
        legend=dict(orientation="h"),
        height=600,
    )

    return fig


def _render_plotly_html(fig: plotly.graph_objects.Figure) -> Any:
    return plotly.io.to_html(
        fig, include_plotlyjs=False, include_mathjax=False, full_html=False
    )


def update() -> None:
    html = render()
    upload(html)


def upload(html: str) -> None:
    client = gcs_client()
    bucket = client.bucket(os.environ["STATIC_SITE_BUCKET"])
    blob = bucket.blob("index.html")
    blob.upload_from_string(html, content_type="text/html")
    blob.cache_control = "private, max-age=300"
    blob.patch()


def gcs_client() -> storage.Client:
    print("real client")
    return storage.Client()


def convert_voltage(adc: int) -> float:
    """Converts the 10-bit adc battery signal to a voltage"""
    # with 129kOhm resistor
    # measured using my yellow multimeter
    # 946 == 4.11V
    # 942 == 4.09V
    # 823 == 3.56V
    x1 = 946
    y1 = 4.11
    x2 = 823
    y2 = 3.56
    m = (y1 - y2) / (x1 - x2)
    # y = mx + c
    # c = y - mx
    c = y1 - (m * x1)

    return m * adc + c
