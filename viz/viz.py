import datetime

import jinja2
import pandas
import plotly
import plotly.io
import plotly.subplots
from google.cloud import firestore

env = jinja2.Environment(
    loader=jinja2.FileSystemLoader("templates"), autoescape=jinja2.select_autoescape()
)


def render() -> str:
    template = env.get_template("viz.html")
    df = load_data()
    fig = _create_figure(df)
    fig_html = _render_plotly_html(fig)
    return template.render(chart_html=fig_html)


def load_data() -> pandas.DataFrame:
    db = _get_firestore_client()
    data = db.collection("weather").document("20210101").get().to_dict()["data"]
    df = pandas.DataFrame(data)
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

    for trace in traces:
        fig.add_trace(trace)

    fig.update_layout(template="none", hovermode="x")

    return fig


def _render_plotly_html(fig: plotly.graph_objects.Figure) -> str:
    return plotly.io.to_html(
        fig, include_plotlyjs=False, include_mathjax=False, full_html=False
    )
