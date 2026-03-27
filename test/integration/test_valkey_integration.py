
import pytest
import redis
import requests

def test_app_sets_valkey_key():
    r = redis.Redis(host='valkey', port=6379)
    r.flushall()

    response = requests.get("http://app:8080/guess?val=42")

    assert response.status_code == 200

    val = r.get("last_guess")
    assert val == b"42"
