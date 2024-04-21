# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
from naja import snl

class DesignStats:
  def __init__(self):
    self.assigns = 0
    self.flat_assigns = 0
    self.primitives = dict()
    self.flat_primitives = dict()
    self.blackboxes = dict()
    self.flat_blackboxes = dict()
    self.ins = dict()
    self.flat_ins = dict()
    self.terms = dict()
  def add_ins_stats(self, ins_stats):
    self.flat_assigns += ins_stats.flat_assigns
    for ins, nb in ins_stats.flat_ins.items():
      self.flat_ins[ins] = self.flat_ins.get(ins, 0) + nb
    for ins, nb in ins_stats.flat_blackboxes.items():
      self.flat_blackboxes[ins] = self.flat_blackboxes.get(ins, 0) + nb
    for primitive, nb in ins_stats.flat_primitives.items():
      self.flat_primitives[primitive] = self.flat_primitives.get(primitive, 0) + nb

def compute_design_stats(design, designs_stats):
  if design in designs_stats:
    return designs_stats.get(design)
  design_stats = DesignStats()
  for ins in design.getInstances():
    model = ins.getModel()
    if model.isAssign():
      design_stats.assigns += 1
      design_stats.flat_assigns += 1
    elif model.isPrimitive():
      design_stats.primitives[model] = design_stats.primitives.get(model, 0) + 1
      design_stats.flat_primitives[model] = design_stats.flat_primitives.get(model, 0) + 1
    elif model.isBlackBox():
      design_stats.blackboxes[model] = design_stats.blackboxes.get(model, 0) + 1
      design_stats.flat_blackboxes[model] = design_stats.flat_blackboxes.get(model, 0) + 1
    else:
      if model in designs_stats:
        model_stats = designs_stats[model]
      else:
        model_stats = compute_design_stats(model, designs_stats)
      design_stats.ins[model] = design_stats.ins.get(model, 0) + 1
      design_stats.flat_ins[model] = design_stats.flat_ins.get(model, 0) + 1
      design_stats.add_ins_stats(model_stats)
  for term in design.getTerms():
    if term.getDirection() == snl.SNLTerm.Direction.Input:
      design_stats.terms["inputs"] = design_stats.terms.get("inputs", 0) + 1
    elif term.getDirection() == snl.SNLTerm.Direction.Output:
      design_stats.terms["outputs"] = design_stats.terms.get("outputs", 0) + 1
    elif term.getDirection() == snl.SNLTerm.Direction.InOut:
      design_stats.terms["inouts"] = design_stats.terms.get("inouts", 0) + 1
    else:
      design_stats.terms["unknowns"] = design_stats.terms.get("unknowns", 0) + 1
  designs_stats[design] = design_stats
  return design_stats

def dump_instances(stats_file, title, instances):
  if len(instances) == 0:
    return
  sorted_instances = sorted(instances.items(), key=lambda item: item[0].getName())
  stats_file.write(title + ' ' + str(sum(j for i, j in sorted_instances)) + '\n')
  line_char = 0
  for instance in sorted_instances:
    if line_char!=0:
      stats_file.write(",")
      line_char += 1
    if line_char>80:
      stats_file.write('\n')
      line_char = 0
    elif line_char!=0:
      stats_file.write(" ")
      line_char += 1
    instance_char =  instance[0].getName() + ":" + str(instance[1])
    line_char += len(instance_char)
    stats_file.write(instance_char)
  stats_file.write('\n\n')

def dump_stats(design, stats_file, designs_stats, dumped_models):
  if design.isPrimitive() or design.isBlackBox():
    return
  if design in dumped_models:
    return
  dumped_models.add(design)
  stats_file.write('*** ' + design.getName() + ' ***\n')
  design_stats = designs_stats.get(design)
  if design_stats is None:
    print('Cannot find ' + str(design) + ' in design_stats')
    raise
  if len(design_stats.terms) > 0:
    stats_file.write("Terms: ")
    first = True
    for terms in design_stats.terms.items():
      if not first:
        stats_file.write(', ')
      else:
        first = False
      stats_file.write(terms[0] + ':' + str(terms[1]))
    stats_file.write("\n")
    
  dump_instances(stats_file, 'Instances:', design_stats.ins)
  dump_instances(stats_file, 'Primitives:', design_stats.primitives)
  dump_instances(stats_file, 'Blackboxes:', design_stats.blackboxes)
  if design_stats.assigns > 0:
    stats_file.write('Assigns: ' + str(design_stats.assigns) + '\n')
  dump_instances(stats_file, 'Flat Blackboxes:', design_stats.flat_blackboxes)
  dump_instances(stats_file, 'Flat Instances:', design_stats.flat_ins)
  dump_instances(stats_file, 'Flat Primitives:', design_stats.flat_primitives)
  if design_stats.flat_assigns > 0:
    stats_file.write('Flat Assigns: ' + str(design_stats.flat_assigns) + '\n')
  stats_file.write('\n')
  for ins in design.getInstances():
    model = ins.getModel()
    dump_stats(model, stats_file, designs_stats, dumped_models) 

def dump_constants(design, analyzed_models):
  if design.isPrimitive():
    return
  if design in analyzed_models:
    return
  analyzed_models.add(design)
  for bitnet in design.getBitNets():
    if bitnet.isConstant():
      logging.info('In design ' + design.getName() + ', constant net ' + bitnet.getName() + ' of type ' + bitnet.getTypeAsString())
      if all(False for _ in bitnet.getComponents()):
        logging.info(' with zero connections\n')
      else:
        logging.info(' connected to:\n')
        for component in bitnet.getComponents():
          logging.info(str(component) + '\n')

  for ins in design.getInstances():
    model = ins.getModel()
    dump_constants(model, analyzed_models)
