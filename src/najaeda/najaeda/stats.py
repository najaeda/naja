# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
import json

from najaeda import netlist


class InstancesStats:
    def __init__(self):
        self.blackboxes = dict()
        self.hier_instances = dict()

    def dump_instance_stats_text(self, instance, stats_files):
        dumped_models = set()
        self.__dump_instance_stats_text(instance, stats_files, dumped_models)

    def __dump_stats(self, file, value_name, value):
        file.write(f"{value_name}: {value} \n")

    def __dump_terms_stats_text(self, instance_stats, file):
        file.write("Terms: ")
        first = True
        for terms in instance_stats.terms.items():
            if not first:
                file.write(", ")
            else:
                first = False
            file.write(terms[0] + ":" + str(terms[1]))
        file.write("\n")

    def __dump_instances(self, stats_file, title, instances):
        if len(instances) == 0:
            return
        sorted_instances = sorted(
            instances.items(), key=lambda item: netlist.get_model_name(item[0])
        )
        stats_file.write(title + " " + str(sum(j for i, j in sorted_instances)) + "\n")
        line_char = 0
        for instance in sorted_instances:
            model_name = netlist.get_model_name(instance[0])
            if line_char != 0:
                stats_file.write(",")
                line_char += 1
            if line_char > 80:
                stats_file.write("\n")
                line_char = 0
            elif line_char != 0:
                stats_file.write(" ")
                line_char += 1
            instance_char = model_name + ":" + str(instance[1])
            line_char += len(instance_char)
            stats_file.write(instance_char)
        stats_file.write("\n\n")

    def __dump_instance_stats_text(self, instance, file, dumped_models):
        if instance.is_primitive() or instance.is_blackbox():
            return
        # Only dump once each model
        model_id = instance.get_model_id()
        if model_id in dumped_models:
            return
        dumped_models.add(model_id)

        file.write("*** " + instance.get_name() + " ***\n")

        instance_stats = self.hier_instances.get(model_id)
        if instance_stats is None:
            print(
                "Cannot find " + str(instance.get_model_name()) + " in instance_stats"
            )
            raise

        if len(instance_stats.terms) > 0:
            self.__dump_terms_stats_text(instance_stats, file)

        # self.__dump_instances(stats_file, "Instances:", design_stats.ins)
        nb_primitives = sum(instance_stats.basic_primitives.values()) + sum(
            instance_stats.primitives.values()
        )
        if nb_primitives > 1:
            self.__dump_stats(file, "Primitives", nb_primitives)

        self.__dump_instances(
            file, "Simple Primitives:", instance_stats.basic_primitives
        )
        self.__dump_instances(file, "Other Primitives:", instance_stats.primitives)
        self.__dump_instances(file, "Blackboxes:", instance_stats.blackboxes)

        if instance_stats.assigns > 0:
            self.__dump_stats(file, "Assigns", instance_stats.assigns)

        self.__dump_instances(file, "Flat Instances:", instance_stats.flat_ins)
        self.__dump_instances(file, "Flat Blackboxes:", instance_stats.flat_blackboxes)

        nb_primitives = sum(instance_stats.flat_basic_primitives.values()) + sum(
            instance_stats.flat_primitives.values()
        )
        if nb_primitives > 1:
            self.__dump_stats(file, "Flat Primitives", nb_primitives)

        self.__dump_instances(
            file, "Flat Simple Primitives:", instance_stats.flat_basic_primitives
        )
        self.__dump_instances(
            file, "Flat Other Primitives:", instance_stats.flat_primitives
        )

        if instance_stats.flat_assigns > 0:
            self.__dump_stats(file, "Flat Assigns", instance_stats.flat_assigns)

        file.write("\n")

        for child_instance in instance.get_child_instances():
            self.__dump_instance_stats_text(child_instance, file, dumped_models)


class InstanceStats:
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


def is_basic_primitive(design):
    return (
        design.is_const0() or design.is_const1() or design.is_buf() or design.is_inv()
    )


