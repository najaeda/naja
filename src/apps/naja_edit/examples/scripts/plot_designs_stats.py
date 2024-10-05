from naja import snl
import pandas as pd

def plot_design_stats(library):
    for design in library:
        nb_terms = len(design.getBitTerms())
        nb_nets = len(design.getBitNets())
        nb_instances = len(design.getInstances())
        pandas_data = pd.DataFrame(
            {'Design': [design.getName()],
             'nb_terms': [nb_terms],
             'nb_nets': [nb_nets],
             'nb_instances': [nb_instances]})
    plot = pandas_data.plot.bar(x='Design', y=['nb_terms', 'nb_nets', 'nb_instances'], rot=0)

def edit():
    universe = snl.SNLUniverse.get()
    topLibrary = universe.getTopLibrary() #top library contains top design

    plot_design_stats(topLibrary)