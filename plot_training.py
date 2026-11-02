import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

CURVES = {  # column suffix -> (label, color)
    "train": ("train (relaxed)", "#2a78d6"),
    "val_relaxed": ("val relaxed", "#eb6834"),
    "val_discrete": ("val discrete", "#1baf7a"),
}


def main(args):
    run = Path(args.run)
    m = pd.read_csv(run / "metrics.csv")

    fig, axs = plt.subplots(1, 2, figsize=(11, 4), sharex=True)
    for ax, metric in zip(axs, ["loss", "acc"]):
        for key, (label, color) in CURVES.items():
            col = f"train_{metric}" if key == "train" else f"{key.replace('val_', f'val_{metric}_')}"
            ax.plot(m["epoch"], m[col], color=color, linewidth=2, label=label)
        ax.set_xlabel("epoch")
        ax.set_ylabel("cross-entropy loss" if metric == "loss" else "accuracy")
        ax.grid(alpha=0.3)
        for side in ("top", "right"):
            ax.spines[side].set_visible(False)
    axs[1].legend(frameon=False)
    fig.suptitle(run.name)
    fig.tight_layout()

    out = Path(args.output) if args.output else run / "training_curves.png"
    fig.savefig(out, dpi=150)
    print(f"saved to {out}")
    if args.show:
        plt.show()


if __name__ == "__main__":
    p = argparse.ArgumentParser(description="Plot loss/accuracy curves from a training run")
    p.add_argument("run", help="run directory containing metrics.csv")
    p.add_argument("--output", default=None, help="image path (default: <run>/training_curves.png)")
    p.add_argument("--show", action="store_true", help="also open an interactive window")
    main(p.parse_args())
