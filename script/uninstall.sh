#!/bin/bash

# ZEN Uninstallation Script
# This script removes ZEN installation, systemd service, and optionally Node.js
# Usage: ./uninstall.sh [OPTIONS]

set -e

# Default values
SERVICE_NAME="relay"
INSTALL_DIR="$HOME/zen"
REMOVE_NODEJS=false
REMOVE_CERTS=false
REMOVE_ACME=false
KEEP_CONFIG=false
DRY_RUN=false
FORCE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Help function
show_help() {
    cat << EOF
ZEN Uninstallation Script

USAGE:
    $0 [OPTIONS]

OPTIONS:
    -s, --service NAME         Systemd service name to remove (default: relay)
    -d, --dir DIRECTORY        ZEN installation directory (default: ~/zen)
    --remove-nodejs            Also remove Node.js and npm
    --remove-certs             Remove SSL certificates (key.pem, cert.pem)
    --remove-acme              Remove acme.sh installation
    --keep-config              Keep configuration files and data
    --force                    Skip confirmation prompts
    --dry-run                  Show what would be done without executing
    -h, --help                 Show this help message

ENVIRONMENT VARIABLES:
    SERVICE_NAME, INSTALL_DIR

EXAMPLES:
    # Basic uninstallation (removes ZEN and service only)
    $0

    # Complete removal including Node.js and certificates
    $0 --remove-nodejs --remove-certs --remove-acme

    # Remove with custom service name and directory
    $0 --service my-zen-relay --dir /opt/zen

    # Dry run to see what would be removed
    $0 --remove-nodejs --remove-certs --dry-run

    # Force removal without prompts
    $0 --force --remove-nodejs --remove-certs

EOF
}

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_debug() {
    echo -e "${BLUE}[DEBUG]${NC} $1"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -s|--service)
            SERVICE_NAME="$2"
            shift 2
            ;;
        -d|--dir)
            INSTALL_DIR="$2"
            shift 2
            ;;
        --remove-nodejs)
            REMOVE_NODEJS=true
            shift
            ;;
        --remove-certs)
            REMOVE_CERTS=true
            shift
            ;;
        --remove-acme)
            REMOVE_ACME=true
            shift
            ;;
        --keep-config)
            KEEP_CONFIG=true
            shift
            ;;
        --force)
            FORCE=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Input validation functions
validate_service_name() {
    local name="$1"
    if [[ ! "$name" =~ ^[a-zA-Z0-9_-]+$ ]]; then
        log_error "Invalid service name: $name. Must contain only alphanumeric, underscore, or dash"
        exit 1
    fi
}

validate_path() {
    local path="$1"
    # Prevent path traversal
    if [[ "$path" =~ \.\./ ]]; then
        log_error "Path traversal detected: $path"
        exit 1
    fi
    # Prevent dangerous system directories
    case "$path" in
        /|/bin|/sbin|/usr|/etc|/var|/lib|/proc|/sys)
            log_error "Refusing to operate on system directory: $path"
            exit 1
            ;;
    esac
}

# Use environment variables with validation
SERVICE_NAME="${SERVICE_NAME:-relay}"
INSTALL_DIR="${INSTALL_DIR:-$HOME/zen}"

# Validate inputs
validate_service_name "$SERVICE_NAME"
validate_path "$INSTALL_DIR"

# Check if running as root for system operations
check_sudo() {
    if [[ $EUID -eq 0 ]]; then
        SUDO=""
    else
        SUDO="sudo"
        if ! command -v sudo &> /dev/null; then
            log_error "This script requires sudo privileges for system operations"
            exit 1
        fi
    fi
}

# Enhanced execute function with timeout and error handling
execute() {
    if [[ "$DRY_RUN" == "true" ]]; then
        log_info "[DRY RUN] Would execute: $*"
        return 0
    fi
    
    local cmd="$1"
    shift
    
    log_info "Executing: $cmd $*"
    
    # Execute with timeout
    if timeout 120 "$cmd" "$@"; then
        return 0
    else
        local exit_code=$?
        log_error "Command failed (exit code $exit_code): $cmd $*"
        return $exit_code
    fi
}

# Confirmation function
confirm() {
    if [[ "$FORCE" == "true" || "$DRY_RUN" == "true" ]]; then
        return 0
    fi
    
    echo -n -e "${YELLOW}$1 (y/N): ${NC}"
    read -r response
    case "$response" in
        [yY][eS]|[yY]) 
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Stop and remove auto-update timer
remove_autoupdate_timer() {
    local tmr="${SERVICE_NAME}-update.timer"
    local svc="${SERVICE_NAME}-update.service"

    for unit in "$tmr" "$svc"; do
        local f="/etc/systemd/system/$unit"
        if [[ -f "$f" ]]; then
            systemctl is-active --quiet "$unit" 2>/dev/null && execute $SUDO systemctl stop "$unit" || true
            systemctl is-enabled --quiet "$unit" 2>/dev/null && execute $SUDO systemctl disable "$unit" || true
            execute $SUDO rm -f "$f"
            log_info "Removed: $f"
        fi
    done
    execute $SUDO systemctl daemon-reload 2>/dev/null || true
}

