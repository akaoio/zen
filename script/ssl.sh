#!/bin/sh

# SSL Certificate Management Script using acme.sh
# This script sets up SSL certificates using Let's Encrypt via acme.sh
# Usage: ./ssl.sh [OPTIONS]

set -e

# Default values
DOMAIN=""
EMAIL=""
WEBROOT=""
XDG_CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
ZEN_CONFIG_DIR="$XDG_CONFIG_HOME/zen"
KEY_FILE="$ZEN_CONFIG_DIR/key.pem"
CERT_FILE="$ZEN_CONFIG_DIR/cert.pem"
ACME_DIR="$HOME/.acme.sh"
FORCE_INSTALL=false
DRY_RUN=false
STAGING=false
RELOAD_CMD=""
AUTO_UPGRADE=true
STANDALONE=false
IS_IPV6=false
AUTO_IP6=true
DNS_MODE=false
DNS_PROVIDER=""
DNS_API_KEY=""
DNS_API_SECRET=""
DNS_EMAIL=""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Help function
show_help() {
    cat << EOF2
SSL Certificate Management Script

USAGE:
    $0 [OPTIONS]

REQUIRED OPTIONS:
    -d, --domain DOMAIN        Domain name for the certificate
    -e, --email EMAIL          Email address for Let's Encrypt notifications

OPTIONAL:
    -w, --webroot PATH         Webroot path for domain validation (default: current dir)
    -k, --key-file PATH        Output path for private key (default: ~/.config/zen/key.pem)
    -c, --cert-file PATH       Output path for certificate (default: ~/.config/zen/cert.pem)
    --acme-dir PATH            ACME installation directory (default: ~/.acme.sh)
    --reload-cmd COMMAND       Command to run after certificate installation
    --standalone               Use standalone mode (temporary web server on port 80)
    --dns [PROVIDER]          Use DNS validation. Options: manual, cloudflare, route53, digitalocean, godaddy
    --dns-api-key KEY         DNS provider API key (required for automatic DNS)
    --dns-api-secret SECRET   DNS provider API secret (for some providers)
    --dns-email EMAIL         DNS provider email (for Cloudflare)
    --force                    Force reinstallation of acme.sh
    --staging                  Use Let's Encrypt staging environment (for testing)
    --no-auto-upgrade          Disable automatic acme.sh upgrades
    --no-auto-ip6              Skip automatic IPv6 certificate (when --domain is a hostname)
    --dry-run                  Show what would be done without executing
    -h, --help                 Show this help message

ENVIRONMENT VARIABLES:
    DOMAIN, EMAIL, WEBROOT, KEY_FILE, CERT_FILE, RELOAD_CMD

EXAMPLES:
    # Basic certificate for a domain
    $0 --domain example.com --email admin@example.com

    # Standalone mode (no web server needed)
    $0 --domain example.com --email admin@example.com --standalone

    # DNS validation with Cloudflare (automatic)
    $0 --domain example.com --email admin@example.com --dns cloudflare --dns-api-key YOUR_CF_API_KEY --dns-email admin@example.com

    # DNS validation with Route53 (automatic)
    $0 --domain example.com --email admin@example.com --dns route53 --dns-api-key AWS_KEY --dns-api-secret AWS_SECRET

    # DNS validation with GoDaddy (automatic)
    $0 --domain example.com --email admin@example.com --dns godaddy --dns-api-key GD_KEY --dns-api-secret GD_SECRET

    # Manual DNS validation (IPv6-only servers)
    $0 --domain example.com --email admin@example.com --dns

    # Raw IPv6 address certificate (uses standalone --listen-v6, requires port 80 free)
    $0 --domain 2a02:c207:2327:7266::1 --email admin@example.com

    # Certificate with custom webroot and reload command
    $0 -d example.com -e admin@example.com -w /var/www/html --reload-cmd "systemctl reload nginx"

    # Staging certificate for testing
    $0 --domain test.example.com --email admin@example.com --staging

    # Custom certificate paths
    $0 -d example.com -e admin@example.com --key-file /etc/ssl/private/example.key --cert-file /etc/ssl/certs/example.crt

EOF2
}

# Logging functions
log_info() {
    printf '%b[INFO]%b %s\n' "${GREEN}" "${NC}" "$1"
}

log_warn() {
    printf '%b[WARN]%b %s\n' "${YELLOW}" "${NC}" "$1"
}

