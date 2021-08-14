import main
import unittest.mock as mock

def test_receiver():
    assert main.receiver_function(mock.MagicMock()) == "Hello World!"