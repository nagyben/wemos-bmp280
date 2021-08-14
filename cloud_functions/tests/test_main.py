import main
import unittest.mock as mock

def test_receiver():
    main.receiver_function(mock.MagicMock())