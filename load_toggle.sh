#!/bin/bash

CORES=${1:-2}  # default: start 2 yes processes

start_load() {
  echo "Starting $CORES load processes..."
  for ((i = 0; i < CORES; i++)); do
    taskset -c $i yes > /dev/null &
    echo $! >> /tmp/load_pids.txt
  done
  echo "CPU load started on cores 0 to $((CORES-1))."
}

stop_load() {
  if [ -f /tmp/load_pids.txt ]; then
    echo "Stopping CPU load..."
    xargs kill < /tmp/load_pids.txt
    rm /tmp/load_pids.txt
    echo "Load stopped."
  else
    echo "No load running."
  fi
}

case "$2" in
  start) start_load ;;
  stop)  stop_load ;;
  *)
    echo "Usage:"
    echo "  ./load_toggle.sh [num_cores] start   # Start load"
    echo "  ./load_toggle.sh 0(any int) stop                # Stop load"
    ;;
esac
