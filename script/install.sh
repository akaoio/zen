#!/bin/bash

# ZEN Installation Script
# Installs Node.js 24, ZEN relay, and sets up a systemd service
# Usage: ./install.sh [OPTIONS]

set -euo pipefail

# Default values
VERSION="main"
PORT="8420"
DOMAIN=""
PEERS=""
RAD="true"
HTTPS_KEY=""
HTTPS_CERT=""
SERVICE_NAME="relay"
INSTALL_DIR="$HOME/zen"
SKIP_DEPS=false
SKIP_SERVICE=false
DRY_RUN=false
NODE_MAJOR=24

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

confirm() {
    local response
    if [[ -e /dev/tty ]]; then
        read -r -p "$1 [y/N] " response </dev/tty
    else
        read -r -p "$1 [y/N] " response
    fi
    [[ "$response" =~ ^[Yy]$ ]]
}

# Run a command (with dry-run support). Works for both external commands and shell built-ins.
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
ZEN Installation Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -v, --version VERSION       Git branch/tag to checkout (default: main)
    -p, --port PORT             Port number for the server (default: 8420)
    -D, --domain DOMAIN         Public domain name (e.g. peer1.akao.io)
    -P, --peers PEERS           Comma-separated list of peer URLs
    -d, --dir DIRECTORY         Installation directory (default: ~/zen)
    -s, --service NAME          Systemd service name (default: relay)
    --rad BOOL                  Enable/disable RAD storage (default: true)
    --https-key PATH            Path to HTTPS key file
    --https-cert PATH           Path to HTTPS certificate file
    --skip-deps                 Skip Node.js/system dependency installation
    --skip-service              Skip systemd service setup
    --dry-run                   Show what would be done without executing
    -h, --help                  Show this help message

EXAMPLES:
    $0
    $0 --port 443 --https-key ~/.config/zen/key.pem --https-cert ~/.config/zen/cert.pem
    $0 --peers "https://peer1.com/zen,https://peer2.com/zen"
    $0 --skip-service

EOF
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)   VERSION="$2";      shift 2 ;;
        -p|--port)      PORT="$2";         shift 2 ;;
        -D|--domain)    DOMAIN="$2";       shift 2 ;;
        -P|--peers)     PEERS="$2";        shift 2 ;;
        -d|--dir)       INSTALL_DIR="$2";  shift 2 ;;
        -s|--service)   SERVICE_NAME="$2"; shift 2 ;;
        --rad)          RAD="$2";          shift 2 ;;
        --https-key)    HTTPS_KEY="$2";    shift 2 ;;
        --https-cert)   HTTPS_CERT="$2";   shift 2 ;;
        --skip-deps)    SKIP_DEPS=true;    shift ;;
        --skip-service) SKIP_SERVICE=true; shift ;;
        --dry-run)      DRY_RUN=true;      shift ;;
        -h|--help)      show_help; exit 0 ;;
        *) log_error "Unknown option: $1"; show_help; exit 1 ;;
    esac
done

