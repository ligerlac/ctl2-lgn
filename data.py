import pandas as pd
import torch
from torch.utils.data import DataLoader, TensorDataset, random_split

CLASSES = ["minbias", "ttbar", "hllhc-z"]
N_PARTICLES = 144
FEATURES = ["pt", "eta", "phi"]


def load_events(path, n_events=None):
    """Return (n_events, N_PARTICLES * len(FEATURES)) float tensor from a puppi csv.
    n_events=None reads the whole file."""
    nrows = None if n_events is None else n_events * N_PARTICLES
    df = pd.read_csv(path, skipinitialspace=True, usecols=FEATURES, nrows=nrows)
    arr = df[FEATURES].to_numpy("float32")
    arr = arr[: len(arr) // N_PARTICLES * N_PARTICLES]  # drop a trailing partial event
    return torch.from_numpy(arr.reshape(-1, N_PARTICLES * len(FEATURES)))


def get_dataloaders(data_dir="data", n_events=None, batch_size=128, val_frac=0.2, seed=0):
    xs, ys = [], []
    for label, name in enumerate(CLASSES):
        x = load_events(f"{data_dir}/{name}.csv", n_events)
        xs.append(x)
        ys.append(torch.full((len(x),), label, dtype=torch.long))
    ds = TensorDataset(torch.cat(xs), torch.cat(ys))

    n_val = int(len(ds) * val_frac)
    train_ds, val_ds = random_split(ds, [len(ds) - n_val, n_val],
                                    generator=torch.Generator().manual_seed(seed))
    return (DataLoader(train_ds, batch_size=batch_size, shuffle=True, drop_last=True),
            DataLoader(val_ds, batch_size=batch_size))
