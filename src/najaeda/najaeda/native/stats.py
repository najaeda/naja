# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
from najaeda import naja


class DesignsStats:
    def __init__(self):
        self.blackboxes = dict()
        self.hier_designs = dict()


class DesignStats:
    def __init__(self):
        self.name = ""
        self.assigns = 0
        self.flat_assigns = 0
        self.basic_primitives = dict()
        self.primitives = dict()
        self.flat_basic_primitives = dict()
        self.flat_primitives = dict()
        self.blackboxes = dict()
        self.flat_blackboxes = dict()
        self.ins = dict()
        self.flat_ins = dict()
        self.terms = dict()
        self.bit_terms = dict()
        self.net_stats = dict()

    def add_ins_stats(self, ins_stats):
        self.flat_assigns += ins_stats.flat_assigns
        for ins, nb in ins_stats.flat_ins.items():
            self.flat_ins[ins] = self.flat_ins.get(ins, 0) + nb
        for ins, nb in ins_stats.flat_blackboxes.items():
            self.flat_blackboxes[ins] = self.flat_blackboxes.get(ins, 0) + nb
        for primitive, nb in ins_stats.flat_primitives.items():
            self.flat_primitives[primitive] = (
                self.flat_primitives.get(primitive, 0) + nb
            )
        for primitive, nb in ins_stats.flat_basic_primitives.items():
            self.flat_basic_primitives[primitive] = (
                self.flat_basic_primitives.get(primitive, 0) + nb
            )


def isBasicPrimitive(design):
    return design.isConst0() or design.isConst1() or design.isBuf() or design.isInv()


def compute_design_stats(design, designs_stats):
    if design in designs_stats.hier_designs:
        return designs_stats.hier_designs.get(design)
    design_stats = DesignStats()
    design_stats.name = design.getName()
    for ins in design.getInstances():
        model = ins.getModel()
        if model.isAssign():
            design_stats.assigns += 1
            design_stats.flat_assigns += 1
        elif model.isPrimitive():
            if isBasicPrimitive(model):
                design_stats.basic_primitives[model] = (
                    design_stats.basic_primitives.get(model, 0) + 1
                )
                design_stats.flat_basic_primitives[model] = (
                    design_stats.flat_basic_primitives.get(model, 0) + 1
                )
            else:
                design_stats.primitives[model] = (
                    design_stats.primitives.get(model, 0) + 1
                )
                design_stats.flat_primitives[model] = (
                    design_stats.flat_primitives.get(model, 0) + 1
                )
        elif model.isBlackBox():
            design_stats.blackboxes[model] = design_stats.blackboxes.get(model, 0) + 1
            design_stats.flat_blackboxes[model] = (
                design_stats.flat_blackboxes.get(model, 0) + 1
            )
            if model not in designs_stats.blackboxes:
                designs_stats.blackboxes[model] = dict()
                compute_design_terms(model, designs_stats.blackboxes[model])
        else:
            if model in designs_stats.hier_designs:
                model_stats = designs_stats.hier_designs[model]
            else:
                model_stats = compute_design_stats(model, designs_stats)
            design_stats.ins[model] = design_stats.ins.get(model, 0) + 1
            design_stats.flat_ins[model] = design_stats.flat_ins.get(model, 0) + 1
            design_stats.add_ins_stats(model_stats)
    compute_design_terms(design, design_stats)
    compute_design_net_stats(design, design_stats)
    designs_stats.hier_designs[design] = design_stats
    return design_stats


