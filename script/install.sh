#!/bin/sh

# ZEN Installation Script
# Installs Node.js 24, ZEN relay, and sets up a systemd service
# Usage: ./install.sh [OPTIONS]

set -eu

# Default values
VERSION="main"
PORT="8420"
DOMAIN=""
PEERS=""
HTTPS_KEY=""
HTTPS_CERT=""
SERVICE_NAME="zen"
INSTALL_DIR="$HOME/zen"
SKIP_DEPS=false
SKIP_SERVICE=false
DRY_RUN=false
YES=false
NODE_MAJOR=24
# Storage resilience (empty = use built-in defaults)
FMB=""        # free-MB threshold before disk-full warning (default: 200)
FRAT=""       # free/total RAM ratio threshold for OOM eviction (default: 0.10)
EVICT=""      # set to 0 to disable memory+disk eviction entirely
# Graph GC tuning (empty = use built-in defaults)
GC_MB=""      # heap MB threshold for in-memory graph GC (default: 400)
GC_SEC=""     # graph GC interval in seconds (default: 60)
GC_KEEP=""    # seconds to keep recently-written souls (default: 120)
UDP_PORT=""   # UDP multicast port for LAN peer discovery (default: 8421)

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { printf '%b[INFO]%b %s\n' "${GREEN}" "${NC}" "$1"; }
log_warn()  { printf '%b[WARN]%b %s\n' "${YELLOW}" "${NC}" "$1"; }
log_error() { printf '%b[ERROR]%b %s\n' "${RED}" "${NC}" "$1"; }

confirm() {
    if [ "$YES" = "true" ]; then
        return 0
    fi
    response=""
    if [ -e /dev/tty ]; then
        printf '%s [y/N] ' "$1" >/dev/tty
        read -r response </dev/tty
    else
        printf '%s [y/N] ' "$1"
        read -r response
    fi
    case "$response" in
        [Yy]) return 0 ;;
    esac
    return 1
}

# Run a command (with dry-run support). Works for both external commands and shell built-ins.
run() {
    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] $*"
        return 0
    fi
    log_info "Running: $*"
    "$@"
}

show_help() {
    cat << EOF2
ZEN Installation Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -v, --version VERSION       Git branch/tag to checkout (default: main)
    -p, --port PORT             Port number for the server (default: 8420)
    -D, --domain DOMAIN         Public domain name (e.g. peer1.akao.io)
    -P, --peers PEERS           Comma-separated list of peer URLs
    -d, --dir DIRECTORY         Installation directory (default: ~/zen)
    -s, --service NAME          Systemd service name (default: zen)
    --https-key PATH            Path to HTTPS key file
    --https-cert PATH           Path to HTTPS certificate file
    --fmb N                     Free-disk-MB threshold for storage warning (default: 200)
    --frat N                    Free/total RAM ratio for OOM eviction (default: 0.10)
    --evict 0|1                 Enable (1) or disable (0) memory+disk eviction (default: 1)
    --gc-mb N                   Heap MB threshold for in-memory graph GC (default: 400)
    --gc-sec N                  Graph GC interval in seconds (default: 60)
    --gc-keep N                 Seconds to keep recently-written souls in GC (default: 120)
    --udp-port N                UDP multicast port for LAN peer discovery (default: 8421)
    --skip-deps                 Skip Node.js/system dependency installation
    --skip-service              Skip systemd service setup
    --dry-run                   Show what would be done without executing
    -y, --yes                   Auto-confirm all prompts (for non-interactive/CI use)
    -h, --help                  Show this help message

EXAMPLES:
    $0
    $0 --port 443 --https-key ~/.config/zen/key.pem --https-cert ~/.config/zen/cert.pem
    $0 --peers "https://peer1.com/zen,https://peer2.com/zen"
    $0 --skip-service

EOF2
}