def compute_instance_stats(instance, instances_stats):
    if instance.get_model_id() in instances_stats.hier_instances:
        return instances_stats.hier_instances.get(instance.get_model_id())
    instance_stats = InstanceStats()
    instance_stats.name = instance.get_model_name()
    for ins in instance.get_child_instances():
        model_id = ins.get_model_id()
        if ins.is_assign():
            instance_stats.assigns += 1
            instance_stats.flat_assigns += 1
        elif ins.is_primitive():
            if is_basic_primitive(ins):
                instance_stats.basic_primitives[model_id] = (
                    instance_stats.basic_primitives.get(model_id, 0) + 1
                )
                instance_stats.flat_basic_primitives[model_id] = (
                    instance_stats.flat_basic_primitives.get(model_id, 0) + 1
                )
            else:
                instance_stats.primitives[model_id] = (
                    instance_stats.primitives.get(model_id, 0) + 1
                )
                instance_stats.flat_primitives[model_id] = (
                    instance_stats.flat_primitives.get(model_id, 0) + 1
                )
        elif ins.is_blackbox():
            instance_stats.blackboxes[model_id] = (
                instance_stats.blackboxes.get(model_id, 0) + 1
            )
            instance_stats.flat_blackboxes[model_id] = (
                instance_stats.flat_blackboxes.get(model_id, 0) + 1
            )
            if model_id not in instance_stats.blackboxes:
                instance_stats.blackboxes[model_id] = dict()
                compute_instance_terms(model_id, instance_stats.blackboxes[model_id])
        else:
            if model_id in instances_stats.hier_instances:
                model_stats = instances_stats.hier_instances[model_id]
            else:
                model_stats = compute_instance_stats(ins, instances_stats)
            instance_stats.ins[model_id] = instance_stats.ins.get(model_id, 0) + 1
            instance_stats.flat_ins[model_id] = (
                instance_stats.flat_ins.get(model_id, 0) + 1
            )
            instance_stats.add_ins_stats(model_stats)
    compute_instance_terms(instance, instance_stats)
    compute_instance_net_stats(instance, instance_stats)
    instances_stats.hier_instances[instance.get_model_id()] = instance_stats
    return instance_stats


def compute_instance_terms(instance, instance_stats):
    for term in instance.get_terms():
        if term.get_direction() == netlist.Term.INPUT:
            instance_stats.terms["inputs"] = instance_stats.terms.get("inputs", 0) + 1
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["inputs"] = (
                instance_stats.bit_terms.get("inputs", 0) + bit_terms
            )
        elif term.get_direction() == netlist.Term.OUTPUT:
            instance_stats.terms["outputs"] = instance_stats.terms.get("outputs", 0) + 1
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["outputs"] = (
                instance_stats.bit_terms.get("outputs", 0) + bit_terms
            )
        elif term.get_direction() == netlist.Term.INOUT:
            instance_stats.terms["inouts"] = instance_stats.terms.get("inouts", 0) + 1
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["inouts"] = (
                instance_stats.bit_terms.get("inouts", 0) + bit_terms
            )
        else:
            instance_stats.terms["unknowns"] = (
                instance_stats.terms.get("unknowns", 0) + 1
            )
            bit_terms = sum(1 for _ in term.get_bits())
            instance_stats.bit_terms["unknowns"] = (
                instance_stats.bit_terms.get("unknowns", 0) + bit_terms
            )


def compute_instance_net_stats(instance, instance_stats):
    for net in instance.get_flat_nets():
        if net.is_const():
            pass
        nb_components = sum(1 for c in net.get_terms())
        instance_stats.net_stats[nb_components] = (
            instance_stats.net_stats.get(nb_components, 0) + 1
        )
        instance_stats.net_stats = dict(sorted(instance_stats.net_stats.items()))


def dump_instances(stats_file, title, instances):
    if len(instances) == 0:
        return
    sorted_instances = sorted(
        instances.items(), key=lambda item: netlist.get_model_name(item[0])
    )
    stats_file.write(title + " " + str(sum(j for i, j in sorted_instances)) + "\n")
    line_char = 0
    for instance in sorted_instances:
        model_name = netlist.get_model_name(instance[0])
        if line_char != 0:
            stats_file.write(",")
            line_char += 1
        if line_char > 80:
            stats_file.write("\n")
            line_char = 0
        elif line_char != 0:
            stats_file.write(" ")
            line_char += 1
        instance_char = model_name + ":" + str(instance[1])
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


