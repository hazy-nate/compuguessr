# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import os
import subprocess
import time
from pathlib import Path
import pytest
import textwrap

def pytest_addoption(parser):
    """Adds custom command line arguments to pytest."""
    parser.addoption(
        "--port", action="store", default="8080", help="Port for the E2E test server"
    )

@pytest.fixture(scope="session")
def server_port(pytestconfig):
    """Retrieves the port from the command line configuration."""
    return pytestconfig.getoption("port")

@pytest.fixture(scope="session")
def server_setup(tmp_path_factory, server_port):
    """Dynamically generates a lighttpd config and sets up paths."""
    tmp_path = tmp_path_factory.mktemp("server")
    bin_path = Path.cwd() / "bin/debug/compuguessr"
    sock_path = tmp_path / "fcgi.sock"

    if not bin_path.exists():
        pytest.fail(f"Executable not found at {bin_path}. Run 'make debug'.")

    # Lighttpd config using the dynamic server_port
    config_content = textwrap.dedent(f"""\
        server.modules       += ( "mod_fastcgi" )
        server.port           = {server_port}
        server.document-root  = "{Path.cwd()}"
        server.bind           = "127.0.0.1"

        fastcgi.server = (
            "/" => ((
                "socket"      => "{sock_path}",
                "check-local" => "disable"
            ))
        )
    """)

    conf_file = tmp_path / "lighttpd.conf"
    conf_file.write_text(config_content)
    return bin_path, sock_path, conf_file

@pytest.fixture(scope="session")
def live_server(server_setup, server_port):
    """Spawns the C daemon and Lighttpd, managing their lifecycles."""
    bin_path, sock_path, conf_file = server_setup

    # 1. Spawn CompuGuessr
    env = os.environ.copy()
    env["FASTCGI_SOCK"] = str(sock_path)

    cg_proc = subprocess.Popen(
        [str(bin_path)],
        env=env,
        cwd=str(Path.cwd())
    )

    # Wait for the socket file to appear
    for _ in range(50):
        if sock_path.exists():
            break
        time.sleep(0.1)

    if not sock_path.exists():
        cg_proc.terminate()
        pytest.fail("CompuGuessr failed to create FASTCGI_SOCK.")

    # 2. Spawn Lighttpd
    lhttpd_proc = subprocess.Popen(["lighttpd", "-D", "-f", str(conf_file)])

    # Wait for Lighttpd to bind to the dynamic port
    time.sleep(0.5)

    # Yield the base URL with the dynamic port
    yield f"http://127.0.0.1:{server_port}"

    # 3. Teardown
    lhttpd_proc.terminate()
    lhttpd_proc.wait()
    cg_proc.terminate()
    cg_proc.wait()