# Parse arguments
while [ $# -gt 0 ]; do
    case $1 in
        -v|--version)   VERSION="$2";      shift 2 ;;
        -p|--port)      PORT="$2";         shift 2 ;;
        -D|--domain)    DOMAIN="$2";       shift 2 ;;
        -P|--peers)     PEERS="$2";        shift 2 ;;
        -d|--dir)       INSTALL_DIR="$2";  shift 2 ;;
        -s|--service)   SERVICE_NAME="$2"; shift 2 ;;
        --https-key)    HTTPS_KEY="$2";    shift 2 ;;
        --https-cert)   HTTPS_CERT="$2";   shift 2 ;;
        --fmb)          FMB="$2";          shift 2 ;;
        --frat)         FRAT="$2";         shift 2 ;;
        --evict)        EVICT="$2";        shift 2 ;;
        --gc-mb)        GC_MB="$2";        shift 2 ;;
        --gc-sec)       GC_SEC="$2";       shift 2 ;;
        --gc-keep)      GC_KEEP="$2";      shift 2 ;;
        --udp-port)     UDP_PORT="$2";     shift 2 ;;
        --skip-deps)    SKIP_DEPS=true;      shift ;;
        --skip-service) SKIP_SERVICE=true;   shift ;;
        --dry-run)      DRY_RUN=true;        shift ;;
        -y|--yes)       YES=true;            shift ;;
        -h|--help)      show_help; exit 0 ;;
        *) log_error "Unknown option: $1"; show_help; exit 1 ;;
    esac
done

# Validation
case "$PORT" in
    ''|*[!0-9]*) log_error "Invalid port: $PORT"; exit 1 ;;
esac
if [ "$PORT" -lt 1 ] || [ "$PORT" -gt 65535 ]; then
    log_error "Invalid port: $PORT"; exit 1
fi
case "$SERVICE_NAME" in
    ''|*[!a-zA-Z0-9_-]*) log_error "Invalid service name: $SERVICE_NAME"; exit 1 ;;
esac
case "$INSTALL_DIR" in
    *..*) log_error "Path traversal detected: $INSTALL_DIR"; exit 1 ;;
esac

# Determine sudo
if [ "$(id -u)" -eq 0 ]; then
    SUDO=""
else
    command -v sudo >/dev/null 2>&1 || { log_error "sudo required"; exit 1; }
    SUDO="sudo"
fi

install_dependencies() {
    if [ "$SKIP_DEPS" = "true" ]; then
        log_info "Skipping dependencies"
        return
    fi

    log_info "Installing Node.js $NODE_MAJOR and dependencies..."

    if command -v apt-get >/dev/null 2>&1; then
        run $SUDO apt-get update -y
        run $SUDO apt-get install -y curl git ca-certificates gnupg

        # Install Node.js via NodeSource
        if ! node --version 2>/dev/null | grep -q "^v${NODE_MAJOR}\."; then
            log_info "Installing Node.js $NODE_MAJOR via NodeSource..."
            if [ "$DRY_RUN" != "true" ]; then
                curl -fsSL "https://deb.nodesource.com/setup_${NODE_MAJOR}.x" | $SUDO bash -
            fi
            run $SUDO apt-get install -y nodejs
        fi
    elif command -v dnf >/dev/null 2>&1; then
        run $SUDO dnf install -y curl git nodejs npm
    elif command -v yum >/dev/null 2>&1; then
        run $SUDO yum install -y curl git nodejs npm
    else
        log_error "Unsupported package manager. Install Node.js $NODE_MAJOR, git, curl manually."
        exit 1
    fi

    log_info "Node.js $(node --version), npm $(npm --version)"
}

