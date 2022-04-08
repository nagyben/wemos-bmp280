from typing import Any

import receiver


def receiver_function(event: Any, context: Any) -> str:
    return receiver.receiver(event, context)
