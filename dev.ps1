#!/bin/pwsh

./scripts/build-default.ps1
if (!$?) {
    exit 1
}

./scripts/run-default.ps1
if (!$?) {
    exit 1
}
