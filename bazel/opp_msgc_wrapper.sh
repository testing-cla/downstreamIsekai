#!/bin/bash

set -e

readarray -td, srcs <<< $1;
readarray -td, import_paths <<< $3;
output_path=$2;
opp_msgc_path=$4;

function main() {
  # Copy all sources to output directory, since opp_msgc always puts its output
  # in the same directory as the source file (and this is not configurable).
  for src in ${srcs[@]}; do
    mkdir -p $(dirname "${output_path}/${src}")
    cp "${src}" "${output_path}/${src}"
  done

  # Set up compiler flags.
  flags="--msg6"
  for import_path in ${import_paths[@]}; do
    flags+=" -I${import_path}"
  done

  # Invoke compiler for each source file.
  for src in ${srcs[@]}; do
    "${opp_msgc_path}" ${flags} "${output_path}/${src}"
  done
}

main
