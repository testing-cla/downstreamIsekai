## Isekai README

**Falcon** is a hardware reliable transport supporting lossy networks and programmable congestion control using [Swift](https://dl.acm.org/doi/10.1145/3387514.3406591). It is targeted to be used for RDMA and gNVMe. Models of Falcon in Isekai simulator support key features such as Reliability, Congestion Control, and Solicitation. The model does not yet support RNR Nacks, and ReSync.

**Isekai** is a network simulator that includes models of a  NIC, switch and network topology.  It includes models for RDMA and Falcon. RDMA models are infinitely fast while Falcon models are accurate timing models, specifically with queuing and scheduling points accurately modeled. Neither models support caching (QP and connection caches), or other NIC bottlenecks such as PCIe.

Isekai traffic generator is able to create synthetic traffic models with Poisson and Uniform arrivals of RDMA Ops, and arbitrary traffic patterns.

Stats collection is a mix of OMNest-based statistics as well as histograms based on [TDigest](https://github.com/facebook/folly/blob/main/folly/stats/TDigest.h).

#### What you need to build Isekai
- **[Bazel](https://bazel.build/)**
- **[Python-dev](https://stackoverflow.com/questions/6230444/how-to-install-python-developer-package)**

#### External Dependencies (automatially download and compile)
- rules_foreign_cc
- ABSL
- bison
- flex
- libxml
- m4 (if not installed in the system)
- OMNetpp (requires commercial license)
- INET
- CRC32
- Folly
- Protobuf
- TensorFlow
- highwayhash
- glog
- Googletest
- Riegeli
- Brotli
- Zstd
- Snappy
- cel-cpp

#### Steps to build Isekai

Falcon is built and tested under the Linux environment. Run the command under the root directory of the Isekai code base. It may take 5-10 mins to build for the first time.

```
bazel build -c opt "..."
```

#### Running Unit Tests

To verify the functionalities of Isekai, please run the following command under the root directory of the Isekai code base to execute all the unit tests.

```
bazel test -c opt "..."
```

#### Sample Simulation Steps

We provide two experiment scenarios under \<Isekai root directory>/examples/. One simulation is a 24-to-24 uniform random traffic pattern with 120Gbps offered load, READ Op of 128KB size in a single rack. The second simulation is a 10-to-1 incast traffic pattern with WRITE op of 128KB size in a superblock.

A shell script named “run_simulation.sh” under the root directory of the Isekai code base is to help execute simulations. For example, to run the above 24-to-24 uniform random simulation, use the command.

```
sh run_simulation.sh -f examples/single_rack_simulation.ini -n examples/
```

The complete usage of the script is as below:

| Args | Required |
| ---- | -------- |
| -f <simulation config ini file> | Yes |
| -n <NED file search paths> | No |
| -c <config section> | No |
| -o <simulation result output dir> | No |

#### Simulation Results
Once the simulation finishes, it will show where the simulation results are stored. For example:

> \###Simulation Succeeds### \
> Config file: /falcon-network-simulator/examples/single_rack_simulation.ini \
> Simulation results in: /falcon-network-simulator/output-single_rack_simulation
> \##########

Under the folder storing the simulation results, there are several types of files:
* .sca file stores the scalar value of the collected statistics. See [here](https://doc.omnetpp.org/omnetpp/manual/#sec:ana-sim:scalar-result-files) for more information.
* .vec file stores the time series data. See [here](https://doc.omnetpp.org/omnetpp/manual/#sec:ana-sim:output-vector-files) for more information.
* <host_id>_falcon_latency_histograms stores the breakdown latency inside the Falcon protocol.
* <host_id>_rdma_latency_histograms stores the breakdown latency inside the RDMA.
