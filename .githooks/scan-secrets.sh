#!/bin/sh
# Blocks commits that contain secrets (INV-15): secret-looking files and key or token patterns.
set -eu

status=0

for file in $(git diff --cached --name-only --diff-filter=ACMR); do
  case "$(basename "$file")" in
    .env | .env.* | *.pem | *.key | *.pfx | *.p12 | id_rsa* | id_ed25519* | secrets.json)
      echo "  BLOCKED: '$file' looks like a secret file."
      status=1
      ;;
  esac
done

if git diff --cached -U0 --diff-filter=ACMR | grep -E '^\+' \
  | grep -Eq -- '-----BEGIN [A-Z ]*PRIVATE KEY-----|gh[pousr]_[A-Za-z0-9]{36}|github_pat_[A-Za-z0-9_]{22,}|AKIA[0-9A-Z]{16}'; then
  echo "  BLOCKED: the staged changes contain something that looks like a private key or an access token."
  status=1
fi

exit "$status"
