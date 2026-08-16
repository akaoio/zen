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

# ── zen doctor ────────────────────────────────────────────────────────────────
# Everything here answers one question: is what is installed still what this
# checkout says it should be? Nothing on this machine errors when the answer is
# no. The auto-update timer sat enabled, active and dead for eight days without
# a single log line, and every deploy in that window quietly needed a human.
DOCTOR_BAD=0
DOCTOR_DEEP=0
DOCTOR_FIX=0

doc_ok()   { printf '  %s✓%s %-20s %s\n' "$GREEN" "$NC" "$1" "$2"; }
doc_warn() { printf '  %s!%s %-20s %s\n' "$YELLOW" "$NC" "$1" "$2"; DOCTOR_BAD=1; }
doc_bad()  { printf '  %s✗%s %-20s %s\n' "$RED" "$NC" "$1" "$2"; DOCTOR_BAD=1; }
doc_fix()  { printf '      %s%s%s\n' "$CYAN" "$1" "$NC"; }

# systemd reports the next firing twice: on the wall clock and on the monotonic
# clock. A timer with neither is enabled, active, and dead.
doctor_timer_dead() {
    local rt mono
    rt=$(systemctl show "$1" -p NextElapseUSecRealtime --value 2>/dev/null || echo "")
    mono=$(systemctl show "$1" -p NextElapseUSecMonotonic --value 2>/dev/null || echo "")
    case "$rt" in "" | 0 | n/a | infinity) ;; *) return 1 ;; esac
    case "$mono" in "" | 0 | n/a | infinity) ;; *) return 1 ;; esac
    return 0
}

cmd_doctor() {
    local install_dir svc ver node_bin exec_start head stamp behind sep
    local unit tmpl want have free data_dir

    for a in "$@"; do
        case "$a" in
            --deep) DOCTOR_DEEP=1 ;;
            --fix)  DOCTOR_FIX=1 ;;
            *) err "Unknown option: $a"; exit 1 ;;
        esac
    done

    install_dir=$(find_install_dir || true)
    svc=$(get_service_name)
    sep="────────────────────────────────"
    printf '%sZEN doctor%s\n%s\n' "$BOLD" "$NC" "$sep"

    # ── install ───────────────────────────────────────────────────────────────
    if [ -z "$install_dir" ]; then
        doc_bad "install" "not found"
        doc_fix "curl -fsSL https://raw.githubusercontent.com/akaoio/zen/main/script/install.sh | bash"
        exit 1
    fi
    ver=$(node -e "process.stdout.write(require('$install_dir/package.json').version)" 2>/dev/null || echo "?")
    doc_ok "install" "$install_dir (v$ver)"

    node_bin=$(command -v node 2>/dev/null || true)
    if [ -z "$node_bin" ]; then
        doc_bad "node" "not on PATH"
    else
        doc_ok "node" "$($node_bin --version) at $node_bin"
    fi

    if ! command -v systemctl >/dev/null 2>&1; then
        doc_warn "systemd" "not available — service checks skipped"
        doctor_summary "$sep"
        return $?
    fi

    # ── the relay ─────────────────────────────────────────────────────────────
    if systemctl is-active --quiet "$svc" 2>/dev/null; then
        doc_ok "service" "$svc active"
    else
        doc_bad "service" "$svc $(systemctl is-active "$svc" 2>/dev/null || echo 'not installed')"
        doc_fix "zen start"
    fi

    # A unit left behind by an older install can still be running happily from a
    # directory this checkout knows nothing about.
    exec_start=$(systemctl show "$svc" -p ExecStart --value 2>/dev/null | sed -n 's/.*path=\([^ ;]*\).*/\1/p' || true)
    case "$(systemctl show "$svc" -p ExecStart --value 2>/dev/null)" in
        *"$install_dir"*) doc_ok "service points at" "$install_dir" ;;
        "") doc_warn "service points at" "unknown" ;;
        *) doc_bad "service points at" "somewhere else, not $install_dir"
           doc_fix "re-run install.sh, or check ExecStart in /etc/systemd/system/$svc.service" ;;
    esac
    if [ -n "$exec_start" ] && [ ! -x "$exec_start" ]; then
        doc_bad "service node" "$exec_start is gone"
        doc_fix "re-run install.sh to point the unit at the node you have now"
    fi

    # ── is the relay running this checkout? ───────────────────────────────────
    head=$(git -C "$install_dir" rev-parse HEAD 2>/dev/null || echo "")
    stamp=$(cat "$ZEN_STATE_DIR/deployed-commit-${svc}" 2>/dev/null || echo "")
    if [ -z "$head" ]; then
        doc_warn "checkout" "not a git checkout"
    elif [ -z "$stamp" ]; then
        doc_warn "deployed" "no record of what was deployed"
        doc_fix "zen update"
    elif [ "$head" = "$stamp" ]; then
        doc_ok "deployed" "$(printf '%.7s' "$head") — the running service is this commit"
    else
        doc_bad "deployed" "service is on $(printf '%.7s' "$stamp"), checkout is $(printf '%.7s' "$head")"
        doc_fix "zen update"
    fi

    # ── is this checkout behind its upstream? ─────────────────────────────────
    if [ -n "$head" ]; then
        if timeout 20 git -C "$install_dir" fetch --quiet origin 2>/dev/null; then
            behind=$(git -C "$install_dir" rev-list --count "HEAD..@{upstream}" 2>/dev/null || echo "?")
            if [ "$behind" = "0" ]; then
                doc_ok "upstream" "up to date"
            elif [ "$behind" = "?" ]; then
                doc_warn "upstream" "no upstream branch set"
            else
                doc_bad "upstream" "$behind commit(s) behind"
                doc_fix "zen update"
            fi
        else
            doc_warn "upstream" "could not reach origin (offline?)"
        fi
    fi

    # ── the machinery that is supposed to do all this without me ─────────────
    if systemctl list-unit-files --type=timer 2>/dev/null | grep -q "${svc}-update.timer"; then
        if doctor_timer_dead "${svc}-update.timer"; then
            doc_bad "auto-update" "timer has no next firing: enabled, active, and dead"
            doc_fix "zen doctor --fix   (or re-run install.sh)"
        else
            doc_ok "auto-update" "next $(systemctl show "${svc}-update.timer" -p NextElapseUSecRealtime --value 2>/dev/null)"
        fi
    else
        doc_bad "auto-update" "timer not installed"
        doc_fix "re-run install.sh"
    fi

    # The units this checkout owns as templates. zen.service is not among them:
    # install.sh builds that one inline, from options this command cannot know.
    for unit in "${svc}-update.service" "${svc}-update.timer"; do
        tmpl="$install_dir/script/$(printf '%s' "$unit" | sed "s|^${svc}|zen|")"
        [ -f "$tmpl" ] && [ -f "/etc/systemd/system/$unit" ] || continue
        want=$(sed -e "s|__ZEN_USER__|$(id -un)|g" -e "s|__ZEN_DIR__|$install_dir|g" -e "s|__ZEN_SERVICE__|$svc|g" "$tmpl")
        have=$(cat "/etc/systemd/system/$unit")
        if [ "$want" = "$have" ]; then
            doc_ok "unit $unit" "matches this checkout"
        else
            doc_bad "unit $unit" "differs from this checkout"
            doc_fix "zen doctor --fix"
        fi
    done

    # ── where the data lives ─────────────────────────────────────────────────
    data_dir="$ZEN_DATA_DIR/radata"
    if [ -d "$data_dir" ]; then
        if [ -w "$data_dir" ]; then
            free=$(df -Pm "$data_dir" 2>/dev/null | awk 'NR==2 {print $4}')
            if [ -n "$free" ] && [ "$free" -lt 200 ]; then
                doc_bad "disk" "${free}MB free where the graph is written"
            else
                doc_ok "disk" "${free}MB free at $data_dir"
            fi
        else
            doc_bad "data" "$data_dir is not writable"
        fi
    else
        doc_warn "data" "$data_dir does not exist yet"
    fi

    # ── the store itself, only when asked: this reads every key back ─────────
    if [ "$DOCTOR_DEEP" = 1 ]; then
        printf '  %s…%s %-20s %s\n' "$CYAN" "$NC" "store" "reading every key back, this takes a while"
        if node "$install_dir/script/radcheck.js" >/dev/null 2>&1; then
            doc_ok "store" "every key on disk reads back"
        else
            doc_bad "store" "some keys do not read back"
            doc_fix "node $install_dir/script/radcheck.js --all"
        fi
    fi

    if [ "$DOCTOR_FIX" = 1 ]; then
        doctor_apply "$install_dir" "$svc"
        return $?
    fi

    doctor_summary "$sep"
    return $?
}

