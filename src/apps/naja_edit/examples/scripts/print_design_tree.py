from naja import snl

def print_design_tree(design):
    for ins in design.getInstances():
        print(f"Instance: {ins.getName()}")
        model = ins.getModel()
        for term in design.getTerms():
            print(f"  Terminal: {term}")
        for net in design.getNets():
            print(f"  Net: {net}")
            for bit in net.getBits():
                for component in bit.getComponents():
                    print(f"    Component: {component}")
        print_design_tree(model)

def edit():
    universe = snl.SNLUniverse.get()
    top = universe.getTopDesign()

    print_design_tree(top)