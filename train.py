import argparse
import csv
import json
from datetime import datetime
from pathlib import Path
from tqdm import tqdm

import torch

from data import get_dataloaders
from models import MODEL_REGISTRY


def evaluate(model, loader, loss_fn, device, discrete=True):
    """Return (loss, accuracy) on loader; discrete=True uses hard gates, False the relaxed model.
    For the baseline both modes are identical."""
    model.train(not discrete)
    total_loss = correct = total = 0
    with torch.no_grad():
        for x, y in loader:
            x, y = x.to(device), y.to(device)
            out = model(x)
            total_loss += loss_fn(out, y).item() * len(y)
            correct += (out.argmax(-1) == y).sum().item()
            total += len(y)
    model.train()
    return total_loss / total, correct / total


def main(args):
    torch.manual_seed(args.seed)

    out_dir = Path(args.output or f"runs/{args.model}_{datetime.now():%Y%m%d-%H%M%S}")
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "config.json").write_text(json.dumps(vars(args), indent=2))
    log_file = open(out_dir / "metrics.csv", "w", newline="")
    log = csv.writer(log_file)
    log.writerow(["epoch", "train_loss", "train_acc", "val_loss_relaxed", "val_acc_relaxed",
                  "val_loss_discrete", "val_acc_discrete"])

    train_loader, val_loader = get_dataloaders(args.data_dir, args.n_events, args.batch_size, seed=args.seed)

    # input preprocessing (thresholds / standardization) is fit on the training set
    x_train = torch.cat([x for x, _ in train_loader])
    model = MODEL_REGISTRY[args.model].from_train_data(x_train).to(args.device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    loss_fn = torch.nn.CrossEntropyLoss()

    for epoch in range(args.epochs):
        total_loss = correct = total = 0
        for x, y in tqdm(train_loader):
            x, y = x.to(args.device), y.to(args.device)
            out = model(x)
            loss = loss_fn(out, y)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            total_loss += loss.item() * len(y)
            correct += (out.argmax(-1) == y).sum().item()
            total += len(y)

        train_loss, train_acc = total_loss / total, correct / total  # relaxed model, running average
        val_loss_r, val_acc_r = evaluate(model, val_loader, loss_fn, args.device, discrete=False)
        val_loss_d, val_acc_d = evaluate(model, val_loader, loss_fn, args.device, discrete=True)
        log.writerow([epoch + 1, train_loss, train_acc, val_loss_r, val_acc_r, val_loss_d, val_acc_d])
        log_file.flush()
        print(f"epoch {epoch + 1:3d} | train loss {train_loss:.4f} acc {train_acc:.4f} "
              f"| val relaxed loss {val_loss_r:.4f} acc {val_acc_r:.4f} "
              f"| val discrete loss {val_loss_d:.4f} acc {val_acc_d:.4f}")

    log_file.close()
    torch.save(model.state_dict(), out_dir / "model.pt")
    print(f"saved to {out_dir}")


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--data-dir", default="data")
    p.add_argument("--model", default="LogicModelSmall", choices=list(MODEL_REGISTRY.keys()))
    p.add_argument("--n-events", type=int, default=None, help="events per process (default: all)")
    p.add_argument("--epochs", type=int, default=20)
    p.add_argument("--batch-size", type=int, default=128)
    p.add_argument("--lr", type=float, default=0.01)
    p.add_argument("--device", default="cpu")
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--output", default=None, help="run directory (default: runs/<model>_<timestamp>)")
    args = p.parse_args()
    main(args)
