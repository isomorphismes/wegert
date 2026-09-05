#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later

# Canonical release identity for direct DEX/JNI builds and F-Droid metadata.
# The legacy Gradle compatibility path must match these values, but it is no
# longer the source of truth for releases.
WEGERT_VERSION_NAME=0.2.0
WEGERT_VERSION_CODE=101

[[ "$WEGERT_VERSION_NAME" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
[[ "$WEGERT_VERSION_CODE" =~ ^[0-9]+$ ]]

export WEGERT_VERSION_NAME WEGERT_VERSION_CODE