install_zen() {
    log_info "Installing ZEN to $INSTALL_DIR..."

    run mkdir -p "$(dirname "$INSTALL_DIR")"

    if [ -d "$INSTALL_DIR/.git" ]; then
        log_info "ZEN directory exists, updating..."
        run git -C "$INSTALL_DIR" fetch origin
        run git -C "$INSTALL_DIR" checkout "$VERSION"
        run git -C "$INSTALL_DIR" pull origin "$VERSION" || true
    else
        run git clone https://github.com/akaoio/zen.git "$INSTALL_DIR"
        run git -C "$INSTALL_DIR" checkout "$VERSION"
    fi

    # nvm.sh uses bash syntax — must invoke via bash when npm not in PATH
    if ! command -v npm >/dev/null 2>&1 && [ -s "$HOME/.nvm/nvm.sh" ]; then
        log_info "npm not in PATH, running npm install via bash+nvm..."
        run bash -c ". \"$HOME/.nvm/nvm.sh\" && npm --prefix \"$INSTALL_DIR\" install"
    else
        run npm --prefix "$INSTALL_DIR" install
    fi
    log_info "ZEN installed successfully"
}

create_service() {
    if [ "$SKIP_SERVICE" = "true" ]; then
        log_info "Skipping service creation"
        return
    fi

    log_info "Creating systemd service: $SERVICE_NAME"

    service_file="/etc/systemd/system/${SERVICE_NAME}.service"
    node_bin="$(command -v node)"

    # Build env lines
    env_lines=""
    if [ -n "$PORT" ]; then
        env_lines="${env_lines}Environment=PORT=$PORT
"
    fi
    if [ -n "$DOMAIN" ]; then
        env_lines="${env_lines}Environment=DOMAIN=$DOMAIN
"
    fi
    if [ -n "$PEERS" ]; then
        env_lines="${env_lines}Environment=PEERS=$PEERS
"
    fi
    if [ -n "$HTTPS_KEY" ]; then
        env_lines="${env_lines}Environment=HTTPS_KEY=$HTTPS_KEY
"
    fi
    if [ -n "$HTTPS_CERT" ]; then
        env_lines="${env_lines}Environment=HTTPS_CERT=$HTTPS_CERT
"
    fi
    # Storage resilience
    if [ -n "$FMB" ]; then
        env_lines="${env_lines}Environment=FMB=$FMB
"
    fi
    if [ -n "$FRAT" ]; then
        env_lines="${env_lines}Environment=FRAT=$FRAT
"
    fi
    if [ -n "$EVICT" ]; then
        env_lines="${env_lines}Environment=EVICT=$EVICT
"
    fi
    # Graph GC tuning
    if [ -n "$GC_MB" ]; then
        env_lines="${env_lines}Environment=GRAPH_GC_MB=$GC_MB
"
    fi
    if [ -n "$GC_SEC" ]; then
        env_lines="${env_lines}Environment=GRAPH_GC_SEC=$GC_SEC
"
    fi
    if [ -n "$GC_KEEP" ]; then
        env_lines="${env_lines}Environment=GRAPH_GC_KEEP=$GC_KEEP
"
    fi
    if [ -n "$UDP_PORT" ]; then
        env_lines="${env_lines}Environment=UDP_PORT=$UDP_PORT
"
    fi

    service_content="$(cat << EOF2
[Unit]
Description=ZEN Graph Database Relay Peer
After=network.target
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=always
RestartSec=1
User=$(whoami)
WorkingDirectory=$INSTALL_DIR
ExecStart=$node_bin $INSTALL_DIR/script/server.js
LimitNOFILE=65536
${env_lines}[Install]
WantedBy=multi-user.target
EOF2
)"

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would write $service_file:"
        printf '%s\n' "$service_content"
    else
        printf '%s\n' "$service_content" | $SUDO tee "$service_file" > /dev/null
    fi

    run $SUDO systemctl daemon-reload
    run $SUDO systemctl enable "$SERVICE_NAME"
    log_info "Service created and enabled"
}

configure_limits() {
    log_info "Configuring system limits..."
    if [ "$DRY_RUN" != "true" ]; then
        if ! grep -q "fs.file-max = 999999" /etc/sysctl.conf 2>/dev/null; then
            printf '%s\n' "fs.file-max = 999999" | $SUDO tee -a /etc/sysctl.conf > /dev/null
        fi
        $SUDO sysctl -p /etc/sysctl.conf
        ulimit -n 65536 || log_warn "Could not set ulimit (non-critical)"
    else
        log_info "[DRY RUN] Would configure sysctl and ulimit"
    fi
    log_info "System limits configured"
}

