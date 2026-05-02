# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import requests
import pytest
import uuid

@pytest.fixture
def auth_client(live_server):
    """Registers a fresh user and returns an authenticated requests Session."""
    client = requests.Session()
    username = f"prof_{uuid.uuid4().hex[:8]}"
    client.post(f"{live_server}/api/auth/register", data={
        "username": username,
        "password": "pwd"
    })
    return client, username

def test_profile_retrieval(live_server, auth_client):
    client, username = auth_client

    res = client.get(f"{live_server}/api/user/profile")
    assert res.status_code == 200

    data = res.json()
    assert data.get("username") == username
    assert data.get("is_guest") is False
    assert data.get("total_points") == 0
    assert data.get("description") == ""

def test_unauthenticated_profile_retrieval(live_server):
    # Attempting to fetch a profile without a SESSION cookie
    res = requests.get(f"{live_server}/api/user/profile")
    assert res.status_code == 401

def test_profile_update(live_server, auth_client):
    client, username = auth_client

    html_desc = "Testing <iframe src='https://example.com'></iframe> embedding."
    update_res = client.post(f"{live_server}/api/user/profile/update", data={
        "description": html_desc
    })
    assert update_res.status_code == 200

    # Verify persistence via public endpoint
    pub_res = requests.post(f"{live_server}/api/user/public", data={
        "username": username
    })
    assert pub_res.status_code == 200
    assert pub_res.json().get("description") == html_desc

def test_account_deletion(live_server, auth_client):
    client, username = auth_client

    del_res = client.post(f"{live_server}/api/user/delete")
    assert del_res.status_code == 200

    # Verify the profile is gone
    prof_res = client.get(f"{live_server}/api/user/profile")
    assert prof_res.status_code == 401

    # Verify public lookup fails
    pub_res = requests.post(f"{live_server}/api/user/public", data={
        "username": username
    })
    assert pub_res.status_code == 404
