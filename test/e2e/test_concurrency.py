# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import concurrent.futures
import requests

def test_high_concurrency_reads(live_server):
    """Blasts the leaderboard endpoint with concurrent connections."""
    def fetch():
        try:
            return requests.get(f"{live_server}/api/leaderboard", timeout=2).status_code
        except requests.RequestException:
            return 0

    # 50 threads firing simultaneously for 250 requests
    with concurrent.futures.ThreadPoolExecutor(max_workers=50) as executor:
        results = list(executor.map(lambda _: fetch(), range(250)))

    # Every single request should be successfully handled by io_uring
    assert all(code == 200 for code in results), "Some requests failed under concurrency load"

def test_high_concurrency_writes(live_server):
    """Tests concurrent memory-mapped DB locks/writes."""
    def register_guest():
        try:
            return requests.post(f"{live_server}/api/auth/guest", timeout=2).status_code
        except requests.RequestException:
            return 0

    with concurrent.futures.ThreadPoolExecutor(max_workers=25) as executor:
        results = list(executor.map(lambda _: register_guest(), range(100)))

    assert all(code == 200 for code in results), "Database corruption or lock contention occurred"