# Validation
if [[ ! "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
    log_error "Invalid port: $PORT"; exit 1
fi
if [[ ! "$SERVICE_NAME" =~ ^[a-zA-Z0-9_-]+$ ]]; then
    log_error "Invalid service name: $SERVICE_NAME"; exit 1
fi
if [[ "$INSTALL_DIR" =~ \.\. ]]; then
    log_error "Path traversal detected: $INSTALL_DIR"; exit 1
fi

# Determine sudo
if [[ $EUID -eq 0 ]]; then
    SUDO=""
else
    command -v sudo &>/dev/null || { log_error "sudo required"; exit 1; }
    SUDO="sudo"
fi

install_dependencies() {
    [[ "$SKIP_DEPS" == "true" ]] && { log_info "Skipping dependencies"; return; }

    log_info "Installing Node.js $NODE_MAJOR and dependencies..."

    if command -v apt-get &>/dev/null; then
        run $SUDO apt-get update -y
        run $SUDO apt-get install -y curl git ca-certificates gnupg

        # Install Node.js via NodeSource
        if ! node --version 2>/dev/null | grep -q "^v${NODE_MAJOR}\."; then
            log_info "Installing Node.js $NODE_MAJOR via NodeSource..."
            if [[ "$DRY_RUN" != "true" ]]; then
                curl -fsSL "https://deb.nodesource.com/setup_${NODE_MAJOR}.x" | $SUDO bash -
            fi
            run $SUDO apt-get install -y nodejs
        fi
    elif command -v dnf &>/dev/null; then
        run $SUDO dnf install -y curl git nodejs npm
    elif command -v yum &>/dev/null; then
        run $SUDO yum install -y curl git nodejs npm
    else
        log_error "Unsupported package manager. Install Node.js $NODE_MAJOR, git, curl manually."; exit 1
    fi

    log_info "Node.js $(node --version), npm $(npm --version)"
}

install_zen() {
    log_info "Installing ZEN to $INSTALL_DIR..."

    run mkdir -p "$(dirname "$INSTALL_DIR")"

    if [[ -d "$INSTALL_DIR/.git" ]]; then
        log_info "ZEN directory exists, updating..."
        run git -C "$INSTALL_DIR" fetch origin
        run git -C "$INSTALL_DIR" checkout "$VERSION"
        run git -C "$INSTALL_DIR" pull origin "$VERSION" || true
    else
        run git clone https://github.com/akaoio/zen.git "$INSTALL_DIR"
        run git -C "$INSTALL_DIR" checkout "$VERSION"
    fi

    run npm --prefix "$INSTALL_DIR" install
    log_info "ZEN installed successfully"
}

create_service() {
    [[ "$SKIP_SERVICE" == "true" ]] && { log_info "Skipping service creation"; return; }

    log_info "Creating systemd service: $SERVICE_NAME"

    local service_file="/etc/systemd/system/${SERVICE_NAME}.service"
    local node_bin
    node_bin="$(command -v node)"

    # Build env lines
    local env_lines=""
    [[ -n "$PORT" ]]       && env_lines+="Environment=PORT=$PORT\n"
    [[ -n "$DOMAIN" ]]     && env_lines+="Environment=DOMAIN=$DOMAIN\n"
    [[ -n "$PEERS" ]]      && env_lines+="Environment=PEERS=$PEERS\n"
    [[ -n "$RAD" ]]        && env_lines+="Environment=RAD=$RAD\n"
    [[ -n "$HTTPS_KEY" ]]  && env_lines+="Environment=HTTPS_KEY=$HTTPS_KEY\n"
    [[ -n "$HTTPS_CERT" ]] && env_lines+="Environment=HTTPS_CERT=$HTTPS_CERT\n"

    local service_content
    service_content="$(cat << EOF
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
$(printf "$env_lines")
[Install]
WantedBy=multi-user.target
EOF
)"

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] Would write $service_file:"
        echo "$service_content"
    else
        echo "$service_content" | $SUDO tee "$service_file" > /dev/null
    fi

    run $SUDO systemctl daemon-reload
    run $SUDO systemctl enable "$SERVICE_NAME"
    log_info "Service created and enabled"
}

configure_limits() {
    log_info "Configuring system limits..."
    if [[ "$DRY_RUN" != "true" ]]; then
        if ! grep -q "fs.file-max = 999999" /etc/sysctl.conf 2>/dev/null; then
            echo "fs.file-max = 999999" | $SUDO tee -a /etc/sysctl.conf > /dev/null
        fi
        $SUDO sysctl -p /etc/sysctl.conf
        ulimit -n 65536 || log_warn "Could not set ulimit (non-critical)"
    else
        log_info "[DRY RUN] Would configure sysctl and ulimit"
    fi
    log_info "System limits configured"
}

start_service() {
    [[ "$SKIP_SERVICE" == "true" ]] && return

    log_info "Starting $SERVICE_NAME..."
    run $SUDO systemctl restart "$SERVICE_NAME"

    if [[ "$DRY_RUN" != "true" ]]; then
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
    local cfg_dir="${XDG_CONFIG_HOME:-$HOME/.config}/zen"
    mkdir -p "$cfg_dir"
    echo "$INSTALL_DIR"  > "$cfg_dir/install_dir"
    echo "$SERVICE_NAME" > "$cfg_dir/service_name"
    echo "$PORT"         > "$cfg_dir/port"
    [[ -n "$DOMAIN" ]] && echo "$DOMAIN" > "$cfg_dir/domain"

    # Determine where to place the `zen` binary
    local bin_dir
    if [[ $EUID -eq 0 ]]; then
        bin_dir="/usr/local/bin"
    elif [[ -w "/usr/local/bin" ]]; then
        bin_dir="/usr/local/bin"
    else
        bin_dir="$HOME/.local/bin"
        mkdir -p "$bin_dir"
    fi

    local target="$bin_dir/zen"

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] Would install zen CLI → $target"
        return
    fi

    if [[ -w "$bin_dir" ]]; then
        cp "$INSTALL_DIR/script/zen.sh" "$target"
        chmod +x "$target"
    else
        $SUDO cp "$INSTALL_DIR/script/zen.sh" "$target"
        $SUDO chmod +x "$target"
    fi

    log_info "zen CLI installed → $target"

    if [[ "$bin_dir" == "$HOME/.local/bin" ]]; then
        if ! echo "$PATH" | grep -q "$HOME/.local/bin"; then
            log_warn "Add ~/.local/bin to your PATH:"
            log_warn "  echo 'export PATH=\"\$HOME/.local/bin:\$PATH\"' >> ~/.bashrc && source ~/.bashrc"
        fi
    fi
}

