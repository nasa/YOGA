#!/usr/bin/env bash

have_jupyter=0
type jupyter-notebook || have_jupyter=1

if [[ $have_jupyter != 0 ]];then
  echo "It appears that you don't have jupyter installed."
  echo "Try running: "
  echo "  pip install jupyter"
  echo "Then run this script again"
  exit 1
fi

jupyter-notebook
