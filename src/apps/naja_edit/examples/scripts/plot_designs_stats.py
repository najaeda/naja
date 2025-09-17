from naja import snl
import pandas as pd
import matplotlib.pyplot as plt


def plot_design_stats(library):
    data_list = []
    for design in library.getDesigns():
        nb_terms = sum(1 for _ in design.getBitTerms())
        nb_nets = sum(1 for _ in design.getBitNets())
        nb_instances = sum(1 for _ in design.getInstances())
        data_list.append({
            'design': design.getName(),
            'nb_terms': nb_terms,
            'nb_nets': nb_nets,
            'nb_instances': nb_instances
        })
    pandas_data = pd.DataFrame(data_list).set_index('design')
    plot = pandas_data.plot.bar(y=['nb_terms', 'nb_nets', 'nb_instances'], stacked=True)

    # Set title and labels
    plot.set_title('Design Statistics', fontsize=16, fontweight='bold')
    plot.set_xlabel('Design Name', fontsize=12)
    plot.set_ylabel('Count', fontsize=12)

    plot_figure = plot.get_figure()
    plot_figure.tight_layout()
    plot_figure.savefig('design_stats.png')


def edit():
    universe = snl.SNLUniverse.get()
    topDesign = universe.getTopDesign()
    topLibrary = topDesign.getLibrary() #top library contains top design

    plot_design_stats(topLibrary)