# def dump_pandas(designs_stats):
#  import pandas
#  import matplotlib.pyplot as plt
#
#  #create a figures directory erase the previous one
#  if os.path.exists('figures'):
#    import shutil
#    shutil.rmtree('figures')
#  os.makedirs('figures')
#
#  data = []
#  for design, design_stats in designs_stats.hier_designs.items():
#    data.append([
#      design.getName(),
#      sum(design_stats.terms.values()),
#      sum(design_stats.bit_terms.values()),
#      sum(design_stats.basic_primitives.values()),
#      sum(design_stats.primitives.values()),
#      sum(design_stats.blackboxes.values()),
#      sum(design_stats.ins.values()),
#      sum(design_stats.flat_ins.values()),
#      sum(design_stats.flat_blackboxes.values()),
#      sum(design_stats.flat_basic_primitives.values()),
#      sum(design_stats.flat_primitives.values())])
#  df = pandas.DataFrame(data, columns=[
#    'Design', 'Terms', 'Bit Terms',
#    'Basic Primitives', 'Primitives', 'Blackboxes', 'Instances',
#    'Flat Instances', 'Flat Blackboxes',
#    'Flat Basic Primitives', 'Flat Primitives'])
#  df.to_csv('figures/designs_stats.csv', index=False)
#
#  net_series = pandas.Series(design_stats.net_stats)
#  nets_plot = net_series.plot(kind='bar',
#      title='Number of nets with a given number of components for\n' + design.getName(),
#      xlabel='number of components', ylabel='number of nets')
#  nets_plot.set_yscale('log')
#  nets_plot.xaxis.set_major_locator(plt.MaxNLocator(100))
#  nets_figure = nets_plot.get_figure()
#  nets_figure.tight_layout()
#  nets_figure.savefig('figures/nets_' + design.getName() + '.png')
#
#  flat_primitives_series = pandas.Series(design_stats.flat_primitives)
#  primitives_plot = flat_primitives_series.plot(kind='bar',
#                  title='Number of  primitives for\n' + design.getName(),
#                  xlabel='primitive', ylabel='number of flat instances')
#  primitives_plot.set_yscale('log')
#  primitives_figure = primitives_plot.get_figure()
#  primitives_figure.tight_layout()
#  primitives_figure.savefig('figures/flat_primitives_' + design.getName() + '.png')


def convert_instance_stats_to_json(instance_stats):
    json_top = list()
    for _, value in instance_stats.hier_instances.items():
        nb_primitives = sum(value.basic_primitives.values())
        +sum(value.primitives.values())
        nb_terms = sum(value.terms.values())
        nb_nets = sum(value.net_stats.keys())
        nb_ins = sum(value.ins.values()) + nb_primitives
        json_top.append(
            {
                "Name": value.name,
                "primitives": nb_primitives,
                "instances": nb_ins,
                "terms": nb_terms,
                "nets": nb_nets,
                # "primitives": value.primitives,
                # "flat_basic_primitives": value.flat_basic_primitives,
                # "flat_primitives": value.flat_primitives,
                # "blackboxes": value.blackboxes,
                # "flat_blackboxes": value.flat_blackboxes,
                # "ins": value.ins,
                # "flat_ins": value.flat_ins,
                # "terms": value.terms,
                # "bit_terms": value.bit_terms,
                # "net_stats": value.net_stats,
            }
        )
    return json_top


def dump_instance_stats_json(instance, stats_file):
    # stats_files = [(InstanceStats.ReportType.JSON, stats_file)]
    # stats_file.write("[\n")
    # dump_instance_stats(design, stats_files)
    # stats_file.write("]")
    instances_stats = InstancesStats()
    compute_instance_stats(instance, instances_stats)
    json_dict = convert_instance_stats_to_json(instances_stats)
    json.dump(json_dict, stats_file, indent=4)


def dump_instance_stats_text(instance, file):
    instances_stats = InstancesStats()
    compute_instance_stats(instance, instances_stats)
    instances_stats.dump_instance_stats_text(instance, file)


def dump_constants(design, analyzed_models):
    if design.isPrimitive():
        return
    if design in analyzed_models:
        return
    analyzed_models.add(design)
    for bitnet in design.getBitNets():
        if bitnet.isConstant():
            message = f"In design {design.getName()}, \
                    constant net {bitnet.getName()} \
                    of type {bitnet.getTypeAsString()}"
            logging.info(message)
            if all(False for _ in bitnet.getComponents()):
                logging.info(" with zero connections\n")
            else:
                logging.info(" connected to:\n")
                for component in bitnet.getComponents():
                    logging.info(str(component) + "\n")

    for ins in design.getInstances():
        model = ins.getModel()
        dump_constants(model, analyzed_models)