log_error() {
    printf '%b[ERROR]%b %s\n' "${RED}" "${NC}" "$1"
}

log_debug() {
    printf '%b[DEBUG]%b %s\n' "${BLUE}" "${NC}" "$1"
}

# Parse command line arguments
while [ $# -gt 0 ]; do
    case "$1" in
        -d|--domain)
            DOMAIN="$2"
            shift 2
            ;;
        -e|--email)
            EMAIL="$2"
            shift 2
            ;;
        -w|--webroot)
            WEBROOT="$2"
            shift 2
            ;;
        -k|--key-file)
            KEY_FILE="$2"
            shift 2
            ;;
        -c|--cert-file)
            CERT_FILE="$2"
            shift 2
            ;;
        --acme-dir)
            ACME_DIR="$2"
            shift 2
            ;;
        --reload-cmd)
            RELOAD_CMD="$2"
            shift 2
            ;;
        --standalone)
            STANDALONE=true
            shift
            ;;
        --dns)
            DNS_MODE=true
            case "${2:-}" in
                ''|--*) DNS_PROVIDER="manual"; shift ;;
                *)      DNS_PROVIDER="$2"; shift 2 ;;
            esac
            ;;
        --dns-api-key)
            DNS_API_KEY="$2"
            shift 2
            ;;
        --dns-api-secret)
            DNS_API_SECRET="$2"
            shift 2
            ;;
        --dns-email)
            DNS_EMAIL="$2"
            shift 2
            ;;
        --force)
            FORCE_INSTALL=true
            shift
            ;;
        --staging)
            STAGING=true
            shift
            ;;
        --no-auto-upgrade)
            AUTO_UPGRADE=false
            shift
            ;;
        --no-auto-ip6)
            AUTO_IP6=false
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

# Discover the outbound global IPv6 of this machine (no Node)
discover_local_ip6() {
    local ip6
    # Prefer route-based discovery
    ip6=$(ip -6 route get 2001:4860:4860::8888 2>/dev/null \
          | sed -n 's/.*src \([0-9a-fA-F:]*\).*/\1/p' | head -1)
    if [ -n "$ip6" ] && is_ipv6_address "$ip6"; then
        printf '%s\n' "$ip6"
        return 0
    fi
    # Fallback: first global unicast address
    ip6=$(ip -6 addr show scope global 2>/dev/null \
          | sed -n 's/.*inet6 \([0-9a-fA-F:]*\)\/.*/\1/p' \
          | grep -v '^fe80' | head -1)
    if [ -n "$ip6" ] && is_ipv6_address "$ip6"; then
        printf '%s\n' "$ip6"
        return 0
    fi
    return 1
}

# Detect if a string is a raw IPv6 address
is_ipv6_address() {
    local d="$1"
    # Strip brackets if present
    d="${d#[}"
    d="${d%]}"
    # Must contain at least two colons (IPv6) and only hex chars + colons
    case "$d" in
        *:*:*) ;;
        *) return 1 ;;
    esac
    case "$d" in
        *[!0-9a-fA-F:]*) return 1 ;;
    esac
    return 0
}

# Input validation functions
validate_domain() {
    local domain="$1"
    if is_ipv6_address "$domain"; then
        return 0
    fi
    # Basic domain validation using case pattern
    case "$domain" in
        '') log_error "Domain cannot be empty"; exit 1 ;;
        *[!a-zA-Z0-9.-]*) log_error "Invalid domain format: $domain"; exit 1 ;;
        .*|*.) log_error "Invalid domain format: $domain"; exit 1 ;;
    esac
    local len
    len=$(printf '%s' "$domain" | wc -c | tr -d ' ')
    if [ "$len" -gt 253 ]; then
        log_error "Domain too long: $domain (max 253 chars)"
        exit 1
    fi
}

validate_email() {
    local email="$1"
    case "$email" in
        *@*.*) ;;
        *) log_error "Invalid email format: $email"; exit 1 ;;
    esac
}

