import inet.node.ethernet.Eth100G;

network TestRoceNetworkIntegration
{
  parameters:
    string simulation_config;

  submodules:
    router: NetworkRouter {
      router_name = "router";
      num_port = 3;
    }
    host[2]: RoceHost;

  connections:
    host[0].in <-- Eth100G <-- router.port$o[0];
    host[0].out --> Eth100G --> router.port$i[0];
    host[1].in <-- Eth100G <-- router.port$o[1];
    host[1].out --> Eth100G --> router.port$i[1];
}
