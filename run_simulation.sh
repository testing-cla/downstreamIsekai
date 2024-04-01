#!/bin/bash

#######################################
# Script to run Isekai simulations.
# Input parameters:
#   -f <simulation config ini file>
#   -n <NED file search paths>
#   -c <config section>
#   -o <simulation result output dir>
#   <simulation config ini file> is required, the other parameters are optional.
#######################################

while getopts f:n:c:o: flag; do
  case "${flag}" in
    f) config_file=${OPTARG};;
    n) ned_paths=${OPTARG};;
    c) config_section=${OPTARG};;
    o) result_dir=${OPTARG};;
  esac
done

# Checks if simulation config file is provided.
if [[ -z "${config_file}" ]]; then
  echo "No simulation config file is provided..."
  exit 1
fi
config_file="$(realpath ${config_file})"

# Sets the config secton to run.
if [[ -z "${config_section}" ]]; then
  config_section="General"
fi

# Sets the simulation result output dir.
if [[ -z "${result_dir}" ]]; then
  simulation_name=$(basename ${config_file})
  result_dir="$(realpath ./output-${simulation_name%.*})"
fi

# Sets up the NED file search paths and simulation binary path.
repo_root="$(pwd)"
src_path=${repo_root}"/bazel-$(basename ${repo_root})"
binary_path=${repo_root}"/bazel-bin/isekai/testing/integration/isekai_simulation_main"
sim_ned_paths=${repo_root}"/isekai/testing/integration/"
sim_ned_paths+=":"${repo_root}"/isekai/fabric/"
sim_ned_paths+=":"${src_path}"/external/inet/src/"
for ned_path in $(echo ${ned_paths} | tr ":" "\n"); do
  sim_ned_paths+=":$(realpath ${ned_path})"
done

# Builds the simulation binary.
rm -f ${binary_path}
bazel build -c opt isekai/testing/integration:isekai_simulation_main --test_output=all
if [ ! -f "${binary_path}" ]; then
  echo "Fail to generate the binary."
  exit 2
fi

# Runs the simulation.
${binary_path} -f ${config_file} -n ${sim_ned_paths} -c ${config_section} --result-dir=${result_dir} -m

# Prints the simulation configs upon success.
printf "\n\n###Simulation Succeeds###\nConfig file: ${config_file}\nSimulation results in: ${result_dir}\n##########\n\n"

