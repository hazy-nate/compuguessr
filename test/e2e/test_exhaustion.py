# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import requests
import uuid

def test_database_boundary_exhaustion(live_server):
    """Spams registration to hit the 1024 user database limit."""
    success_count = 0
    failure_caught = False

    # Attempt to create 1050 users (beyond the 1024 limit)
    for _ in range(1050):
        res = requests.post(f"{live_server}/api/auth/register", data={
            "username": f"usr_{uuid.uuid4().hex[:8]}",
            "password": "pwd"
        })

        if res.status_code == 200:
            success_count += 1
        else:
            # We hit the DB boundary, it should reject gracefully (e.g., 500 or 409)
            failure_caught = True
            break

    # We should never exceed the fixed memory pool size
    assert success_count <= 1024
    assert failure_caught is True, "Database failed to reject users after reaching capacity"

    # CRITICAL: Verify the daemon is still alive and didn't segfault when it hit the limit
    lb_res = requests.get(f"{live_server}/api/leaderboard")
    assert lb_res.status_code == 200