install_autoupdate() {
    [[ "$SKIP_SERVICE" == "true" ]] && { log_info "Skipping auto-update timer (--skip-service)"; return; }

    log_info "Installing auto-update timer: ${SERVICE_NAME}-update"

    local node_bin
    node_bin="$(command -v node)"
    local update_svc="/etc/systemd/system/${SERVICE_NAME}-update.service"
    local update_tmr="/etc/systemd/system/${SERVICE_NAME}-update.timer"

    local svc_content tmr_content
    svc_content="$(sed \
        -e "s|__ZEN_USER__|$(whoami)|g" \
        -e "s|__ZEN_DIR__|$INSTALL_DIR|g" \
        -e "s|__ZEN_SERVICE__|$SERVICE_NAME|g" \
        "$INSTALL_DIR/script/zen-update.service")"
    tmr_content="$(sed \
        -e "s|__ZEN_SERVICE__|$SERVICE_NAME|g" \
        "$INSTALL_DIR/script/zen-update.timer")"

    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] Would write $update_svc"
        log_info "[DRY RUN] Would write $update_tmr"
        return
    fi

    echo "$svc_content" | $SUDO tee "$update_svc" > /dev/null
    echo "$tmr_content" | $SUDO tee "$update_tmr" > /dev/null
    $SUDO systemctl daemon-reload
    $SUDO systemctl enable --now "${SERVICE_NAME}-update.timer"
    log_info "Auto-update timer enabled (checks every hour, restarts only on new commits)"
}

rollback() {
    log_warn "Installation failed, rolling back..."
    if [[ -f "/etc/systemd/system/${SERVICE_NAME}.service" ]]; then
        $SUDO systemctl stop "$SERVICE_NAME" 2>/dev/null || true
        $SUDO systemctl disable "$SERVICE_NAME" 2>/dev/null || true
        $SUDO rm -f "/etc/systemd/system/${SERVICE_NAME}.service"
        $SUDO systemctl daemon-reload
    fi
    log_error "Rolled back. Fix errors and re-run."
    exit 1
}

trap rollback ERR

main() {
    log_info "Starting ZEN installation..."
    log_info "  Version:   $VERSION"
    log_info "  Port:      $PORT"
    log_info "  Directory: $INSTALL_DIR"
    log_info "  Service:   $SERVICE_NAME"
    [[ -n "$DOMAIN" ]]    && log_info "  Domain:    $DOMAIN"
    [[ -n "$PEERS" ]]     && log_info "  Peers:     $PEERS"
    [[ -n "$HTTPS_KEY" ]] && log_info "  HTTPS Key: $HTTPS_KEY"
    [[ -n "$HTTPS_CERT" ]] && log_info "  HTTPS Cert: $HTTPS_CERT"

    if [[ -d "$INSTALL_DIR" ]] && [[ "$(ls -A "$INSTALL_DIR" 2>/dev/null)" ]]; then
        confirm "Directory $INSTALL_DIR is not empty. Continue?" || { log_info "Cancelled"; exit 0; }
    fi

    # Interactive prompts — use /dev/tty so this works with curl|bash too
    if [[ -e /dev/tty ]] && [[ "$SKIP_SERVICE" != "true" ]]; then
        if [[ -z "$DOMAIN" ]]; then
            read -rp "$(echo -e '\033[1;34m[ZEN]\033[0m') Your public domain or IP (e.g. peer1.example.com) [leave blank to auto-detect]: " _dom </dev/tty
            DOMAIN="${_dom:-}"
        fi
        if [[ "$PORT" == "8420" ]]; then
            read -rp "$(echo -e '\033[1;34m[ZEN]\033[0m') Server port [8420]: " _port </dev/tty
            PORT="${_port:-8420}"
            if [[ ! "$PORT" =~ ^[0-9]+$ ]] || (( PORT < 1 || PORT > 65535 )); then
                log_error "Invalid port: $PORT"; exit 1
            fi
        fi
    fi

    local available_space
    available_space=$(df "$(dirname "$INSTALL_DIR")" | awk 'NR==2 {print $4}')
    if (( available_space < 512000 )); then
        log_error "Insufficient disk space (need 500MB, have $((available_space/1024))MB)"; exit 1
    fi

    install_dependencies
    install_zen
    create_service
    configure_limits
    start_service
    install_autoupdate
    install_cli

    trap - ERR

    log_info "ZEN installation completed!"
    [[ "$SKIP_SERVICE" != "true" ]] && log_info "  Logs: journalctl -u $SERVICE_NAME -f"
    log_info "  Dir:  $INSTALL_DIR"
    log_info "  Run:  zen status"
}

main "$@"