doctor_summary() {
    printf '%s\n' "$1"
    if [ "$DOCTOR_BAD" = 0 ]; then
        info "Nothing to fix."
        return 0
    fi
    printf '%sSomething above needs attention. `zen doctor --fix` handles the unit files;%s\n' "$YELLOW" "$NC"
    printf '%sthe rest are one-liners printed next to each finding.%s\n' "$YELLOW" "$NC"
    return 1
}

# Only the units this checkout owns, rewritten from its own templates. Anything
# a human put in them by hand -- the HTTPS lines ssl.sh adds, for one -- lives in
# zen.service, which this does not touch.
doctor_apply() {
    local install_dir svc SUDO unit tmpl
    install_dir="$1"
    svc="$2"
    SUDO=$(get_sudo)
    printf '%s\n' "────────────────────────────────"
    for unit in "${svc}-update.service" "${svc}-update.timer"; do
        tmpl="$install_dir/script/$(printf '%s' "$unit" | sed "s|^${svc}|zen|")"
        [ -f "$tmpl" ] || continue
        info "Writing /etc/systemd/system/$unit from $tmpl"
        sed -e "s|__ZEN_USER__|$(id -un)|g" -e "s|__ZEN_DIR__|$install_dir|g" -e "s|__ZEN_SERVICE__|$svc|g" \
            "$tmpl" | $SUDO tee "/etc/systemd/system/$unit" > /dev/null
    done
    $SUDO systemctl daemon-reload
    $SUDO systemctl restart "${svc}-update.timer"
    if doctor_timer_dead "${svc}-update.timer"; then
        err "The timer still has no next firing. Look at: systemctl status ${svc}-update.timer"
        return 1
    fi
    info "Auto-update timer fires next at $(systemctl show "${svc}-update.timer" -p NextElapseUSecRealtime --value 2>/dev/null)"
    return 0
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
    doctor      Check that what is installed still matches this checkout
                  --fix    rewrite the auto-update units from this checkout
                  --deep   also read every key in the store back
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
    doctor)              cmd_doctor "$@" ;;
    start)               cmd_start "$@" ;;
    stop)                cmd_stop "$@" ;;
    restart)             cmd_restart "$@" ;;
    logs)                cmd_logs "$@" ;;
    update)              cmd_update "$@" ;;
    uninstall)           cmd_uninstall "$@" ;;
    help|-h|--help)      show_help ;;
    *) err "Unknown command: $cmd"; show_help; exit 1 ;;
esac
