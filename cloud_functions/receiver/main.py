import receiver


def receiver_function(event, context):
    return receiver.receiver(event, context)


def receiver_authenticated(event, context):
    return "OK"
