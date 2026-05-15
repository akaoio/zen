#!/bin/sh
# zen - ZEN Entropy Network CLI
# POSIX / XDG Base Directory Specification compliant
# https://specifications.freedesktop.org/basedir-spec/latest/

set -eu

# ── XDG Base Directory defaults ──────────────────────────────────────────────
XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
XDG_DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
XDG_STATE_HOME="${XDG_STATE_HOME:-$HOME/.local/state}"

ZEN_CONFIG_DIR="$XDG_CONFIG_HOME/zen"
ZEN_DATA_DIR="$XDG_DATA_HOME/zen"
ZEN_STATE_DIR="$XDG_STATE_HOME/zen"

# ── Terminal colours ──────────────────────────────────────────────────────────
if [ -t 1 ]; then
    RED=$(printf '\033[0;31m')
    GREEN=$(printf '\033[0;32m')
    YELLOW=$(printf '\033[1;33m')
    CYAN=$(printf '\033[0;36m')
    BOLD=$(printf '\033[1m')
    NC=$(printf '\033[0m')
else
    RED=''
    GREEN=''
    YELLOW=''
    CYAN=''
    BOLD=''
    NC=''
fi

err()  { printf '%s[error]%s %s\n' "$RED" "$NC" "$*" >&2; }
info() { printf '%s[info]%s %s\n' "$GREEN" "$NC" "$*"; }

# ── Locate install directory ──────────────────────────────────────────────────
find_install_dir() {
    local f
    f="$ZEN_CONFIG_DIR/install_dir"
    if [ -f "$f" ]; then
        local d
        d=$(cat "$f")
        if [ -d "$d/script" ]; then
            printf '%s\n' "$d"
            return 0
        fi
    fi
    # Fallback: common locations
    for d in "$HOME/zen" "/opt/zen"; do
        if [ -d "$d/script" ]; then
            printf '%s\n' "$d"
            return 0
        fi
    done
}

# ── zen status ────────────────────────────────────────────────────────────────
cmd_status() {
    local install_dir service_name node_ver
    local sep

    install_dir=$(find_install_dir || true)
    service_name=$(get_service_name)
    sep="────────────────────────────────"

    printf '%sZEN%s\n' "$BOLD" "$NC"
    printf '%s\n' "$sep"
    if [ -n "$install_dir" ] && [ -f "$install_dir/package.json" ]; then
        local ver
        ver=$(node -e "process.stdout.write(require('$install_dir/package.json').version)" 2>/dev/null || echo "unknown")
        printf '  version    %s%s%s\n' "$CYAN" "$ver" "$NC"
        printf '%s\n' "  install    $install_dir"
    else
        printf '  %sZEN not found — run: curl -fsSL https://raw.githubusercontent.com/akaoio/zen/main/script/install.sh | bash%s\n' "$YELLOW" "$NC"
    fi
    node_ver=$(node --version 2>/dev/null || echo "not installed")
    printf '%s\n' "  node.js    $node_ver"

    printf '\n'
    printf '%sService  (%s)%s\n' "$BOLD" "$service_name" "$NC"
    printf '%s\n' "$sep"
    if command -v systemctl >/dev/null 2>&1 && systemctl is-active --quiet "$service_name" 2>/dev/null; then
        local active
        local col
        local elapsed
        local port

        active=$(systemctl is-active "$service_name" 2>/dev/null || echo "unknown")
        col=$GREEN
        if [ "$active" != "active" ]; then
            col=$RED
        fi
        printf '  status     %s%s%s\n' "$col" "$active" "$NC"

        elapsed=$(systemctl show "$service_name" --property=ActiveEnterTimestamp 2>/dev/null \
            | sed 's/ActiveEnterTimestamp=//' | grep -v '^$' || true)
        if [ -n "$elapsed" ]; then
            printf '%s\n' "  since      $elapsed"
        fi

        port=$(systemctl show "$service_name" --property=Environment 2>/dev/null \
            | grep -oP 'PORT=\K[0-9]+' || true)
        if [ -n "$port" ]; then
            printf '%s\n' "  port       $port"
        fi
    else
        printf '  %sservice not found%s  (not a systemd host, or service not installed)\n' "$YELLOW" "$NC"
    fi

    printf '\n'
    printf '%sAuto-update  (%s-update.timer)%s\n' "$BOLD" "$service_name" "$NC"
    printf '%s\n' "$sep"
    if command -v systemctl >/dev/null 2>&1 && systemctl list-unit-files --type=timer 2>/dev/null | grep -q "${service_name}-update.timer"; then
        local tmr_active
        local col
        local next
        local last

        tmr_active=$(systemctl is-active "${service_name}-update.timer" 2>/dev/null || echo "inactive")
        col=$GREEN
        if [ "$tmr_active" != "active" ]; then
            col=$YELLOW
        fi
        printf '  status     %s%s%s\n' "$col" "$tmr_active" "$NC"

        next=$(systemctl show "${service_name}-update.timer" --property=NextElapseUSecRealtime 2>/dev/null \
            | sed 's/NextElapseUSecRealtime=//' | grep -v '^$' || true)
        last=$(systemctl show "${service_name}-update.service" --property=ExecMainExitTimestamp 2>/dev/null \
            | sed 's/ExecMainExitTimestamp=//' | grep -v '^$\|^0$' || true)
        if [ -n "$next" ] && [ "$next" != "0" ]; then
            printf '%s\n' "  next       $next"
        fi
        if [ -n "$last" ]; then
            printf '%s\n' "  last run   $last"
        fi
    else
        printf '  %stimer not installed%s  (re-run install.sh to enable)\n' "$YELLOW" "$NC"
    fi

    printf '\n'
    printf '%sPaths  (XDG)%s\n' "$BOLD" "$NC"
    printf '%s\n' "$sep"
    printf '%s\n' "  data       $ZEN_DATA_DIR"
    printf '%s\n' "  state      $ZEN_STATE_DIR"

    local cert
    cert="$ZEN_CONFIG_DIR/cert.pem"
    if [ -f "$cert" ]; then
        local expiry

        printf '\n'
        printf '%sSSL%s\n' "$BOLD" "$NC"
        printf '%s\n' "$sep"
        printf '%s\n' "  cert       $cert"
        expiry=$(openssl x509 -enddate -noout -in "$cert" 2>/dev/null \
            | sed 's/notAfter=//' || echo "unknown")
        printf '%s\n' "  expires    $expiry"
    fi
}