def compute_design_terms(design, design_stats):
    for term in design.getTerms():
        if term.getDirection() == naja.SNLTerm.Direction.Input:
            design_stats.terms["inputs"] = design_stats.terms.get("inputs", 0) + 1
            bit_terms = sum(1 for _ in term.getBits())
            design_stats.bit_terms["inputs"] = (
                design_stats.bit_terms.get("inputs", 0) + bit_terms
            )
        elif term.getDirection() == naja.SNLTerm.Direction.Output:
            design_stats.terms["outputs"] = design_stats.terms.get("outputs", 0) + 1
            bit_terms = sum(1 for _ in term.getBits())
            design_stats.bit_terms["outputs"] = (
                design_stats.bit_terms.get("outputs", 0) + bit_terms
            )
        elif term.getDirection() == naja.SNLTerm.Direction.InOut:
            design_stats.terms["inouts"] = design_stats.terms.get("inouts", 0) + 1
            bit_terms = sum(1 for _ in term.getBits())
            design_stats.bit_terms["inouts"] = (
                design_stats.bit_terms.get("inouts", 0) + bit_terms
            )
        else:
            design_stats.terms["unknowns"] = design_stats.terms.get("unknowns", 0) + 1
            bit_terms = sum(1 for _ in term.getBits())
            design_stats.bit_terms["unknowns"] = (
                design_stats.bit_terms.get("unknowns", 0) + bit_terms
            )


def compute_design_net_stats(design, design_stats):
    for net in design.getBitNets():
        if net.isConstant():
            pass
        nb_components = sum(1 for c in net.getComponents())
        design_stats.net_stats[nb_components] = (
            design_stats.net_stats.get(nb_components, 0) + 1
        )
        design_stats.net_stats = dict(sorted(design_stats.net_stats.items()))


def dump_instances(stats_file, title, instances):
    if len(instances) == 0:
        return
    sorted_instances = sorted(instances.items(), key=lambda item: item[0].getName())
    stats_file.write(title + " " + str(sum(j for i, j in sorted_instances)) + "\n")
    line_char = 0
    for instance in sorted_instances:
        if line_char != 0:
            stats_file.write(",")
            line_char += 1
        if line_char > 80:
            stats_file.write("\n")
            line_char = 0
        elif line_char != 0:
            stats_file.write(" ")
            line_char += 1
        instance_char = instance[0].getName() + ":" + str(instance[1])
        line_char += len(instance_char)
        stats_file.write(instance_char)
    stats_file.write("\n\n")


def dump_blackboxes_stats(stats_file, design_stats):
    if len(design_stats.blackboxes) > 0:
        stats_file.write("*** BlackBoxes ***\n")
        for bbox in design_stats.blackboxes.items():
            design = bbox[0]
            design_terms = bbox[1]
            stats_file.write("*** " + design.getName() + " ***\n")
            if len(design_terms) > 0:
                stats_file.write("Terms: ")
                first = True
                for terms in design_terms.items():
                    if not first:
                        stats_file.write(", ")
                    else:
                        first = False
                    stats_file.write(terms[0] + ":" + str(terms[1]))
                stats_file.write("\n")
            stats_file.write("\n")


def dump_stats(design, stats_file, designs_stats, dumped_models):
    if design.isPrimitive() or design.isBlackBox():
        return
    if design in dumped_models:
        return
    dumped_models.add(design)
    stats_file.write("*** " + design.getName() + " ***\n")
    design_stats = designs_stats.hier_designs.get(design)
    if design_stats is None:
        print("Cannot find " + str(design) + " in design_stats")
        raise
    if len(design_stats.terms) > 0:
        stats_file.write("Terms: ")
        first = True
        for terms in design_stats.terms.items():
            if not first:
                stats_file.write(", ")
            else:
                first = False
            stats_file.write(terms[0] + ":" + str(terms[1]))
        stats_file.write("\n")

    dump_instances(stats_file, "Instances:", design_stats.ins)
    nb_primitives = sum(design_stats.basic_primitives.values()) + sum(
        design_stats.primitives.values()
    )
    if nb_primitives > 1:
        stats_file.write("Primitives: " + str(nb_primitives) + "\n")
    dump_instances(stats_file, "Simple Primitives:", design_stats.basic_primitives)
    dump_instances(stats_file, "Other Primitives:", design_stats.primitives)
    dump_instances(stats_file, "Blackboxes:", design_stats.blackboxes)
    if design_stats.assigns > 0:
        stats_file.write("Assigns: " + str(design_stats.assigns) + "\n")
    dump_instances(stats_file, "Flat Instances:", design_stats.flat_ins)
    dump_instances(stats_file, "Flat Blackboxes:", design_stats.flat_blackboxes)
    nb_primitives = sum(design_stats.flat_basic_primitives.values()) + sum(
        design_stats.flat_primitives.values()
    )
    if nb_primitives > 1:
        stats_file.write("Flat Primitives: " + str(nb_primitives) + "\n")
    dump_instances(
        stats_file, "Flat Simple Primitives:", design_stats.flat_basic_primitives
    )
    dump_instances(stats_file, "Flat Other Primitives:", design_stats.flat_primitives)
    if design_stats.flat_assigns > 0:
        stats_file.write("Flat Assigns: " + str(design_stats.flat_assigns) + "\n")
    stats_file.write("\n")
    for ins in design.getInstances():
        model = ins.getModel()
        dump_stats(model, stats_file, designs_stats, dumped_models)


