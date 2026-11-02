import argparse
import json
from datetime import datetime
from pathlib import Path

import numpy as np
import torch
from torchlogix.circuit import Circuit
from torchlogix.utils import set_export_mode

from data import get_dataloaders
from models import MODEL_REGISTRY


def load_model(run):
    """Rebuild the model of a training run and load its trained weights."""
    config = json.loads((run / "config.json").read_text())
    model_cls = MODEL_REGISTRY[config["model"]]
    state = torch.load(run / "model.pt", map_location="cpu")
    model = model_cls(state["0.thresholds"])  # thresholds are stored in the state dict
    model.load_state_dict(state)
    return model.eval(), config


def main(args):
    run = Path(args.run)
    model, config = load_model(run)
    # the circuit takes bits, so binarization stays outside of it
    binarize, logic = model[0], torch.nn.Sequential(*list(model)[1:])

    n_bits = model[1].in_dim
    set_export_mode(logic)
    circuit = Circuit.from_model(logic, input_shape=(n_bits,))
    print(f"original circuit: {circuit}")
    circuit.simplify()
    print(f"simplified circuit: {circuit}")

    # check the compiled circuit against the pytorch model on the run's validation split
    _, val_loader = get_dataloaders(config["data_dir"], config["n_events"], config["batch_size"], seed=config["seed"])
    x, y = map(torch.cat, zip(*val_loader))
    bits = binarize(x).bool()
    with torch.no_grad():
        t0 = datetime.now()
        preds = logic(bits)
        print(f"pytorch model:  {datetime.now() - t0}")

    circuit.compile(opt_level=args.opt_level)
    t0 = datetime.now()
    preds_compiled = torch.from_numpy(circuit(bits.numpy(), use_compiled=True)).float()
    print(f"compiled model: {datetime.now() - t0} ({len(bits)} events)")

    assert torch.allclose(preds.float(), preds_compiled, atol=1e-4), "compiled predictions do not match!"
    print(f"outputs match | val acc {(preds_compiled.argmax(-1) == y).float().mean():.4f}")

    (run / "circuit.json").write_text(json.dumps(circuit.to_dict()))
    (run / "circuit.c").write_text(circuit.get_c_code())
    (run / "circuit.v").write_text(circuit.get_verilog_code())
    np.savetxt(run / "thresholds.csv", binarize.thresholds.numpy(), delimiter=",")
    print(f"saved circuit.json, circuit.c, circuit.v, thresholds.csv to {run}")


if __name__ == "__main__":
    p = argparse.ArgumentParser(description="Load a trained model and compile it to a logic circuit")
    p.add_argument("run", help="run directory containing config.json and model.pt")
    p.add_argument("--opt-level", type=int, default=1, help="C compiler optimization level")
    main(p.parse_args())
