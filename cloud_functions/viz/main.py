from typing import Any

import viz


def viz_function(*args: Any, **kwargs: dict[str, Any]) -> None:
    return viz.update()
