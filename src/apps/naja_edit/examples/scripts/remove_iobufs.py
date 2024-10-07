from naja import snl

def remove_io_bufs(design):
    for ins in design.getInstances():
        model = ins.getModel()
        if model.isPrimitive():
            model_name = model.getName()
            if model_name in ['IBUF', 'BUFG']:
                input_net = ins.getInstTerm(model.getScalarTerm('I')).getNet()
                output_net = ins.getInstTerm(model.getScalarTerm('O')).getNet()
                for component in output_net.getComponents():
                    component.setNet(input_net)
                output_net.destroy()
                ins.destroy()
            elif model_name == 'OBUF':
                input_net = ins.getInstTerm(model.getScalarTerm('I')).getNet()
                output_net = ins.getInstTerm(model.getScalarTerm('O')).getNet()
                for component in input_net.getComponents():
                    component.setNet(output_net)
                input_net.destroy()
                ins.destroy()

def edit():
    universe = snl.SNLUniverse.get()
    top = universe.getTopDesign()

    remove_io_bufs(top)