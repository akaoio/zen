#!/bin/bash

# ZEN Update Script
# Pulls latest code and restarts the relay service
# Usage: ./update.sh [OPTIONS]

set -euo pipefail

SERVICE_NAME="relay"
INSTALL_DIR="$HOME/zen"
VERSION="main"
DRY_RUN=false

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

run() {
    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] $*"
        return 0
    fi
    log_info "Running: $*"
    "$@"
}

show_help() {
    cat << EOF
ZEN Update Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -d, --dir DIRECTORY     Installation directory (default: ~/zen)
    -s, --service NAME      Systemd service name (default: relay)
    -v, --version VERSION   Branch/tag to update to (default: main)
    --dry-run               Show what would be done without executing
    -h, --help              Show this help message

EXAMPLES:
    $0
    $0 --dir /opt/zen --service relay
    $0 --version v1.2.0

EOF
}

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--dir)       INSTALL_DIR="$2"; shift 2 ;;
        -s|--service)   SERVICE_NAME="$2"; shift 2 ;;
        -v|--version)   VERSION="$2"; shift 2 ;;
        --dry-run)      DRY_RUN=true; shift ;;
        -h|--help)      show_help; exit 0 ;;
        *) log_error "Unknown option: $1"; show_help; exit 1 ;;
    esac
done

if [[ $EUID -eq 0 ]]; then
    SUDO=""
else
    command -v sudo &>/dev/null || { log_error "sudo required"; exit 1; }
    SUDO="sudo"
fi

if [[ ! -d "$INSTALL_DIR/.git" ]]; then
    log_error "ZEN not found at $INSTALL_DIR. Run install.sh first."
    exit 1
fi

# Capture current commit for rollback
PREV_COMMIT=$(git -C "$INSTALL_DIR" rev-parse HEAD)

rollback() {
    log_warn "Update failed, rolling back to $PREV_COMMIT..."
    git -C "$INSTALL_DIR" checkout "$PREV_COMMIT" || true
    $SUDO systemctl restart "$SERVICE_NAME" 2>/dev/null || true
    log_error "Rolled back. Service restarted."
    exit 1
}

restart_service() {
    if [[ -f "/etc/systemd/system/${SERVICE_NAME}.service" ]]; then
        run $SUDO systemctl restart "$SERVICE_NAME"
        if [[ "$DRY_RUN" != "true" ]]; then
            sleep 2
            if systemctl is-active --quiet "$SERVICE_NAME"; then
                log_info "Service restarted successfully"
            else
                log_warn "Service may have failed. Check: journalctl -u $SERVICE_NAME -n 50"
            fi
        fi
    else
        log_warn "Service '$SERVICE_NAME' not found — skipping restart"
    fi
}

trap rollback ERR

log_info "Updating ZEN at $INSTALL_DIR..."
log_info "  Branch:  $VERSION"
log_info "  Service: $SERVICE_NAME"

# Pull latest code
run git -C "$INSTALL_DIR" fetch origin
run git -C "$INSTALL_DIR" checkout "$VERSION"
run git -C "$INSTALL_DIR" pull origin "$VERSION"

NEW_COMMIT=$(git -C "$INSTALL_DIR" rev-parse HEAD)

if [[ "$PREV_COMMIT" == "$NEW_COMMIT" ]]; then
    log_info "Already up to date ($(git -C "$INSTALL_DIR" log -1 --format='%h %s'))"
    log_info "  No code changes; skipping restart."
    trap - ERR
    log_info "ZEN update check completed!"
    log_info "  Logs: journalctl -u $SERVICE_NAME -f"
    exit 0
fi

log_info "Updated: ${PREV_COMMIT:0:7} → ${NEW_COMMIT:0:7}"
log_info "$(git -C "$INSTALL_DIR" log -1 --format='  %s (%cr)' HEAD)"

# Install/update dependencies
run npm --prefix "$INSTALL_DIR" install --omit=dev

# Re-install CLI binary so new commands (start/stop/restart/logs) are available
if [[ -f "$INSTALL_DIR/script/zen.sh" ]]; then
    run $SUDO cp "$INSTALL_DIR/script/zen.sh" /usr/local/bin/zen
    run $SUDO chmod +x /usr/local/bin/zen
    log_info "zen CLI updated"
fi

# Restart service
restart_service

trap - ERR

log_info "ZEN update completed!"
log_info "  Logs: journalctl -u $SERVICE_NAME -f"
