#!/usr/bin/env bash
# Generate self-signed TLS certs for the Cat Laser relay server.
# Uses mkcert if available (recommended), otherwise falls back to openssl.

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CERT_DIR="$SCRIPT_DIR/certs"
mkdir -p "$CERT_DIR"

LOCAL_IP=$(ipconfig getifaddr en0 2>/dev/null || hostname -I 2>/dev/null | awk '{print $1}')
HOSTNAME=$(hostname)

if command -v mkcert &>/dev/null; then
  echo "Using mkcert..."
  mkcert -install 2>/dev/null || true
  mkcert -key-file "$CERT_DIR/key.pem" -cert-file "$CERT_DIR/cert.pem" \
    "$HOSTNAME" "$LOCAL_IP" localhost 127.0.0.1

  CA_ROOT=$(mkcert -CAROOT)
  echo ""
  echo "=== iPhone Setup ==="
  echo "AirDrop this CA cert to your iPhone, then trust it:"
  echo "  $CA_ROOT/rootCA.pem"
  echo ""
  echo "On iPhone:"
  echo "  1. Install the profile when prompted"
  echo "  2. Settings > General > About > Certificate Trust Settings"
  echo "  3. Enable full trust for the mkcert root CA"
else
  echo "mkcert not found, using openssl..."
  openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 \
    -keyout "$CERT_DIR/key.pem" -out "$CERT_DIR/cert.pem" \
    -days 365 -nodes \
    -subj "/CN=CatLaser" \
    -addext "subjectAltName=IP:${LOCAL_IP},DNS:localhost"

  echo ""
  echo "=== iPhone Setup ==="
  echo "AirDrop this cert to your iPhone, then trust it:"
  echo "  $CERT_DIR/cert.pem"
  echo ""
  echo "On iPhone:"
  echo "  1. Install the profile when prompted"
  echo "  2. Settings > General > About > Certificate Trust Settings"
  echo "  3. Enable full trust for CatLaser"
fi

echo ""
echo "Certs written to $CERT_DIR"
