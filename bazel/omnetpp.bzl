"""Build rules for OMNeT++."""

def omnetpp_msg(name, srcs, import_paths = [], **kwargs):
    header_names = [src.replace(".msg", "_m.h") for src in srcs]
    source_names = [src.replace(".msg", "_m.cc") for src in srcs]

    native.genrule(
        name = name,
        srcs = srcs,
        outs = header_names + source_names,
        cmd = """
          $(location @omnetpp//:opp_msgc_wrapper) "$(SRCS)" "$(GENDIR)" {0} "$(location @omnetpp//:opp_msgc)"
        """.format('""' + ",".join(["$(RULEDIR)/" + path for path in import_paths])),
        tools = [
            "@omnetpp//:opp_msgc",
            "@omnetpp//:opp_msgc_wrapper",
        ],
    )
