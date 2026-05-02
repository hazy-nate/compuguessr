# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import requests
import uuid

def test_guest_authentication(live_server):
    res = requests.post(f"{live_server}/api/auth/guest")
    assert res.status_code == 200
    assert "SESSION" in res.cookies

    data = res.json()
    assert data.get("status") == "success"

def test_registration_and_login_flow(live_server):
    test_user = f"user_{uuid.uuid4().hex[:8]}"
    test_pass = "SecurePass123"

    reg_res = requests.post(f"{live_server}/api/auth/register", data={
        "username": test_user,
        "password": test_pass
    })
    assert reg_res.status_code == 200
    assert "SESSION" in reg_res.cookies
    assert reg_res.json().get("username") == test_user

    dup_res = requests.post(f"{live_server}/api/auth/register", data={
        "username": test_user,
        "password": "AnotherPassword"
    })
    assert dup_res.status_code == 409

    fail_res = requests.post(f"{live_server}/api/auth/login", data={
        "username": test_user,
        "password": "WrongPassword"
    })
    assert fail_res.status_code == 401

    session = requests.Session()
    login_res = session.post(f"{live_server}/api/auth/login", data={
        "username": test_user,
        "password": test_pass
    })
    assert login_res.status_code == 200
    assert "SESSION" in session.cookies
    assert login_res.json().get("username") == test_user
