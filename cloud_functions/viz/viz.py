import datetime
import json
import logging
import os
import sys
from typing import Any, Dict, List

import jinja2
import pandas
import plotly
import plotly.io
import plotly.subplots
import structlog
from google.cloud import firestore, storage

env = jinja2.Environment(
    loader=jinja2.FileSystemLoader("templates"), autoescape=jinja2.select_autoescape()
)


def configure_logging() -> None:
    """Configure logging for the application."""
    shared_processors = [
        structlog.contextvars.merge_contextvars,
        structlog.processors.add_log_level,
        structlog.processors.StackInfoRenderer(),
        structlog.dev.set_exc_info,
        structlog.processors.TimeStamper(fmt="iso"),
    ]

    processors: list[Any]

    if sys.stderr.isatty():
        print("we are in a tty")
        processors = shared_processors + [
            structlog.dev.ConsoleRenderer(),
        ]

    else:
        processors = [
            structlog.processors.dict_tracebacks,
            structlog.processors.JSONRenderer(),
        ]

    structlog.configure(
        wrapper_class=structlog.make_filtering_bound_logger(logging.NOTSET),
        context_class=dict,
        logger_factory=structlog.PrintLoggerFactory(),
        processors=processors,
    )


configure_logging()
LOGGER = structlog.get_logger()


def render() -> str:
    template = env.get_template("viz.html")
    df = load_data()
    df = preprocess(df)
    fig = _create_figure(df)
    fig_html = _render_plotly_html(fig)
    return template.render(
        chart_html=fig_html,
        **_rename_df_columns_for_template_injection(df),
        date=datetime.datetime.now().strftime("%H:%M on %d %b %Y"),
    )


def preprocess(df: pandas.DataFrame) -> pandas.DataFrame:
    df["timestamp"] = pandas.to_datetime(df["timestamp"], utc=True)
    df["pressure_mbar"] = df["pressure_Pa"] / 100
    df["voltage"] = df["Vcc"].apply(convert_voltage)
    df = _filter_out_Vcc_lt_500(df)
    df = _filter_out_lt_90k_pressure(df)
    df = _filter_out_gt_110k_pressure(df)
    df = _last_n_days(df, 30)
    return df


def _filter_out_Vcc_lt_500(df: pandas.DataFrame) -> pandas.DataFrame:
    return df.loc[df["Vcc"] >= 500].reset_index(drop=True)


def _filter_out_lt_90k_pressure(df: pandas.DataFrame) -> pandas.DataFrame:
    return df.loc[df["pressure_Pa"] >= 90_000].reset_index(drop=True)


def _filter_out_gt_110k_pressure(df: pandas.DataFrame) -> pandas.DataFrame:
    return df.loc[df["pressure_Pa"] <= 110_000].reset_index(drop=True)


def _last_n_days(df: pandas.DataFrame, days: int) -> pandas.DataFrame:
    return df.loc[
        df["timestamp"]
        > datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(days=days)
    ]


def _rename_df_columns_for_template_injection(df: pandas.DataFrame) -> Dict[str, Any]:
    df = df.rename(columns={"humidity_%": "humidity"})
    return df.iloc[-1].to_dict()  # type: ignore


def load_data(days: int = 30) -> pandas.DataFrame:
    collection_name = os.environ["FIREBASE_COLLECTION"]
    db = _get_firestore_client()
    log = LOGGER.bind(firestore_collection_name=collection_name, firestore_client=db)

    df = pandas.DataFrame()

    for date in pandas.date_range(
        start=datetime.datetime.today() - datetime.timedelta(days=days),
        end=datetime.datetime.today(),
    ):
        datestring = date.strftime("%Y%m%d")
        doc = db.collection(collection_name).document(datestring).get()

        if doc.exists and (data := doc.to_dict()):
            df = pandas.concat([df, pandas.DataFrame(data.get("data"))])

    df = df.reset_index(drop=True)
    log = log.bind(df_shape=df.shape)
    log.debug("Loaded data from firestore")
    return df


def _get_firestore_client() -> firestore.Client:
    client = firestore.Client()
    return client


def _create_figure(df: pandas.DataFrame) -> plotly.graph_objects.Figure:
    LOGGER.info("Creating figure")
    MARKER_SIZE = 2
    traces = [
        plotly.graph_objects.Scattergl(
            x=df["timestamp"],
            y=df["temp_C"],
            name="temp_C",
            hovertemplate="<b>%{x}</b><br>%{y:.1f}&deg;C",
            mode="markers",
            marker=dict(size=MARKER_SIZE),
        ),
        plotly.graph_objects.Scattergl(
            x=df["timestamp"],
            y=df["pressure_Pa"] / 100,
            name="pressure_mbar",
            yaxis="y2",
            hovertemplate="<b>%{x}</b><br>%{y:.0f}mbar",
            mode="markers",
            marker=dict(size=MARKER_SIZE),
        ),
        plotly.graph_objects.Scattergl(
            x=df["timestamp"],
            y=df["humidity_%"],
            name="humidity_%",
            yaxis="y3",
            hovertemplate="<b>%{x}</b><br>%{y:.1f}%",
            mode="markers",
            marker=dict(size=MARKER_SIZE),
        ),
        plotly.graph_objects.Scattergl(
            x=df["timestamp"],
            y=df["voltage"],
            name="V",
            yaxis="y4",
            hovertemplate="<b>%{x}</b><br>%{y:.2f}V",
            mode="markers",
            marker=dict(size=MARKER_SIZE),
        ),
        plotly.graph_objects.Scattergl(
            x=df["timestamp"],
            y=df["postConnectTime_ms"],
            name="postConnectTime_ms",
            yaxis="y5",
            hovertemplate="<b>%{x}</b><br>%{y:.0f}ms",
            mode="markers",
            marker=dict(size=MARKER_SIZE),
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
    log = LOGGER.bind(client=client)

    bucket = client.bucket(os.environ["STATIC_SITE_BUCKET"])
    log = log.bind(bucket=bucket)

    blob = bucket.blob("index.html")
    log = log.bind(blob=blob)

    blob.upload_from_string(html, content_type="text/html")

    blob.cache_control = "private, max-age=300"
    blob.patch()

    log.debug("Uploaded blob")


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
