import math
import torch
from torch import nn
from torchlogix.layers import Binarization, FixedBinarization, LogicDense, GroupSum


MODEL_REGISTRY = {}

def register_model(cls):
    MODEL_REGISTRY[cls.__name__] = cls
    return cls


class LogicModel(nn.Sequential):
    n_input_bits: int | None = None
    widths: list[int] | None = None
    tau: float | None = None
    in_dim: int = 144 * 3
    out_dim: int = 3
    layer_kwargs: dict = {}

    def __init__(self, thresholds):
        w_last = self.widths[-1]
        assert w_last % self.out_dim == 0, f"last width {w_last} must be divisible by out_dim {self.out_dim}"
        bits_per_group = w_last / self.out_dim
        lk = self.layer_kwargs

        if self.tau is None:
            self.tau = math.sqrt(bits_per_group) / 2.

        dims = [self.in_dim * self.n_input_bits] + self.widths
        super(LogicModel, self).__init__(
            FixedBinarization(thresholds),
            *[LogicDense(in_dim=d_in, out_dim=d_out, **lk) for d_in, d_out in zip(dims[:-1], dims[1:])],
            LogicDense(in_dim=self.widths[-1], out_dim=w_last, **lk),
            GroupSum(k=self.out_dim, tau=self.tau)
        )

    @classmethod
    def from_train_data(cls, x_train):
        """Build the model with per-feature quantile thresholds from the training set."""
        thresholds = Binarization.get_initial_thresholds(
            x_train, num_bits=cls.n_input_bits, one_per="feature", method="distributive")
        return cls(thresholds)


@register_model
class LogicModelTiny(LogicModel):
    n_input_bits = 4
    widths = [1024, 512, 258]


@register_model
class LogicModelSmall(LogicModel):
    n_input_bits = 4
    widths = [4000, 4000, 3000]


@register_model
class LogicModelMedium(LogicModel):
    n_input_bits = 4
    widths = [16_000, 16_000, 15_000]


class Standardize(nn.Module):
    def __init__(self, mean, std):
        super().__init__()
        self.register_buffer("mean", mean)
        self.register_buffer("std", std)

    def forward(self, x):
        return (x - self.mean) / self.std


@register_model
class BaselineMLP(nn.Sequential):
    """Plain float MLP (no logic gates, no quantization) as an accuracy reference."""
    widths: list[int] = [4, 2]
    in_dim: int = 144 * 3
    out_dim: int = 3

    def __init__(self, mean, std):
        dims = [self.in_dim] + self.widths
        layers = [Standardize(mean, std)]
        for d_in, d_out in zip(dims[:-1], dims[1:]):
            layers += [nn.Linear(d_in, d_out), nn.ReLU()]
        super().__init__(*layers, nn.Linear(dims[-1], self.out_dim))

    @classmethod
    def from_train_data(cls, x_train):
        """Build the model with per-feature standardization from the training set."""
        return cls(x_train.mean(0), x_train.std(0).clamp_min(1e-6))
