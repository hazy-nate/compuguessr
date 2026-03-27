# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

from pathlib import Path

import pytest
import textwrap

@pytest.fixture
def lighttpd_config(tmp_path: Path):
    """
    Dynamically generates a lighttpd config for testing.
    tmp_path is a built-in pytest fixture that gives each test a unique temp dir.
    """

    bin_path = Path.cwd() / "bin/debug/compuguessr"
    sock_path = tmp_path / "fcgi.sock"

    config_content = textwrap.dedent(f"""\
        server.modules       += ( "mod_fastcgi" )
        server.port           = 8080
        server.document-root  = "/var/www/html"

        fastcgi.server = (
            "/" => ((
                "bin-path"    => "{bin_path}",
                "socket"      => "{sock_path}",
                "max-procs"   => 1,
                "check-local" => "disable"
            ))
        )
    """)

    conf_file = tmp_path / "lighttpd.conf"
    conf_file.write_text(config_content)

    return conf_file