# ── zen update ────────────────────────────────────────────────────────────────
cmd_update() {
    local install_dir

    install_dir=$(find_install_dir || true)
    if [ -z "$install_dir" ]; then
        err "ZEN install directory not found."
        err "Re-run: curl -fsSL https://raw.githubusercontent.com/akaoio/zen/main/script/install.sh | bash"
        exit 1
    fi
    exec "$install_dir/script/update.sh" "$@"
}

# ── service helpers ───────────────────────────────────────────────────────────
get_service_name() {
    local svc

    svc=''
    if [ -f "$ZEN_CONFIG_DIR/service_name" ]; then
        svc=$(cat "$ZEN_CONFIG_DIR/service_name")
    fi
    printf '%s\n' "${svc:-zen}"
}

ensure_systemd() {
    if ! command -v systemctl >/dev/null 2>&1; then
        err "systemd not available on this host"
        exit 1
    fi
}

get_sudo() {
    if [ "$(id -u)" -eq 0 ]; then
        printf '\n'
        return 0
    fi
    if command -v sudo >/dev/null 2>&1; then
        printf '%s\n' "sudo"
        return 0
    fi
    err "sudo is required but not available"
    exit 1
}

# ── zen start ─────────────────────────────────────────────────────────────────
cmd_start() {
    local svc SUDO

    ensure_systemd
    svc=$(get_service_name)
    SUDO=$(get_sudo)
    info "Starting $svc…"
    $SUDO systemctl start "$svc"
    if systemctl is-active --quiet "$svc"; then
        info "Service ${BOLD}$svc${NC} is ${GREEN}active${NC}"
    else
        err "Service '$svc' did not start"
        exit 1
    fi
}

# ── zen stop ──────────────────────────────────────────────────────────────────
cmd_stop() {
    local svc SUDO

    ensure_systemd
    svc=$(get_service_name)
    SUDO=$(get_sudo)
    info "Stopping $svc…"
    $SUDO systemctl stop "$svc"
    if ! systemctl is-active --quiet "$svc" 2>/dev/null; then
        info "Service ${BOLD}$svc${NC} is ${YELLOW}stopped${NC}"
    else
        err "Service '$svc' did not stop"
        exit 1
    fi
}

# ── zen restart ───────────────────────────────────────────────────────────────
cmd_restart() {
    local svc SUDO

    ensure_systemd
    svc=$(get_service_name)
    SUDO=$(get_sudo)
    info "Restarting $svc…"
    $SUDO systemctl restart "$svc"
    if systemctl is-active --quiet "$svc"; then
        info "Service ${BOLD}$svc${NC} restarted — ${GREEN}active${NC}"
    else
        err "Service '$svc' failed to restart"
        exit 1
    fi
}

# ── zen logs ──────────────────────────────────────────────────────────────────
cmd_logs() {
    local svc

    ensure_systemd
    svc=$(get_service_name)
    exec journalctl -u "$svc" -f "$@"
}

# ── zen uninstall ─────────────────────────────────────────────────────────────
cmd_uninstall() {
    local install_dir

    install_dir=$(find_install_dir || true)
    if [ -z "$install_dir" ]; then
        err "ZEN install directory not found."
        exit 1
    fi
    exec "$install_dir/script/uninstall.sh" "$@"
}

# ── help ──────────────────────────────────────────────────────────────────────
show_help() {
    cat << EOF
${BOLD}zen${NC} — ZEN Entropy Network CLI

${BOLD}USAGE${NC}
    zen <command> [options]

${BOLD}COMMANDS${NC}
    status      Show relay status, service state, and XDG paths
    start       Start the relay service
    stop        Stop the relay service
    restart     Restart the relay service
    logs        Follow relay service logs  (passes args to journalctl -f)
    update      Pull latest code and restart service
    uninstall   Remove ZEN from this system
    help        Show this message

${BOLD}XDG PATHS${NC}
    Config   \$XDG_CONFIG_HOME/zen   (default: ~/.config/zen)
    Data     \$XDG_DATA_HOME/zen     (default: ~/.local/share/zen)
    State    \$XDG_STATE_HOME/zen    (default: ~/.local/state/zen)

${BOLD}INSTALL / REINSTALL${NC}
    curl -fsSL https://raw.githubusercontent.com/akaoio/zen/main/script/install.sh | bash
EOF
}

# ── dispatch ──────────────────────────────────────────────────────────────────
cmd="${1:-help}"
if [ "$#" -ge 1 ]; then shift; fi
case "$cmd" in
    status)              cmd_status "$@" ;;
    start)               cmd_start "$@" ;;
    stop)                cmd_stop "$@" ;;
    restart)             cmd_restart "$@" ;;
    logs)                cmd_logs "$@" ;;
    update)              cmd_update "$@" ;;
    uninstall)           cmd_uninstall "$@" ;;
    help|-h|--help)      show_help ;;
    *) err "Unknown command: $cmd"; show_help; exit 1 ;;
esac
