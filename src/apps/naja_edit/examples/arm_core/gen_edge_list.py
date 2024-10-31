import logging
from naja import snl

def edit():
  logging.basicConfig(filename='edit.log', filemode='w' ,level=logging.DEBUG)
  universe = snl.SNLUniverse.get()
  if universe is None:
    logging.critical('No loaded SNLUniverse')
    return 1
  top = universe.getTopDesign()
  if top is None:
    logging.critical('SNLUniverse does not contain any top SNLDesign')
    return 1
  else:
    logging.info('Found top design ' + str(top))
  
  edges_file = open('edges.list','w')
  nodes_file = open('nodes.list', 'w')
  global_component_id = 0
  components = dict()
  for net in top.getBitNets():
    edge = set()
    first = True
    for component in net.getComponents():
      if isinstance(component, snl.SNLInstTerm):
        instance = component.getInstance()
        edge.add(instance)
      else:
        #term
        edge.add(component)
    if len(edge)>1:
      for component in edge:
        component_id = components.get(component)
        if component_id == None:
          component_id = global_component_id
          nodes_file.write(str(component))
          nodes_file.write('\n')
          global_component_id += 1
          components[component] = component_id
        edges_file.write(str(component_id))
        edges_file.write(" ")
      edges_file.write("\n")
