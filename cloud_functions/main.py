import receiver


def receiver_function(request):
    return receiver.receiver(request)


def receiver_authenticated(request):
    print(request.json)
    return "OK"