# Remove passwordless sudo rules installed by install.sh
remove_sudoers() {
    local sudoers_file="/etc/sudoers.d/zen-${SERVICE_NAME}"
    if [[ -f "$sudoers_file" ]]; then
        execute $SUDO rm -f "$sudoers_file"
        log_info "Removed sudoers rule: $sudoers_file"
    fi
}

# Stop and remove systemd service
remove_service() {
    SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
    
    if [[ ! -f "$SERVICE_FILE" ]]; then
        log_info "Service $SERVICE_NAME not found, skipping"
        return
    fi
    
    log_info "Removing systemd service: $SERVICE_NAME"
    
    # Stop service if running
    if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
        log_info "Stopping service: $SERVICE_NAME"
        execute $SUDO systemctl stop "$SERVICE_NAME"
    fi
    
    # Disable service
    if systemctl is-enabled --quiet "$SERVICE_NAME" 2>/dev/null; then
        log_info "Disabling service: $SERVICE_NAME"
        execute $SUDO systemctl disable "$SERVICE_NAME"
    fi
    
    # Remove service file
    log_info "Removing service file: $SERVICE_FILE"
    execute $SUDO rm -f "$SERVICE_FILE"
    
    # Reload systemd
    execute $SUDO systemctl daemon-reload
    execute $SUDO systemctl reset-failed
    
    log_info "Service removed successfully"
}

# Remove ZEN installation
remove_zen() {
    if [[ ! -d "$INSTALL_DIR" ]]; then
        log_info "ZEN installation directory not found: $INSTALL_DIR"
        return
    fi
    
    log_info "Removing ZEN installation: $INSTALL_DIR"
    
    # Additional safety checks before removal
    if [[ ! -d "$INSTALL_DIR" ]]; then
        log_info "Directory does not exist: $INSTALL_DIR"
        return
    fi
    
    # Verify it looks like a ZEN installation
    if [[ ! -f "$INSTALL_DIR/package.json" ]] || ! grep -q '"name".*"zen"\|"@akaoio/zen"' "$INSTALL_DIR/package.json" 2>/dev/null; then
        log_warn "Directory does not appear to be a ZEN installation: $INSTALL_DIR"
        if ! confirm "Remove directory anyway?"; then
            log_info "Keeping directory"
            return
        fi
    fi
    
    if confirm "Remove ZEN installation directory $INSTALL_DIR?"; then
        # Stop any processes using the directory first
        local pids=$(lsof +D "$INSTALL_DIR" 2>/dev/null | awk 'NR>1 {print $2}' | sort -u || true)
        if [[ -n "$pids" ]]; then
            log_warn "Processes are using files in $INSTALL_DIR: $pids"
            if confirm "Kill these processes before removal?"; then
                echo "$pids" | xargs -r kill -TERM
                sleep 2
                echo "$pids" | xargs -r kill -KILL 2>/dev/null || true
            else
                log_error "Cannot remove directory while processes are using it"
                return 1
            fi
        fi
        
        execute rm -rf "$INSTALL_DIR"
        log_info "ZEN installation removed"
    else
        log_info "Keeping ZEN installation directory"
    fi
}

# Remove Node.js and npm
remove_nodejs() {
    if [[ "$REMOVE_NODEJS" != "true" ]]; then
        return
    fi
    
    if ! command -v node &> /dev/null && ! command -v npm &> /dev/null; then
        log_info "Node.js not found, skipping removal"
        return
    fi
    
    log_warn "This will remove Node.js and npm from your system"
    if ! confirm "Remove Node.js and npm?"; then
        log_info "Keeping Node.js and npm"
        return
    fi
    
    log_info "Removing Node.js and npm..."
    
    # Detect package manager and remove Node.js
    if command -v apt-get &> /dev/null; then
        execute $SUDO apt-get remove --purge -y nodejs npm
        execute $SUDO apt-get autoremove -y
    elif command -v yum &> /dev/null; then
        execute $SUDO yum remove -y nodejs npm
    elif command -v dnf &> /dev/null; then
        execute $SUDO dnf remove -y nodejs npm
    else
        log_warn "Unsupported package manager. Please remove Node.js manually."
        return
    fi
    
    # Remove global npm packages directory
    if [[ -d "$HOME/.npm" ]]; then
        if confirm "Remove npm cache and global packages (~/.npm)?"; then
            execute rm -rf "$HOME/.npm"
        fi
    fi
    
    log_info "Node.js and npm removed"
}

# Remove SSL certificates
remove_certificates() {
    if [[ "$REMOVE_CERTS" != "true" ]]; then
        return
    fi
    
    log_info "Removing SSL certificates..."
    
    XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
    CERT_FILES=("$XDG_CONFIG_HOME/zen/key.pem" "$XDG_CONFIG_HOME/zen/cert.pem" "/etc/ssl/private/zen.key" "/etc/ssl/certs/zen.crt")
    
    for cert_file in "${CERT_FILES[@]}"; do
        if [[ -f "$cert_file" ]]; then
            if confirm "Remove certificate file $cert_file?"; then
                execute rm -f "$cert_file"
                log_info "Removed: $cert_file"
            fi
        fi
    done
}