validate_path() {
    local path="$1"
    case "$path" in
        *../*|*..) log_error "Path traversal detected: $path"; exit 1 ;;
    esac
    case "$path" in
        *.pem|*.key|*.crt|*.cert)
            case "$path" in
                /*) ;;
                *) log_error "Key/cert files must use absolute paths: $path"; exit 1 ;;
            esac
            ;;
    esac
}

sanitize_command() {
    local cmd="$1"
    # Remove potentially dangerous characters
    printf '%s\n' "$cmd" | sed 's/[;&|`$(){}\[\]\\]//g'
}

get_acme_domain_dir() {
    local escaped_domain

    if [ -d "$ACME_DIR/${DOMAIN}_ecc" ]; then
        echo "$ACME_DIR/${DOMAIN}_ecc"
        return 0
    fi

    if [ -d "$ACME_DIR/$DOMAIN" ]; then
        echo "$ACME_DIR/$DOMAIN"
        return 0
    fi

    # acme.sh may encode colons in IPv6 dirs (some versions use underscores)
    escaped_domain=$(printf '%s' "$DOMAIN" | sed 's/:/_/g')
    if [ -d "$ACME_DIR/${escaped_domain}_ecc" ]; then
        echo "$ACME_DIR/${escaped_domain}_ecc"
        return 0
    fi
    if [ -d "$ACME_DIR/$escaped_domain" ]; then
        echo "$ACME_DIR/$escaped_domain"
        return 0
    fi

    return 1
}

# Use environment variables if not set by flags (with defaults)
DOMAIN="${DOMAIN:-}"
EMAIL="${EMAIL:-}"
WEBROOT="${WEBROOT:-}"
KEY_FILE="${KEY_FILE:-$ZEN_CONFIG_DIR/key.pem}"
CERT_FILE="${CERT_FILE:-$ZEN_CONFIG_DIR/cert.pem}"
RELOAD_CMD="${RELOAD_CMD:-}"

# Validate required parameters
if [ -z "$DOMAIN" ]; then
    log_error "Domain is required. Use --domain or set DOMAIN environment variable."
    show_help
    exit 1
fi

if [ -z "$EMAIL" ]; then
    log_error "Email is required. Use --email or set EMAIL environment variable."
    show_help
    exit 1
fi

# Auto-detect raw IPv6 address — force standalone + listen-v6 mode
if is_ipv6_address "$DOMAIN"; then
    IS_IPV6=true
    STANDALONE=true
    # Strip brackets if user passed [2001:db8::1]
    DOMAIN="${DOMAIN#[}"
    DOMAIN="${DOMAIN%]}"
    log_info "Raw IPv6 address detected — using standalone --listen-v6 mode"
fi

# Validate inputs
validate_domain "$DOMAIN"
validate_email "$EMAIL"
[ -n "$WEBROOT" ] && validate_path "$WEBROOT"
validate_path "$KEY_FILE"
validate_path "$CERT_FILE"
[ -n "$RELOAD_CMD" ] && RELOAD_CMD="$(sanitize_command "$RELOAD_CMD")"

# Set default webroot if not specified
if [ -z "$WEBROOT" ]; then
    WEBROOT="$(pwd)"
    log_info "Using current directory as webroot: $WEBROOT"
fi

# Dry run function
execute() {
    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would execute: $*"
    else
        "$@"
    fi
}

# Check if acme.sh is installed
check_acme_installation() {
    if [ -d "$ACME_DIR" ] && [ -f "$ACME_DIR/acme.sh" ] && [ "$FORCE_INSTALL" != "true" ]; then
        log_info "acme.sh is already installed at $ACME_DIR"
        return 0
    else
        return 1
    fi
}

# Install acme.sh
install_acme() {
    if check_acme_installation; then
        return 0
    fi

    log_info "Installing acme.sh..."

    local temp_dir

    # Remove existing installation if force install
    if [ "$FORCE_INSTALL" = "true" ] && [ -d "$ACME_DIR" ]; then
        log_info "Force install: removing existing acme.sh installation"
        execute rm -rf "$ACME_DIR"
    fi

    # Create temporary directory for download
    temp_dir=$(mktemp -d)

    cleanup_temp_dir() {
        [ -d "$temp_dir" ] && rm -rf "$temp_dir"
    }

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would download and install acme.sh"
        cleanup_temp_dir
        return 0
    fi

    # Download and install acme.sh
    execute git clone https://github.com/acmesh-official/acme.sh.git "$temp_dir/acme.sh"

    # Build install command from within the cloned repo because acme.sh expects
    # its support files to be available relative to the current working directory.
    INSTALL_CMD="cd \"$temp_dir/acme.sh\" && ./acme.sh --install --home \"$ACME_DIR\" --accountemail \"$EMAIL\""
    if [ "$AUTO_UPGRADE" = "false" ]; then
        INSTALL_CMD="$INSTALL_CMD --noupgrade"
    fi

    execute sh -c "$INSTALL_CMD"
    cleanup_temp_dir

    log_info "acme.sh installed successfully"
}

# Issue certificate
issue_certificate() {
    log_info "Issuing certificate for domain: $DOMAIN"

    # Build acme.sh command
    if [ "$STANDALONE" = "true" ]; then
        if [ "$IS_IPV6" = "true" ]; then
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --standalone --listen-v6"
            log_info "Using standalone IPv6 mode (temporary web server on port 80, IPv6)"
        else
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --standalone"
            log_info "Using standalone mode (temporary web server on port 80)"
        fi
    elif [ "$DNS_MODE" = "true" ]; then
        # Setup DNS provider environment variables
        if [ "$DNS_PROVIDER" = "cloudflare" ]; then
            [ -n "$DNS_API_KEY" ] && export CF_Key="$DNS_API_KEY"
            [ -n "$DNS_EMAIL" ] && export CF_Email="$DNS_EMAIL"
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --dns dns_cf"
            log_info "Using Cloudflare automatic DNS validation"
        elif [ "$DNS_PROVIDER" = "route53" ]; then
            [ -n "$DNS_API_KEY" ] && export AWS_ACCESS_KEY_ID="$DNS_API_KEY"
            [ -n "$DNS_API_SECRET" ] && export AWS_SECRET_ACCESS_KEY="$DNS_API_SECRET"
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --dns dns_aws"
            log_info "Using Route53 automatic DNS validation"
        elif [ "$DNS_PROVIDER" = "digitalocean" ]; then
            [ -n "$DNS_API_KEY" ] && export DO_API_KEY="$DNS_API_KEY"
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --dns dns_dgon"
            log_info "Using DigitalOcean automatic DNS validation"
        elif [ "$DNS_PROVIDER" = "godaddy" ]; then
            [ -n "$DNS_API_KEY" ] && export GD_Key="$DNS_API_KEY"
            [ -n "$DNS_API_SECRET" ] && export GD_Secret="$DNS_API_SECRET"
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --dns dns_gd"
            log_info "Using GoDaddy automatic DNS validation"
        else
            ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 --dns --yes-I-know-dns-manual-mode-enough-go-ahead-please"
            log_info "Using DNS validation mode (manual TXT record)"
        fi
    else
        ACME_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt --issue -d \"$DOMAIN\" --keylength ec-256 -w \"$WEBROOT\""
        log_info "Using webroot mode with path: $WEBROOT"
    fi

    if [ "$STAGING" = "true" ]; then
        ACME_CMD="$ACME_CMD --staging"
        log_warn "Using Let's Encrypt staging environment (test certificates)"
    fi

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would issue certificate with: $ACME_CMD"
        return 0
    fi

    # Execute certificate issuance with proper error handling
    log_debug "Executing: $ACME_CMD"

    if execute sh -c "$ACME_CMD"; then
        log_info "Certificate issued successfully"

        # Verify certificate was actually created
        local cert_dir
        cert_dir=$(get_acme_domain_dir) || true
        if [ -z "$cert_dir" ] || [ ! -f "$cert_dir/fullchain.cer" ]; then
            log_error "Certificate issuance reported success but files not found"
            exit 1
        fi
    else
        local exit_code
        exit_code=$?
        log_error "Certificate issuance failed with exit code $exit_code"

        # Show debug information
        log_info "For troubleshooting, run with --debug:"
        log_info "$ACME_DIR/acme.sh --issue -d $DOMAIN --debug"
        exit 1
    fi
}

# Install certificate
install_certificate() {
    log_info "Installing certificate to:"
    log_info "  Key file: $KEY_FILE"
    log_info "  Cert file: $CERT_FILE"

    # Create directories if they don't exist
    execute mkdir -p "$(dirname "$KEY_FILE")"
    execute mkdir -p "$(dirname "$CERT_FILE")"

    local cert_type_flag
    cert_type_flag=""
    if [ -d "$ACME_DIR/${DOMAIN}_ecc" ]; then
        cert_type_flag=" --ecc"
    fi

    # Build install command
    INSTALL_CMD="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --install-cert -d \"$DOMAIN\"$cert_type_flag --key-file \"$KEY_FILE\" --fullchain-file \"$CERT_FILE\""

    if [ -n "$RELOAD_CMD" ]; then
        INSTALL_CMD="$INSTALL_CMD --reloadcmd \"$RELOAD_CMD\""
        log_info "  Reload command: $RELOAD_CMD"
    fi

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would install certificate with: $INSTALL_CMD"
        return 0
    fi

    # Execute certificate installation
    if execute sh -c "$INSTALL_CMD"; then
        log_info "Certificate installed successfully"
    else
        log_error "Certificate installation failed"
        exit 1
    fi
}

# Verify certificate
verify_certificate() {
    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would verify certificate files"
        return 0
    fi

    log_info "Verifying certificate installation..."

    if [ -f "$KEY_FILE" ] && [ -f "$CERT_FILE" ]; then
        log_info "Certificate files exist:"
        log_info "  Key: $KEY_FILE ($(stat -c%s "$KEY_FILE") bytes)"
        log_info "  Cert: $CERT_FILE ($(stat -c%s "$CERT_FILE") bytes)"

        # Check certificate validity
        if command -v openssl >/dev/null 2>&1; then
            CERT_INFO=$(openssl x509 -in "$CERT_FILE" -text -noout 2>/dev/null || echo "")
            if [ -n "$CERT_INFO" ]; then
                EXPIRY=$(printf '%s\n' "$CERT_INFO" | grep "Not After" | cut -d: -f2- | xargs)
                log_info "  Certificate expires: $EXPIRY"
            fi
        fi
    else
        log_error "Certificate files not found after installation"
        exit 1
    fi
}

# Issue and install a certificate for the local IPv6 address
# Called automatically after the domain cert when AUTO_IP6=true
issue_ip6_cert() {
    local ip6="$1"
    log_info "Auto-issuing certificate for local IPv6: $ip6"

    local ip6_key="$ZEN_CONFIG_DIR/ip6-key.pem"
    local ip6_cert="$ZEN_CONFIG_DIR/ip6-cert.pem"

    # Check port 80 (IPv6)
    if ss -tlnp 2>/dev/null | grep -q '\[::\]:80\|:::80'; then
        log_warn "Port 80 (IPv6) busy — skipping IPv6 cert auto-issue. Run manually after freeing port 80."
        return 0
    fi

    local ip6_acme_cmd
    ip6_acme_cmd="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" --server letsencrypt \
--issue -d \"$ip6\" --keylength ec-256 --standalone --listen-v6"

    if [ "$STAGING" = "true" ]; then
        ip6_acme_cmd="$ip6_acme_cmd --staging"
    fi

    if [ "$DRY_RUN" = "true" ]; then
        log_info "[DRY RUN] Would issue IPv6 cert: $ip6_acme_cmd"
        log_info "[DRY RUN] Would install IPv6 cert → $ip6_key / $ip6_cert"
        return 0
    fi

    log_debug "Executing IPv6 cert: $ip6_acme_cmd"
    if sh -c "$ip6_acme_cmd"; then
        log_info "IPv6 certificate issued"
    else
        log_warn "IPv6 cert issuance failed (non-fatal). You can retry manually:"
        log_warn "  $0 --domain $ip6 --email $EMAIL --key-file $ip6_key --cert-file $ip6_cert"
        return 0  # non-fatal — domain cert is already installed
    fi

    # Install IPv6 cert to predictable path; register reload hook for renewal
    local ip6_install_cmd
    ip6_install_cmd="\"$ACME_DIR/acme.sh\" --home \"$ACME_DIR\" \
--install-cert -d \"$ip6\" --ecc \
--key-file \"$ip6_key\" --fullchain-file \"$ip6_cert\""

    if [ -n "$RELOAD_CMD" ]; then
        ip6_install_cmd="$ip6_install_cmd --reloadcmd \"$RELOAD_CMD\""
    fi

    if sh -c "$ip6_install_cmd"; then
        log_info "IPv6 certificate installed → $ip6_key"
        log_info "IPv6 certificate installed → $ip6_cert"
    else
        log_warn "IPv6 cert install step failed. Retry: $ip6_install_cmd"
    fi
}

# Show renewal information
show_renewal_info() {
    log_info "Certificate renewal information:"
    log_info "  Certificates are automatically renewed by acme.sh"
    log_info "  Check renewal status: $ACME_DIR/acme.sh --home $ACME_DIR --list"
    log_info "  Manual renewal: $ACME_DIR/acme.sh --home $ACME_DIR --renew -d $DOMAIN"
    if [ -n "$RELOAD_CMD" ]; then
        log_info "  Service will be reloaded automatically: $RELOAD_CMD"
    fi
}

# Cleanup function for failed SSL operations
cleanup_ssl() {
    status=$?
    if [ "$status" -eq 0 ]; then
        return 0
    fi

    trap - 0
    log_warn "SSL setup failed, cleaning up..."

    # Remove failed certificate attempt
    if [ -d "$ACME_DIR/$DOMAIN" ]; then
        rm -rf "$ACME_DIR/$DOMAIN"
        log_info "Removed failed certificate directory"
    fi
    if [ -d "$ACME_DIR/${DOMAIN}_ecc" ]; then
        rm -rf "$ACME_DIR/${DOMAIN}_ecc"
        log_info "Removed failed certificate directory"
    fi

    # Remove incomplete certificate files
    [ -f "$KEY_FILE" ] && [ ! -s "$KEY_FILE" ] && rm -f "$KEY_FILE"
    [ -f "$CERT_FILE" ] && [ ! -s "$CERT_FILE" ] && rm -f "$CERT_FILE"

    log_error "SSL setup failed and cleaned up"
    exit "$status"
}

# Set up error trap
trap 'cleanup_ssl' 0

# Main process
main() {
    log_info "Starting SSL certificate setup..."
    log_info "Domain: $DOMAIN"
    log_info "Email: $EMAIL"
    log_info "Webroot: $WEBROOT"
    log_info "Key file: $KEY_FILE"
    log_info "Cert file: $CERT_FILE"
    [ "$STAGING" = "true" ] && log_warn "Using staging environment"
    [ "$DRY_RUN" = "true" ] && log_warn "DRY RUN MODE - No changes will be made"

    # Pre-flight checks
    if [ "$STANDALONE" = "true" ]; then
        # Check if port 80 is available for standalone mode
        if [ "$IS_IPV6" = "true" ]; then
            if ss -tlnp | grep -q '\[::\]:80\|:::80'; then
                log_error "Port 80 (IPv6) is already in use. Stop other services or use DNS mode"
                exit 1
            fi
        else
            if ss -tlnp | grep -q ':80 '; then
                log_error "Port 80 is already in use. Stop other services or use webroot mode"
                exit 1
            fi
        fi
    fi

    if [ "$DNS_MODE" = "true" ] && [ "$DNS_PROVIDER" != "manual" ]; then
        # Validate DNS API credentials are provided
        case "$DNS_PROVIDER" in
            cloudflare)
                if [ -z "$DNS_API_KEY" ] || [ -z "$DNS_EMAIL" ]; then
                    log_error "Cloudflare requires --dns-api-key and --dns-email"
                    exit 1
                fi
                ;;
            route53)
                if [ -z "$DNS_API_KEY" ] || [ -z "$DNS_API_SECRET" ]; then
                    log_error "Route53 requires --dns-api-key and --dns-api-secret"
                    exit 1
                fi
                ;;
            digitalocean)
                if [ -z "$DNS_API_KEY" ]; then
                    log_error "DigitalOcean requires --dns-api-key"
                    exit 1
                fi
                ;;
            godaddy)
                if [ -z "$DNS_API_KEY" ] || [ -z "$DNS_API_SECRET" ]; then
                    log_error "GoDaddy requires --dns-api-key and --dns-api-secret"
                    exit 1
                fi
                ;;
        esac
    fi

    install_acme
    issue_certificate
    install_certificate
    verify_certificate
    show_renewal_info

    # Auto-issue cert for local IPv6 when domain cert was requested
    # (not when we were already issuing for a raw IPv6 address)
    if [ "$AUTO_IP6" = "true" ] && [ "$IS_IPV6" = "false" ]; then
        local detected_ip6
        detected_ip6=$(discover_local_ip6 2>/dev/null || true)
        if [ -n "$detected_ip6" ] && [ "$detected_ip6" != "$DOMAIN" ]; then
            log_info "Local IPv6 detected: $detected_ip6 — requesting certificate"
            issue_ip6_cert "$detected_ip6"
        else
            log_info "No global IPv6 address found on this machine — skipping IPv6 cert"
        fi
    fi

    # Disable error trap on success
    trap - 0

    log_info "SSL certificate setup completed successfully!"
}

# Run main function
main "$@"