def dump_pandas(designs_stats):
    import pandas
    import matplotlib.pyplot as plt

    # create a figures directory erase the previous one
    if os.path.exists("figures"):
        import shutil

        shutil.rmtree("figures")
    os.makedirs("figures")

    data = []
    for design, design_stats in designs_stats.hier_designs.items():
        data.append(
            [
                design.getName(),
                sum(design_stats.terms.values()),
                sum(design_stats.bit_terms.values()),
                sum(design_stats.basic_primitives.values()),
                sum(design_stats.primitives.values()),
                sum(design_stats.blackboxes.values()),
                sum(design_stats.ins.values()),
                sum(design_stats.flat_ins.values()),
                sum(design_stats.flat_blackboxes.values()),
                sum(design_stats.flat_basic_primitives.values()),
                sum(design_stats.flat_primitives.values()),
            ]
        )
    df = pandas.DataFrame(
        data,
        columns=[
            "Design",
            "Terms",
            "Bit Terms",
            "Basic Primitives",
            "Primitives",
            "Blackboxes",
            "Instances",
            "Flat Instances",
            "Flat Blackboxes",
            "Flat Basic Primitives",
            "Flat Primitives",
        ],
    )
    df.to_csv("figures/designs_stats.csv", index=False)

    net_series = pandas.Series(design_stats.net_stats)
    nets_plot = net_series.plot(
        kind="bar",
        title="Number of nets with a given number of components for\n"
        + design.getName(),
        xlabel="number of components",
        ylabel="number of nets",
    )
    nets_plot.set_yscale("log")
    nets_plot.xaxis.set_major_locator(plt.MaxNLocator(100))
    nets_figure = nets_plot.get_figure()
    nets_figure.tight_layout()
    nets_figure.savefig("figures/nets_" + design.getName() + ".png")

    flat_primitives_series = pandas.Series(design_stats.flat_primitives)
    primitives_plot = flat_primitives_series.plot(
        kind="bar",
        title="Number of  primitives for\n" + design.getName(),
        xlabel="primitive",
        ylabel="number of flat instances",
    )
    primitives_plot.set_yscale("log")
    primitives_figure = primitives_plot.get_figure()
    primitives_figure.tight_layout()
    primitives_figure.savefig("figures/flat_primitives_" + design.getName() + ".png")


def compute_and_dump_design_stats(design, stats_file, with_pandas=False):
    designs_stats = DesignsStats()
    compute_design_stats(design, designs_stats)
    dumped_models = set()
    dump_stats(design, stats_file, designs_stats, dumped_models)
    dump_blackboxes_stats(stats_file, designs_stats)
    if with_pandas:
        dump_pandas(designs_stats)


def dump_constants(design, analyzed_models):
    if design.isPrimitive():
        return
    if design in analyzed_models:
        return
    analyzed_models.add(design)
    for bitnet in design.getBitNets():
        if bitnet.isConstant():
            logging.info(
                "In design "
                + design.getName()
                + ", constant net "
                + bitnet.getName()
                + " of type "
                + bitnet.getTypeAsString()
            )
            if all(False for _ in bitnet.getComponents()):
                logging.info(" with zero connections\n")
            else:
                logging.info(" connected to:\n")
                for component in bitnet.getComponents():
                    logging.info(str(component) + "\n")

    for ins in design.getInstances():
        model = ins.getModel()
        dump_constants(model, analyzed_models)
