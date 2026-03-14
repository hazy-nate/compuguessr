#!/bin/bash

set -e

BLUE='\033[34m'
BOLD='\033[1m'
RESET='\033[0m'

printf "${BLUE}${BOLD}[*] Starting dependency installation...${RESET}\n"

if [ -f /etc/debian_version ]; then
    PKGMGR="apt-get"
    UPDATE_CMD="sudo apt-get update"
    INSTALL_CMD="sudo apt-get install -y"
elif [ -f /etc/redhat-release ]; then
    PKGMGR="dnf"
    UPDATE_CMD="sudo dnf check-update"
    INSTALL_CMD="sudo dnf install -y"
else
    printf "Unsupported distribution. Please install dependencies manually.\n"
    exit 1
fi

printf "${BLUE}${BOLD}[*] Installing system tools via ${PKGMGR}...${RESET}\n"
$UPDATE_CMD || true
$INSTALL_CMD nasm gcc make lighttpd python3 python3-pip python3-venv

if ! command -v robodoc &> /dev/null; then
    printf "${BLUE}${BOLD}[*] Robodoc not found. Attempting install...${RESET}\n"
    $INSTALL_CMD robodoc || printf "Note: robodoc might require manual compilation on this distro.\n"
fi

printf "${BLUE}${BOLD}[*] Setting up Python virtual environment...${RESET}\n"
if [ ! -d "venv" ]; then
    python3 -m venv venv
fi

source venv/bin/activate
pip install --upgrade pip
pip install sphinx sphinx-rtd-theme

printf "${BLUE}${BOLD}[*] Environment setup complete!${RESET}\n"

