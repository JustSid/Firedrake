#!/bin/bash

if [ $# -le 1 ]; then
	echo "Please provide a a template name and project name!"
    exit 0
fi

BASEDIR=$(dirname $0)

TEMPLATE=t_$1

PROJECT_TITLE=$2
PROJECT_NAME=lib$PROJECT_TITLE
CREATOR=$USER

PROJECT_DIR="$BASEDIR/$PROJECT_NAME"
MAKEFILE="$PROJECT_DIR/Makefile"

rm -rf "$PROJECT_DIR"
cp -r "$BASEDIR/templates/$TEMPLATE" "$PROJECT_DIR"

FILES=$(find "$PROJECT_DIR" -type f \( -name "template*" \))

for f in $FILES
do
	sed -i "" "s/{PROJECT_TITLE}/$PROJECT_TITLE/g;s/{PROJECT_NAME}/$PROJECT_NAME/g;s/{CREATOR}/$CREATOR/g" "$f"
	mv $f "$PROJECT_DIR/$PROJECT_TITLE.$(echo $(basename $f) | awk -F . '{if (NF>1) {print $NF}}')"
done

sed -i "" "s/{PROJECT_TITLE}/$PROJECT_TITLE/g;s/{PROJECT_NAME}/$PROJECT_NAME/g;s/{CREATOR}/$CREATOR/g" "$MAKEFILE"
