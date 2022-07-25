from typing import Any, Dict

import viz


def viz_function(*args: Any, **kwargs: Dict[str, Any]) -> None:
    return viz.update()