configure_sudoers() {
    if [ "$SKIP_SERVICE" = "true" ]; then
        return
    fi
    if [ "$(id -u)" -eq 0 ]; then
        return
    fi

    sudoers_file="/etc/sudoers.d/${SERVICE_NAME}"
    user="$(whoami)"
    systemctl_bin="$(command -v systemctl)"
    cp_bin="$(command -v cp)"
    chmod_bin="$(command -v chmod)"

    log_info "Configuring passwordless sudo for service management..."

    # Allow managing the zen service and updating the CLI binary without a password.
    # All paths are fully qualified to minimise the attack surface.
    content="# ZEN — passwordless sudo rules (managed by install.sh)
${user} ALL=(root) NOPASSWD: ${systemctl_bin} start ${SERVICE_NAME}
${user} ALL=(root) NOPASSWD: ${systemctl_bin} stop ${SERVICE_NAME}
${user} ALL=(root) NOPASSWD: ${systemctl_bin} restart ${SERVICE_NAME}
${user} ALL=(root) NOPASSWD: ${cp_bin} ${INSTALL_DIR}/script/zen.sh /usr/local/bin/zen
${user} ALL=(root) NOPASSWD: ${chmod_bin} +x /usr/local/bin/zen"

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would write $sudoers_file"
        return
    fi

    tmpf="$(mktemp)"
    printf '%s\n' "$content" > "$tmpf"
    if visudo -c -f "$tmpf" >/dev/null 2>&1; then
        $SUDO install -m 0440 -o root -g root "$tmpf" "$sudoers_file"
        # Remove any old sudoers file from previous naming (zen-<service>)
        $SUDO rm -f "/etc/sudoers.d/zen-${SERVICE_NAME}" 2>/dev/null || true
        log_info "Sudoers rule installed: $sudoers_file"
    else
        log_warn "visudo validation failed — skipping (auto-update will need manual sudo)"
    fi
    rm -f "$tmpf"
}

# Migrate from an old service name (e.g. "relay") to SERVICE_NAME (e.g. "zen").
# Called automatically when re-running install.sh on a machine with the old service.
migrate_old_service() {
    if [ "$SKIP_SERVICE" = "true" ]; then
        return
    fi
    old_name="relay"
    old_svc="/etc/systemd/system/${old_name}.service"

    # Nothing to do if same name or old service doesn't exist
    if [ "$SERVICE_NAME" = "$old_name" ]; then
        return
    fi
    if [ ! -f "$old_svc" ]; then
        return
    fi

    log_info "Migrating service: $old_name → $SERVICE_NAME"

    # Copy systemd drop-in overrides (e.g. peers.conf) to new service name
    old_drop="/etc/systemd/system/${old_name}.service.d"
    new_drop="/etc/systemd/system/${SERVICE_NAME}.service.d"
    if [ "$DRY_RUN" != "true" ] && [ -d "$old_drop" ]; then
        $SUDO mkdir -p "$new_drop"
        $SUDO cp -r "${old_drop}/." "$new_drop/"
        log_info "Migrated service drop-ins: $old_drop → $new_drop"
    fi

    # Stop and remove old service and its auto-update timer
    for unit in "${old_name}-update.timer" "${old_name}-update.service" "${old_name}.service"; do
        if systemctl list-unit-files "$unit" >/dev/null 2>&1 | grep -q "$unit"; then
            run $SUDO systemctl stop "$unit" 2>/dev/null || true
            run $SUDO systemctl disable "$unit" 2>/dev/null || true
        fi
        f="/etc/systemd/system/$unit"
        if [ -f "$f" ]; then
            run $SUDO rm -f "$f"
        fi
    done

    # Remove old sudoers file (both naming conventions)
    if [ "$DRY_RUN" != "true" ]; then
        $SUDO rm -f "/etc/sudoers.d/zen-${old_name}" "/etc/sudoers.d/${old_name}" 2>/dev/null || true
    fi

    if [ "$DRY_RUN" != "true" ]; then
        $SUDO systemctl daemon-reload
    fi
    log_info "Old '$old_name' service removed"
}

