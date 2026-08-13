#!/bin/sh

# ZEN Update Script
# Pulls latest code and restarts the relay service
# Usage: ./update.sh [OPTIONS]

# Guard: copy self to tmpfile so sh doesn't re-read a modified script
# when git pull replaces this file mid-execution (classic race condition).
case "$0" in
    /tmp/zen-update.*)
        # already running from tmpfile — proceed normally
        ;;
    *)
        _tmpf=$(mktemp /tmp/zen-update.XXXXXX)
        cp "$0" "$_tmpf"
        chmod +x "$_tmpf"
        exec sh "$_tmpf" "$@"
        ;;
esac

set -eu

SERVICE_NAME="zen"
INSTALL_DIR="$HOME/zen"
VERSION="main"
DRY_RUN=false

log_info()  { printf '\033[0;32m[INFO]\033[0m %s\n' "$1"; }
log_warn()  { printf '\033[1;33m[WARN]\033[0m %s\n' "$1"; }
log_error() { printf '\033[0;31m[ERROR]\033[0m %s\n' "$1"; }

run() {
    if [ "$DRY_RUN" = "true" ]; then
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

while [ $# -gt 0 ]; do
    case "$1" in
        -d|--dir)       INSTALL_DIR="$2"; shift 2 ;;
        -s|--service)   SERVICE_NAME="$2"; shift 2 ;;
        -v|--version)   VERSION="$2"; shift 2 ;;
        --dry-run)      DRY_RUN=true; shift ;;
        -h|--help)      show_help; exit 0 ;;
        *) log_error "Unknown option: $1"; show_help; exit 1 ;;
    esac
done

# Under `sudo` every path this script works from points at the wrong place:
# INSTALL_DIR is "$HOME/zen", nvm lives in "$HOME/.nvm", the state stamp is
# under "$HOME/.local/state" -- and $HOME is /root. It would look for a checkout
# that is not there and install a launcher nobody runs. Root is only ever needed
# for the one `systemctl restart`, which this script asks for itself.
if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
    log_error "Run 'zen update' as ${SUDO_USER}, not with sudo."
    log_error "It updates the checkout in that user's home; root only enters for"
    log_error "the service restart, which this script requests on its own."
    exit 1
fi

if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    command -v sudo >/dev/null 2>&1 || { log_error "sudo required"; exit 1; }
    SUDO="sudo -n"  # non-interactive: fail fast if no sudoers rule instead of hanging
fi

if [ ! -d "$INSTALL_DIR/.git" ]; then
    log_error "ZEN not found at $INSTALL_DIR. Run install.sh first."
    exit 1
fi

# Capture current commit for rollback
PREV_COMMIT=$(git -C "$INSTALL_DIR" rev-parse HEAD)

# Stamp file records the commit the SERVICE was last (re)started at — not the
# commit git last pulled. We restart when the running service is behind the
# working tree (HEAD != stamp), so a tree advanced out-of-band (a manual pull,
# a push from this very checkout) still triggers a restart on the next run,
# instead of being masked by a no-op "already up to date" pull.
STATE_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/zen"
STAMP="$STATE_DIR/deployed-commit-${SERVICE_NAME}"
mkdir -p "$STATE_DIR" 2>/dev/null || true
DEPLOYED_COMMIT=$(cat "$STAMP" 2>/dev/null || echo "")

rollback() {
    log_warn "Update failed, rolling back to $PREV_COMMIT..."
    git -C "$INSTALL_DIR" checkout "$PREV_COMMIT" || true
    $SUDO systemctl restart "$SERVICE_NAME" 2>/dev/null || true
    log_error "Rolled back. Service restarted."
    exit 1
}

restart_service() {
    if [ -f "/etc/systemd/system/${SERVICE_NAME}.service" ]; then
        run $SUDO systemctl restart "$SERVICE_NAME"
        if [ "$DRY_RUN" != "true" ]; then
            sleep 2
            if systemctl is-active --quiet "$SERVICE_NAME"; then
                log_info "Service restarted successfully"
                # Record the commit now running so future runs can detect drift.
                git -C "$INSTALL_DIR" rev-parse HEAD > "$STAMP" 2>/dev/null || true
            else
                log_warn "Service may have failed. Check: journalctl -u $SERVICE_NAME -n 50"
            fi
        fi
    else
        log_warn "Service '$SERVICE_NAME' not found — skipping restart"
    fi
}

