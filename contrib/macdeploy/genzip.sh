#!/bin/bash

PRODUCT_NAME="MotaCoin-Qt"
EXPORT_PATH="/Applications"
APP_PATH="$EXPORT_PATH/$PRODUCT_NAME.app"
ZIP_PATH="$EXPORT_PATH/$PRODUCT_NAME.zip"

# Create a ZIP archive suitable for notarization.
/usr/bin/ditto -c -k --keepParent "$APP_PATH" "$ZIP_PATH"

# As a convenience, open the export folder in Finder.
/usr/bin/open "$EXPORT_PATH"
