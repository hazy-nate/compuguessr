# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import requests

def test_leaderboard_retrieval(live_server):
    res = requests.get(f"{live_server}/api/leaderboard")
    assert res.status_code == 200

    data = res.json()
    assert "leaderboard" in data
    assert isinstance(data["leaderboard"], list)

    # Verify sorting logic (highest points should be first)
    lb = data["leaderboard"]
    for i in range(len(lb) - 1):
        assert lb[i]["points"] >= lb[i+1]["points"]

def test_challenge_verification_requires_auth(live_server):
    res = requests.post(f"{live_server}/api/challenge/verify", data={
        "id": "instruction_001",
        "q1": "1"
    })
    # Cannot submit answers without at least a Guest session
    assert res.status_code == 401

def test_challenge_verification_logic(live_server):
    # Setup a guest session
    client = requests.Session()
    client.post(f"{live_server}/api/auth/guest")

    # Submit an answer to a challenge
    res = client.post(f"{live_server}/api/challenge/verify", data={
        "id": "instruction_001",
        "q1": "WRONG_ANSWER"
    })

    assert res.status_code == 200
    data = res.json()

    # Even if wrong, the API should return a structured evaluation
    assert "status" in data
    assert "total_earned" in data
    assert isinstance(data.get("results"), list)