# The systemd units live in the repo, but a host that installed months ago is
# still running whatever it got then: fixing a unit here does not rewrite it
# there. That is how this relay ended up with an auto-update timer whose next
# firing was `infinity` -- active, enabled, and silently never running again.
# Nothing errors when a deploy simply does not happen, so say it out loud.
check_units() {
    tmr="/etc/systemd/system/${SERVICE_NAME}-update.timer"
    src="$INSTALL_DIR/script/zen-update.timer"
    [ -f "$tmr" ] && [ -f "$src" ] || return 0
    want=$(sed -e "s|__ZEN_SERVICE__|$SERVICE_NAME|g" "$src")
    have=$(cat "$tmr")
    if [ "$want" != "$have" ]; then
        log_warn "The installed auto-update timer differs from the one in this checkout."
        log_warn "Updates will keep working only if that timer still fires. To refresh it:"
        log_warn "  sudo cp $src $tmr && sudo sed -i 's|__ZEN_SERVICE__|$SERVICE_NAME|g' $tmr"
        log_warn "  sudo systemctl daemon-reload && sudo systemctl restart ${SERVICE_NAME}-update.timer"
    fi
    # systemd reports the next firing twice: on the wall clock and on the
    # monotonic clock. A timer with neither is enabled, active, and dead.
    # Empty, 0, n/a and infinity all mean "nothing scheduled" here.
    rt=$(systemctl show "${SERVICE_NAME}-update.timer" -p NextElapseUSecRealtime --value 2>/dev/null)
    mono=$(systemctl show "${SERVICE_NAME}-update.timer" -p NextElapseUSecMonotonic --value 2>/dev/null)
    case "$rt" in "" | 0 | n/a | infinity) rt_dead=1 ;; *) rt_dead=0 ;; esac
    case "$mono" in "" | 0 | n/a | infinity) mono_dead=1 ;; *) mono_dead=0 ;; esac
    if [ "$rt_dead" = 1 ] && [ "$mono_dead" = 1 ]; then
        log_warn "The auto-update timer has no next firing scheduled: it is enabled, active, and dead."
        log_warn "Until it is refreshed, every deploy needs 'zen update' by hand."
    fi
}

log_info "Updating ZEN at $INSTALL_DIR..."
log_info "  Branch:  $VERSION"
log_info "  Service: $SERVICE_NAME"

# Pull latest code
run git -C "$INSTALL_DIR" fetch origin
run git -C "$INSTALL_DIR" checkout "$VERSION"
run git -C "$INSTALL_DIR" pull origin "$VERSION"

NEW_COMMIT=$(git -C "$INSTALL_DIR" rev-parse HEAD)

# Restart when the running service is behind the working tree, regardless of
# whether this run's pull changed anything.
# Check the deploy machinery before the early exit below -- "already deployed"
# is the common path, and a timer that stopped firing is exactly what makes
# every run land there forever.
check_units

if [ "$DEPLOYED_COMMIT" = "$NEW_COMMIT" ] && systemctl is-active --quiet "$SERVICE_NAME"; then
    log_info "Already deployed ($(git -C "$INSTALL_DIR" log -1 --format='%h %s'))"
    log_info "  Service already running this commit; skipping restart."
    log_info "ZEN update check completed!"
    log_info "  Logs: journalctl -u $SERVICE_NAME -f"
    exit 0
fi

if [ "$PREV_COMMIT" != "$NEW_COMMIT" ]; then
    PREV_SHORT=$(printf '%.7s' "$PREV_COMMIT")
    NEW_SHORT=$(printf '%.7s' "$NEW_COMMIT")
    log_info "Updated: $PREV_SHORT → $NEW_SHORT"
else
    log_info "Working tree unchanged, but service is behind — redeploying $(printf '%.7s' "$NEW_COMMIT")"
fi
log_info "$(git -C "$INSTALL_DIR" log -1 --format='  %s (%cr)' HEAD)"

# Resolve npm: system PATH first, then nvm default, then any nvm version
NPM=$(command -v npm 2>/dev/null || true)
if [ -z "$NPM" ] && [ -s "$HOME/.nvm/nvm.sh" ]; then
    # shellcheck source=/dev/null
    . "$HOME/.nvm/nvm.sh" 2>/dev/null || true
    NPM=$(command -v npm 2>/dev/null || true)
fi
if [ -z "$NPM" ]; then
    log_warn "npm not found — skipping dependency install"
else
    # Install/update dependencies
    run "$NPM" --prefix "$INSTALL_DIR" install --omit=dev
fi

# Re-install CLI binary so new commands (start/stop/restart/logs) are available
if [ -f "$INSTALL_DIR/script/zen.sh" ]; then
    # Update whichever location the binary was originally installed to
    if [ -f "$HOME/.local/bin/zen" ]; then
        run cp "$INSTALL_DIR/script/zen.sh" "$HOME/.local/bin/zen"
        run chmod +x "$HOME/.local/bin/zen"
        log_info "zen CLI updated (~/.local/bin/zen)"
    fi
    if [ -f "/usr/local/bin/zen" ]; then
        run $SUDO cp "$INSTALL_DIR/script/zen.sh" /usr/local/bin/zen
        run $SUDO chmod +x /usr/local/bin/zen
        log_info "zen CLI updated (/usr/local/bin/zen)"
    fi
fi


# Restart service
restart_service

log_info "ZEN update completed!"
log_info "  Logs: journalctl -u $SERVICE_NAME -f"