start_service() {
    if [ "$SKIP_SERVICE" = "true" ]; then
        return
    fi

    log_info "Starting $SERVICE_NAME..."
    run $SUDO systemctl restart "$SERVICE_NAME"

    if [ "$DRY_RUN" != "true" ]; then
        sleep 2
        if systemctl is-active --quiet "$SERVICE_NAME"; then
            log_info "Service is running"
        else
            log_warn "Service may have failed. Check: journalctl -u $SERVICE_NAME -n 50"
        fi
    fi
}

install_cli() {
    # Record install location in XDG config so the `zen` CLI can find it
    cfg_dir="${XDG_CONFIG_HOME:-$HOME/.config}/zen"
    mkdir -p "$cfg_dir"
    echo "$INSTALL_DIR"  > "$cfg_dir/install_dir"
    echo "$SERVICE_NAME" > "$cfg_dir/service_name"
    echo "$PORT"         > "$cfg_dir/port"
    if [ -n "$DOMAIN" ]; then
        echo "$DOMAIN" > "$cfg_dir/domain"
    fi

    # Determine where to place the `zen` binary
    if [ "$(id -u)" -eq 0 ]; then
        bin_dir="/usr/local/bin"
    elif [ -w "/usr/local/bin" ]; then
        bin_dir="/usr/local/bin"
    else
        bin_dir="$HOME/.local/bin"
        mkdir -p "$bin_dir"
    fi

    target="$bin_dir/zen"

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would install zen CLI → $target"
        return
    fi

    if [ -w "$bin_dir" ]; then
        cp "$INSTALL_DIR/script/zen.sh" "$target"
        chmod +x "$target"
    else
        $SUDO cp "$INSTALL_DIR/script/zen.sh" "$target"
        $SUDO chmod +x "$target"
    fi

    log_info "zen CLI installed → $target"

    if [ "$bin_dir" = "$HOME/.local/bin" ]; then
        if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
            log_warn "Add ~/.local/bin to your PATH:"
            log_warn "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc && source ~/.bashrc"
        fi
    fi
}

install_autoupdate() {
    if [ "$SKIP_SERVICE" = "true" ]; then
        log_info "Skipping auto-update timer (--skip-service)"
        return
    fi

    log_info "Installing auto-update timer: ${SERVICE_NAME}-update"

    node_bin="$(command -v node)"
    update_svc="/etc/systemd/system/${SERVICE_NAME}-update.service"
    update_tmr="/etc/systemd/system/${SERVICE_NAME}-update.timer"

    svc_content="$(sed \
        -e "s|__ZEN_USER__|$(whoami)|g" \
        -e "s|__ZEN_DIR__|$INSTALL_DIR|g" \
        -e "s|__ZEN_SERVICE__|$SERVICE_NAME|g" \
        "$INSTALL_DIR/script/zen-update.service")"
    tmr_content="$(sed \
        -e "s|__ZEN_SERVICE__|$SERVICE_NAME|g" \
        "$INSTALL_DIR/script/zen-update.timer")"

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would write $update_svc"
        log_info "[DRY RUN] Would write $update_tmr"
        return
    fi

    printf '%s\n' "$svc_content" | $SUDO tee "$update_svc" > /dev/null
    printf '%s\n' "$tmr_content" | $SUDO tee "$update_tmr" > /dev/null
    $SUDO systemctl daemon-reload
    $SUDO systemctl enable --now "${SERVICE_NAME}-update.timer"
    log_info "Auto-update timer enabled (checks every hour, restarts only on new commits)"
}