# Remove acme.sh
remove_acme() {
    if [[ "$REMOVE_ACME" != "true" ]]; then
        return
    fi
    
    ACME_DIR="$HOME/.acme.sh"
    
    if [[ ! -d "$ACME_DIR" ]]; then
        log_info "acme.sh not found, skipping removal"
        return
    fi
    
    log_info "Removing acme.sh installation..."
    
    if confirm "Remove acme.sh installation ($ACME_DIR)?"; then
        # Run acme.sh uninstall if available
        if [[ -f "$ACME_DIR/acme.sh" ]]; then
            log_info "Running acme.sh uninstall..."
            execute "$ACME_DIR/acme.sh" --uninstall || true
        fi
        
        # Remove directory
        execute rm -rf "$ACME_DIR"
        log_info "acme.sh removed"
    fi
}

# Remove configuration and data files
remove_config() {
    if [[ "$KEEP_CONFIG" == "true" ]]; then
        log_info "Keeping configuration files as requested"
        return
    fi
    
    log_info "Removing configuration and data files..."
    
    # Common ZEN data directories
    XDG_DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
    XDG_STATE_HOME="${XDG_STATE_HOME:-$HOME/.local/state}"
    DATA_DIRS=("$XDG_DATA_HOME/zen" "$XDG_STATE_HOME/zen" "${INSTALL_DIR%/*}/data" "${INSTALL_DIR%/*}/radata")
    
    for data_dir in "${DATA_DIRS[@]}"; do
        if [[ -d "$data_dir" ]]; then
            if confirm "Remove data directory $data_dir?"; then
                execute rm -rf "$data_dir"
                log_info "Removed: $data_dir"
            fi
        fi
    done
}

# Reset system limits
reset_limits() {
    log_info "Checking system limits configuration..."
    
    SYSCTL_FILE="/etc/sysctl.conf"
    if [[ -f "$SYSCTL_FILE" ]] && grep -q "fs.file-max = 999999" "$SYSCTL_FILE"; then
        if confirm "Remove file descriptor limit setting from $SYSCTL_FILE?"; then
            if [[ "$DRY_RUN" == "true" ]]; then
                log_info "[DRY RUN] Would remove 'fs.file-max = 999999' from $SYSCTL_FILE"
            else
                execute $SUDO sed -i '/fs.file-max = 999999/d' "$SYSCTL_FILE"
                execute $SUDO sysctl -p "$SYSCTL_FILE"
                log_info "System limits configuration removed"
            fi
        fi
    fi
}

# Remove zen CLI binary
remove_cli() {
    log_info "Removing zen CLI..."
    for bin in "/usr/local/bin/zen" "$HOME/.local/bin/zen"; do
        if [[ -f "$bin" ]]; then
            if [[ -w "$(dirname "$bin")" ]]; then
                execute rm -f "$bin"
            else
                execute $SUDO rm -f "$bin"
            fi
            log_info "Removed: $bin"
        fi
    done
    # Remove XDG config metadata written by install_cli
    XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
    for f in "$XDG_CONFIG_HOME/zen/install_dir" "$XDG_CONFIG_HOME/zen/service_name"; do
        [[ -f "$f" ]] && execute rm -f "$f" && log_info "Removed: $f"
    done
}

# Show what will be removed
show_removal_plan() {
    log_info "Uninstallation plan:"
    log_info "  Service: $SERVICE_NAME"
    log_info "  Installation directory: $INSTALL_DIR"
    [[ "$REMOVE_NODEJS" == "true" ]] && log_info "  Node.js and npm: YES"
    [[ "$REMOVE_CERTS" == "true" ]] && log_info "  SSL certificates: YES"
    [[ "$REMOVE_ACME" == "true" ]] && log_info "  acme.sh: YES"
    [[ "$KEEP_CONFIG" == "true" ]] && log_info "  Configuration data: KEEP" || log_info "  Configuration data: REMOVE"
    [[ "$DRY_RUN" == "true" ]] && log_warn "DRY RUN MODE - No changes will be made"
    echo
}

# Main uninstallation process
main() {
    log_info "Starting ZEN uninstallation..."
    
    show_removal_plan
    
    if [[ "$FORCE" != "true" && "$DRY_RUN" != "true" ]]; then
        if ! confirm "Proceed with uninstallation?"; then
            log_info "Uninstallation cancelled"
            exit 0
        fi
    fi
    
    check_sudo
    remove_service
    remove_autoupdate_timer
    remove_sudoers
    remove_cli
    remove_zen
    remove_nodejs
    remove_certificates
    remove_acme
    remove_config
    reset_limits
    
    log_info "ZEN uninstallation completed!"
    
    if [[ "$DRY_RUN" != "true" ]]; then
        log_info "You may want to:"
        log_info "  - Remove any remaining Node.js global packages manually"
        log_info "  - Check for any remaining ZEN-related files in your system"
        log_info "  - Restart your system if you removed Node.js"
    fi
}

# Run main function
main "$@"