rollback() {
    log_warn "Installation failed, rolling back..."
    if [ -f "/etc/systemd/system/${SERVICE_NAME}.service" ]; then
        $SUDO systemctl stop "$SERVICE_NAME" 2>/dev/null || true
        $SUDO systemctl disable "$SERVICE_NAME" 2>/dev/null || true
        $SUDO rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
        $SUDO systemctl daemon-reload
    fi
    log_error "Rolled back. Fix errors and re-run."
    exit 1
}

main() {
    log_info "Starting ZEN installation..."
    log_info "  Version:   $VERSION"
    log_info "  Port:      $PORT"
    log_info "  Directory: $INSTALL_DIR"
    log_info "  Service:   $SERVICE_NAME"
    if [ -n "$DOMAIN" ]; then
        log_info "  Domain:    $DOMAIN"
    fi
    if [ -n "$PEERS" ]; then
        log_info "  Peers:     $PEERS"
    fi
    if [ -n "$HTTPS_KEY" ]; then
        log_info "  HTTPS Key: $HTTPS_KEY"
    fi
    if [ -n "$HTTPS_CERT" ]; then
        log_info "  HTTPS Cert: $HTTPS_CERT"
    fi
    if [ -n "$FMB" ]; then
        log_info "  FMB:       $FMB MB (disk warning)"
    fi
    if [ -n "$FRAT" ]; then
        log_info "  FRAT:      $FRAT (RAM eviction ratio)"
    fi
    if [ -n "$EVICT" ]; then
        log_info "  EVICT:     $EVICT"
    fi
    if [ -n "$GC_MB" ]; then
        log_info "  GC_MB:     $GC_MB MB"
    fi
    if [ -n "$GC_SEC" ]; then
        log_info "  GC_SEC:    $GC_SEC s"
    fi
    if [ -n "$GC_KEEP" ]; then
        log_info "  GC_KEEP:   $GC_KEEP s"
    fi
    if [ -n "$UDP_PORT" ]; then
        log_info "  UDP_PORT:  $UDP_PORT"
    fi

    if [ -d "$INSTALL_DIR" ] && [ -n "$(ls -A "$INSTALL_DIR" 2>/dev/null)" ]; then
        confirm "Directory $INSTALL_DIR is not empty. Continue?" || { log_info "Cancelled"; exit 0; }
    fi

    # Interactive prompts — use /dev/tty so this works with curl|bash too
    if [ -e /dev/tty ] && [ "$SKIP_SERVICE" != "true" ] && [ "$YES" != "true" ]; then
        if [ -z "$DOMAIN" ]; then
            printf '%b Your public domain or IP (e.g. peer1.example.com) [leave blank to auto-detect]: ' '\033[1;34m[ZEN]\033[0m' >/dev/tty
            read -r _dom </dev/tty
            DOMAIN="${_dom:-}"
        fi
        if [ "$PORT" = "8420" ]; then
            printf '%b Server port [8420]: ' '\033[1;34m[ZEN]\033[0m' >/dev/tty
            read -r _port </dev/tty
            PORT="${_port:-8420}"
            case "$PORT" in
                ''|*[!0-9]*) log_error "Invalid port: $PORT"; exit 1 ;;
            esac
            if [ "$PORT" -lt 1 ] || [ "$PORT" -gt 65535 ]; then
                log_error "Invalid port: $PORT"; exit 1
            fi
        fi
    fi

    available_space=$(df "$(dirname "$INSTALL_DIR")" | awk 'NR==2 {print $4}')
    if [ "$available_space" -lt 512000 ]; then
        log_error "Insufficient disk space (need 500MB, have $((available_space/1024))MB)"
        exit 1
    fi

    install_dependencies
    install_zen
    migrate_old_service
    create_service
    configure_limits
    configure_sudoers
    start_service
    install_autoupdate
    install_cli

    log_info "ZEN installation completed!"
    if [ "$SKIP_SERVICE" != "true" ]; then
        log_info "  Logs: journalctl -u $SERVICE_NAME -f"
    fi
    log_info "  Dir:  $INSTALL_DIR"
    log_info "  Run:  zen status"
}

main "$@"
